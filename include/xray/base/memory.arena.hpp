#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#if defined(__has_feature)
#if __has_feature(address_sanitizer) // for clang
// GCC and MSVC already set this
// https://learn.microsoft.com/en-us/cpp/sanitizers/asan-building?view=msvc-160
#define __SANITIZE_ADDRESS__
#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

namespace xray::base::details {

#if defined(__SANITIZE_ADDRESS__)

inline void
poison_memory_region(void* ptr, std::size_t n)
{
  ASAN_POISON_MEMORY_REGION(ptr, n);
}

inline void
unpoison_memory_region(void* ptr, std::size_t n)
{
  ASAN_UNPOISON_MEMORY_REGION(ptr, n);
}
#else

inline void
poison_memory_region(void* ptr, std::size_t n)
{
}

inline void
unpoison_memory_region(void* ptr, std::size_t n)
{
}

#endif

}

//
// based on this article
// https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
namespace xray::base {

inline bool
is_power_of_two(uintptr_t x) noexcept
{
  return (x & (x - 1)) == 0;
}

inline uintptr_t
align_forward(uintptr_t ptr, size_t align)
{
  uintptr_t p, a, modulo;

  assert(is_power_of_two(align));

  p = ptr;
  a = (uintptr_t)align;
  // Same as (p % a) but faster as 'a' is a power of two
  modulo = p & (a - 1);

  if (modulo != 0) {
    // If 'p' address is not aligned, push the address to the
    // next value which is aligned
    p += a - modulo;
  }
  return p;
}

struct MemoryArena
{
  static constexpr const size_t DEFAULT_ALIGNMENT = (2 * sizeof(void*));
  uint8_t* buf{};
  size_t buf_len{};
  size_t prev_offset{};
  size_t curr_offset{};

  MemoryArena(std::span<uint8_t> s) noexcept
    : MemoryArena{ s.data(), s.size_bytes() }
  {
    details::poison_memory_region(this->buf, this->buf_len);
  }

  MemoryArena(uint8_t* backing_buffer, size_t backing_buffer_length) noexcept
    : buf{ backing_buffer }
    , buf_len{ backing_buffer_length }
  {
    details::poison_memory_region(this->buf, this->buf_len);
  }

  ~MemoryArena() { details::unpoison_memory_region(this->buf, this->buf_len); }

  void* alloc_align(size_t size, size_t align) noexcept
  {
    // Align 'curr_offset' forward to the specified alignment
    uintptr_t curr_ptr = (uintptr_t)this->buf + (uintptr_t)this->curr_offset;
    uintptr_t offset = align_forward(curr_ptr, align);
    offset -= (uintptr_t)this->buf; // Change to relative offset

    // Check to see if the backing memory has space left
    if (offset + size <= this->buf_len) {
      void* ptr = &this->buf[offset];
      this->prev_offset = offset;
      this->curr_offset = offset + size;

      details::unpoison_memory_region(ptr, size);

      // Zero new memory by default
      memset(ptr, 0, size);
      return ptr;
    }
    // Return NULL if the arena is out of memory (or handle differently)
    return nullptr;
  }

  void free(void* ptr, std::size_t n) noexcept
  {
    details::unpoison_memory_region(ptr, n);
  }

  void* resize_align(void* old_memory,
                     size_t old_size,
                     size_t new_size,
                     size_t align) noexcept
  {
    unsigned char* old_mem = static_cast<unsigned char*>(old_memory);

    assert(is_power_of_two(align));

    if (old_mem == nullptr || old_size == 0) {
      return alloc_align(new_size, align);
    } else if (this->buf <= old_mem && old_mem < this->buf + buf_len) {
      if (this->buf + this->prev_offset == old_mem) {
        this->curr_offset = this->prev_offset + new_size;
        if (new_size > old_size) {

          details::unpoison_memory_region(&this->buf[this->curr_offset],
                                          new_size - old_size);
          //
          // Zero the new memory by default
          memset(&this->buf[this->curr_offset], 0, new_size - old_size);
        }
        return old_memory;
      } else {
        void* new_memory = alloc_align(new_size, align);
        size_t copy_size = old_size < new_size ? old_size : new_size;
        // Copy across old memory to the new memory
        memmove(new_memory, old_memory, copy_size);
        return new_memory;
      }
    } else {
      assert(0 && "Memory is out of bounds of the buffer in this arena");
      return nullptr;
    }
  }

  void free_all()
  {
    this->curr_offset = 0;
    this->prev_offset = 0;
  }

  MemoryArena(const MemoryArena&) = delete;
  MemoryArena& operator=(const MemoryArena&) = delete;
};

struct ScratchPadArena
{
  MemoryArena* arena;
  size_t prev_offset;
  size_t curr_offset;

  explicit ScratchPadArena(MemoryArena* arena_) noexcept
    : arena{ arena_ }
    , prev_offset{ arena_->prev_offset }
    , curr_offset{ arena_->curr_offset }
  {
  }

  ~ScratchPadArena()
  {
    arena->prev_offset = prev_offset;
    arena->curr_offset = curr_offset;
  }

  ScratchPadArena(const ScratchPadArena&) = delete;
  ScratchPadArena& operator=(const ScratchPadArena&) = delete;
};

template<class T, std::size_t Align = alignof(std::max_align_t)>
class MemoryArenaAllocator
{
public:
  using value_type = T;
  static auto constexpr alignment = Align;

  MemoryArenaAllocator<T, Align>
  select_on_container_copy_construction() noexcept
  {
    return MemoryArenaAllocator{ this->a_ };
  }

private:
  MemoryArena* a_;

public:
  MemoryArenaAllocator(ScratchPadArena& s) noexcept
    : a_{ s.arena }
  {
  }

  MemoryArenaAllocator(MemoryArena& a) noexcept
    : a_{ &a }
  {
  }

  template<class U>
  MemoryArenaAllocator(const MemoryArenaAllocator<U, alignment>& a) noexcept
    : a_{ a.a_ }
  {
  }

  T* allocate(std::size_t n) noexcept
  {
    return reinterpret_cast<T*>(a_->alloc_align(n * sizeof(T), alignof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept
  {
    a_->free(reinterpret_cast<void*>(p), n);
  }

  template<class T1, std::size_t A1, class U, std::size_t A2>
  friend bool operator==(const MemoryArenaAllocator<T1, A1>& x,
                         const MemoryArenaAllocator<U, A2>& y) noexcept;

  template<class U, std::size_t A>
  friend class MemoryArenaAllocator;

  template<class _Up>
  struct rebind
  {
    using other = MemoryArenaAllocator<_Up, alignment>;
  };
};

template<class T, std::size_t A1, class U, std::size_t A2>
inline bool
operator==(const MemoryArenaAllocator<T, A1>& x,
           const MemoryArenaAllocator<U, A2>& y) noexcept
{
  return A1 == A2 && x.a_ == y.a_;
}

template<class T, std::size_t A1, class U, std::size_t A2>
inline bool
operator!=(const MemoryArenaAllocator<T, A1>& x,
           const MemoryArenaAllocator<U, A2>& y) noexcept
{
  return !(x == y);
}

}
