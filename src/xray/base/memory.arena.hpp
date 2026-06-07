#pragma once

#include "xray/xray.hpp"
#include "xray/base/xray.slice.hpp"

//
// based on this article
// https://www.gingerbill.org/article/2019/02/08/memory-allocation-strategies-002/
namespace xray::base {

struct MemoryStats {
	size_t allocations{};
	size_t largest_alloc{};
	size_t high_water{};
	size_t allocated{};
	size_t freed{};
};

struct MemoryArena {
	static constexpr const size_t DEFAULT_ALIGNMENT = (2 * sizeof(void*));

	enum class BlockType_t : U8 {
		VirtualMem,
		StaticStorageMem,
	};

	U8* os_block{};
	U8* start_ptr{};
	U8* end_ptr{};
	ISIZE allocated_size{};
	ISIZE commit_offset{};
	ISIZE curr_offset{};
	ISIZE prev_offset{};
	MemoryStats stats{};
	BlockType_t block_type{BlockType_t::VirtualMem};

	static MemoryArena* create(const ISIZE commit_size, const ISIZE bytes_size) noexcept;
	static MemoryArena* create_with_storage_block(xrSlice_t<U8> storage_block) noexcept;

	template <typename T>
	[[nodiscard]] T* alloc_align() noexcept {
		return reinterpret_cast<T*>(alloc_align(sizeof(T), alignof(T)).s_ptr);
	}

	template <typename T>
	[[nodiscard]] xrSlice_t<T> alloc_align(const ISIZE items) noexcept {
		xrSlice_t<U8> slice = alloc_align(xr_size_of(T) * items, xr_align_of(T));
		return xrSlice_t<T>{
			.s_ptr = reinterpret_cast<T*>(slice.s_ptr),
			.s_len = items,
		};
	}

	[[nodiscard]] xrSlice_t<U8> alloc_align(const ISIZE size, const ISIZE align) noexcept;

	void free(void* ptr, const ISIZE n) noexcept { this->free(slice_from_ptr_and_len((U8*)ptr, n)); }
	void free(const xrSlice_t<U8> range) noexcept;

	[[nodiscard]] xrSlice_t<U8> resize_align(
		void* ptr, const ISIZE prev_size, const ISIZE new_size, const ISIZE align
	) noexcept;

	void free_all() noexcept;
	void internal_commit_pages(const ISIZE req_size) noexcept;
	ISIZE usable_size() const noexcept { return end_ptr - start_ptr; }
	// MemoryArena(const MemoryArena&)			   = delete;
	// MemoryArena& operator=(const MemoryArena&) = delete;
};

struct ScratchPadArena {
	MemoryArena* arena;
	ISIZE prev_offset;
	ISIZE curr_offset;

	explicit ScratchPadArena(MemoryArena* arena_) noexcept
		: arena{arena_}, prev_offset{arena_->prev_offset}, curr_offset{arena_->curr_offset} {}

	explicit ScratchPadArena(MemoryArena& arena_) noexcept : ScratchPadArena{&arena_} {}

	~ScratchPadArena() {
		arena->prev_offset = prev_offset;
		arena->curr_offset = curr_offset;
	}

	ScratchPadArena(const ScratchPadArena&)			   = delete;
	ScratchPadArena& operator=(const ScratchPadArena&) = delete;
};

template <class T, std::size_t Align = alignof(std::max_align_t)>
class MemoryArenaAllocator {
public:
	using value_type				= T;
	static auto constexpr alignment = Align;

	MemoryArenaAllocator<T, Align> select_on_container_copy_construction() noexcept {
		return MemoryArenaAllocator{this->a_};
	}

private:
	MemoryArena* a_;

public:
	MemoryArenaAllocator(ScratchPadArena& s) noexcept : a_{s.arena} {}
	MemoryArenaAllocator(MemoryArena& a) noexcept : a_{&a} {}

	template <class U>
	MemoryArenaAllocator(const MemoryArenaAllocator<U, alignment>& a) noexcept : a_{a.a_} {}

	T* allocate(std::size_t n) noexcept {
		xrSlice_t<U8> mem_range = a_->alloc_align(n * xr_size_of(T), xr_align_of(T));
		return reinterpret_cast<T*>(mem_range.s_ptr);
	}

	void deallocate(T* p, std::size_t n) noexcept { a_->free(reinterpret_cast<void*>(p), static_cast<ISIZE>(n)); }

	template <class T1, std::size_t A1, class U, std::size_t A2>
	friend bool operator==(const MemoryArenaAllocator<T1, A1>& x, const MemoryArenaAllocator<U, A2>& y) noexcept;

	template <class U, std::size_t A>
	friend class MemoryArenaAllocator;

	template <class _Up>
	struct rebind {
		using other = MemoryArenaAllocator<_Up, alignment>;
	};
};

template <class T, std::size_t A1, class U, std::size_t A2>
inline bool operator==(const MemoryArenaAllocator<T, A1>& x, const MemoryArenaAllocator<U, A2>& y) noexcept {
	return A1 == A2 && x.a_ == y.a_;
}

template <class T, std::size_t A1, class U, std::size_t A2>
inline bool operator!=(const MemoryArenaAllocator<T, A1>& x, const MemoryArenaAllocator<U, A2>& y) noexcept {
	return !(x == y);
}

}  // namespace xray::base
