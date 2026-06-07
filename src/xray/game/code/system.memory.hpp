#pragma once

#include <atomic_queue/atomic_queue.h>

#include "xray/xray.hpp"
#include "xray/base/xray.slice.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/xray.debug.hpp"

namespace B5 {

struct GlobalMemoryConfig {
	uint32_t medium_arenas{8};
	uint32_t large_arenas{4};
	uint32_t small_arenas{16};
	uint32_t large_arena_size{64};
	uint32_t medium_arena_size{32};
	uint32_t small_arena_size{16};

	static constexpr size_t LARGE_ARENA_SIZE  = 64 * 1024 * 1024;
	static constexpr size_t MEDIUM_ARENA_SIZE = 32 * 1024 * 1024;
	static constexpr size_t SMALL_ARENA_SIZE  = 16 * 1024 * 1024;
};

struct LargeArenaTag {};
struct MediumArenaTag {};
struct SmallArenaTag {};

template <typename TagType>
struct ScopedMemoryArena {
	ScopedMemoryArena(xray::U8* backing_buffer, xray::ISIZE backing_buffer_length) noexcept
		: arena{xray::base::MemoryArena::create_with_storage_block(
			  xray::base::slice_from_ptr_and_len(backing_buffer, backing_buffer_length)
		  )} {}

	explicit ScopedMemoryArena(xray::base::xrSlice_t<xray::U8> s) noexcept
		: ScopedMemoryArena{s.data(), s.size_bytes()} {}
	~ScopedMemoryArena();

	xray::base::MemoryArena* arena;
};

using ScopedSmallArenaType	= ScopedMemoryArena<SmallArenaTag>;
using ScopedMediumArenaType = ScopedMemoryArena<MediumArenaTag>;
using ScopedLargeArenaType	= ScopedMemoryArena<LargeArenaTag>;

class GlobalMemorySystem {
private:
	GlobalMemorySystem() noexcept;

public:
	static GlobalMemorySystem* instance() noexcept {
		static GlobalMemorySystem the_one_and_only{};
		return &the_one_and_only;
	}

	template <typename TagType>
	ScopedMemoryArena<TagType> grab_arena() {
		if constexpr (std::is_same_v<TagType, LargeArenaTag>) {
			void* mem_ptr{};
			if (!this->free_large.try_pop(mem_ptr)) {
				XRAY_BREAK_IF_DEBUGGER_ATTACHED();
			}

			return ScopedMemoryArena<TagType>{xray::base::slice_from_ptr_and_len(
				static_cast<xray::U8*>(mem_ptr), GlobalMemoryConfig::LARGE_ARENA_SIZE
			)};
		} else if constexpr (std::is_same_v<TagType, MediumArenaTag>) {
			void* mem_ptr{};
			if (!free_medium.try_pop(mem_ptr)) {
				XRAY_BREAK_IF_DEBUGGER_ATTACHED();
			}
			return ScopedMemoryArena<TagType>{xray::base::slice_from_ptr_and_len(
				static_cast<xray::U8*>(mem_ptr), GlobalMemoryConfig::MEDIUM_ARENA_SIZE
			)};
		} else if constexpr (std::is_same_v<TagType, SmallArenaTag>) {
			void* mem_ptr{};
			if (!free_small.try_pop(mem_ptr)) {
				XRAY_BREAK_IF_DEBUGGER_ATTACHED();
			}
			return ScopedMemoryArena<TagType>{xray::base::slice_from_ptr_and_len(
				static_cast<xray::U8*>(mem_ptr), GlobalMemoryConfig::SMALL_ARENA_SIZE
			)};
		} else {
			static_assert(false, "Unhandled arena type");
		}
	}

	template <typename TagType>
	void release_arena(xray::base::MemoryArena& arena) {
		if constexpr (std::is_same_v<TagType, LargeArenaTag>) {
			free_large.push(arena.os_block);
		} else if constexpr (std::is_same_v<TagType, MediumArenaTag>) {
			free_medium.push(arena.os_block);
		} else if constexpr (std::is_same_v<TagType, SmallArenaTag>) {
			free_small.push(arena.os_block);
		} else {
			static_assert(false, "Unhandled arena type");
		}
	}

	auto grab_large_arena() noexcept { return grab_arena<LargeArenaTag>(); }
	auto grab_medium_arena() noexcept { return grab_arena<MediumArenaTag>(); }
	auto grab_small_arena() noexcept { return grab_arena<SmallArenaTag>(); }

	~GlobalMemorySystem() {}

private:
	atomic_queue::AtomicQueue<void*, 16> free_small;
	atomic_queue::AtomicQueue<void*, 16> free_medium;
	atomic_queue::AtomicQueue<void*, 16> free_large;

	GlobalMemorySystem(const GlobalMemorySystem&)			 = delete;
	GlobalMemorySystem& operator=(const GlobalMemorySystem&) = delete;
};

template <typename TagType>
ScopedMemoryArena<TagType>::~ScopedMemoryArena() {
	GlobalMemorySystem::instance()->release_arena<TagType>(*this->arena);
}

}  // namespace B5
