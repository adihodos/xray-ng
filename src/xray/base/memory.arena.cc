#include "xray/base/memory.arena.hpp"

#include <cstring>
#include <new>

#include "xray/base/xray.os.hpp"
#include "xray/base/xray.debug.hpp"
#include "xray/base/minstd/algo.hpp"
#include "xray/base/logger.hpp"

namespace {
constexpr xray::ISIZE DEFAULT_COMMIT_SIZE = 2 * 1024 * 1024;
}

xray::base::MemoryArena* xray::base::MemoryArena::create(
	const ISIZE initial_commit_size, const ISIZE bytes_size
) noexcept {
	//
	// allocate 1 extra page for the header, it makes things so much simpler
	const ISIZE page_rounded_size = xr_round_up(bytes_size + g_SysInfo->page_size, g_SysInfo->page_size);
	xrSlice_t<U8> reserved_range  = xrSys_ReserveMemory(nullptr, page_rounded_size);
	if (!reserved_range) {
		XRAY_BREAK_IF_DEBUGGER_ATTACHED();
		return nullptr;
	}

	const ISIZE commit_size = minstd::max_of(DEFAULT_COMMIT_SIZE, initial_commit_size);

	reserved_range = xrSys_CommitMemory(reserved_range[slice_to{commit_size}]);
	if (!reserved_range) {
		XRAY_BREAK_IF_DEBUGGER_ATTACHED();
		return nullptr;
	}

	const ISIZE arena_adjust = xr_pad_align(UPTR(reserved_range.s_ptr), xr_align_of(MemoryArena));
	MemoryArena* arena_ptr	 = new (reinterpret_cast<void*>(reserved_range.s_ptr + arena_adjust)) MemoryArena{
		  .os_block		  = reserved_range.s_ptr,
		  .start_ptr	  = reserved_range.s_ptr + g_SysInfo->page_size,
		  .end_ptr		  = reserved_range.s_ptr + page_rounded_size,
		  .allocated_size = page_rounded_size,
		  .commit_offset  = commit_size - g_SysInfo->page_size,
		  .curr_offset	  = 0,
		  .prev_offset	  = 0,
		  .stats		  = {},
		  .block_type	  = BlockType_t::VirtualMem,
	  };

	XR_LOG_INFO(
		"Arena with HEAP storage block (%p - %ld)\n.start_ptr = %p,\n.end_ptr = %p,\n.commit_offset = %ld,\nusable "
		"size = "
		"%ld",
		arena_ptr->os_block,
		arena_ptr->allocated_size,
		arena_ptr->start_ptr,
		arena_ptr->end_ptr,
		arena_ptr->commit_offset,
		arena_ptr->usable_size()
	);

	poison_memory_region(arena_ptr->start_ptr, static_cast<USIZE>(arena_ptr->commit_offset));
	return arena_ptr;
}

xray::base::MemoryArena* xray::base::MemoryArena::create_with_storage_block(xrSlice_t<U8> storage_block) noexcept {
	const ISIZE arena_adjust = xr_pad_align(UPTR(storage_block.s_ptr), xr_align_of(MemoryArena));
	MemoryArena* arena_ptr	 = new (reinterpret_cast<void*>(storage_block.s_ptr + arena_adjust)) MemoryArena{
		  .os_block		  = storage_block.s_ptr,
		  .start_ptr	  = storage_block.s_ptr + arena_adjust + xr_size_of(MemoryArena),
		  .end_ptr		  = storage_block.s_ptr + storage_block.s_len,
		  .allocated_size = storage_block.s_len,
		  .commit_offset  = storage_block.s_len,
		  .curr_offset	  = 0,
		  .prev_offset	  = 0,
		  .stats		  = {},
		  .block_type	  = BlockType_t::StaticStorageMem,
	  };

	XR_LOG_INFO(
		"Arena with static storage block (%p - %ld)\n.start_ptr = %p,\n.end_ptr = %p,\n.commit_offset = %ld,\nusable "
		"size = "
		"%ld",
		storage_block.s_ptr,
		storage_block.s_len,
		arena_ptr->start_ptr,
		arena_ptr->end_ptr,
		arena_ptr->commit_offset,
		arena_ptr->usable_size()
	);

	poison_memory_region(arena_ptr->start_ptr, arena_ptr->end_ptr - arena_ptr->start_ptr);
	return arena_ptr;
}

void xray::base::MemoryArena::internal_commit_pages(const ISIZE req_size) noexcept {
	if (block_type == BlockType_t::StaticStorageMem) {
		return;
	}

	const ISIZE commit_diff = this->commit_offset - this->curr_offset - req_size;
	if (commit_diff < 0) {
		UPTR start_addr = reinterpret_cast<UPTR>(this->start_ptr + this->curr_offset);
		if (!xr_is_multiple_of(start_addr, UPTR(g_SysInfo->page_size))) {
			start_addr = xr_round_down(start_addr, UPTR(g_SysInfo->page_size));
		}

		XRAY_ASSERT_NOMSG(start_addr <= UPTR(this->end_ptr));

		const ISIZE remaining_bytes = this->end_ptr - reinterpret_cast<U8*>(start_addr);
		if (remaining_bytes <= 0) {
			// log.fatalf("Arena {} request to commit {} bytes exceeds capacity!", arena, req_size)
			return;
		}

		const ISIZE bytes_to_commit = minstd::min_of(xr_round_up(req_size, DEFAULT_COMMIT_SIZE), remaining_bytes);

		const xrSlice_t<U8> commited_range =
			xrSys_CommitMemory(slice_from_ptr_and_len(reinterpret_cast<U8*>(start_addr), bytes_to_commit));
		if (!commited_range) {
			// log.assertf(op_result == 0, "Commit pages @ {:x}, size {} failure", start_addr, bytes_to_commit)
			XRAY_BREAK_IF_DEBUGGER_ATTACHED();
			return;
		}

		U8* prev_commit_end			= this->start_ptr + this->commit_offset;
		U8* curr_commit_end			= reinterpret_cast<U8*>(start_addr) + bytes_to_commit;
		const ISIZE bytes_to_poison = curr_commit_end - prev_commit_end;
		XRAY_ASSERT_NOMSG(bytes_to_poison > 0);

		// log.infof("commit pages: bytes to poison {:p} -> {:p} ({})", prev_commit_end, curr_commit_end,
		// bytes_to_poison)

		this->commit_offset += bytes_to_poison;
		poison_memory_region(reinterpret_cast<void*>(prev_commit_end), static_cast<USIZE>(bytes_to_poison));
	}
}

[[nodiscard]] xray::base::xrSlice_t<xray::U8> xray::base::MemoryArena::alloc_align(
	const ISIZE size, const ISIZE align
) noexcept {
	MemoryArena* arena			= this;
	const UPTR free_start		= reinterpret_cast<UPTR>(arena->start_ptr) + arena->curr_offset;
	const ISIZE align_padding	= static_cast<ISIZE>(xr_pad_align(free_start, static_cast<UPTR>(align)));
	const ISIZE bytes_remaining = reinterpret_cast<UPTR>(arena->end_ptr) - (free_start + align_padding) - size;

	if (bytes_remaining < 0) {
		XRAY_BREAK_IF_DEBUGGER_ATTACHED();
		// log.fatalf("Arena {} request for {} bytes, align {} exceeds capacity!", arena, bytes, align)
		return xrSlice_t<U8>{};
	}

	arena->internal_commit_pages(align_padding + size);
	arena->prev_offset = arena->curr_offset;
	arena->curr_offset += align_padding + size;
	//
	// TODO: stats
	arena->stats.allocations += 1;
	arena->stats.allocated += size;
	arena->stats.largest_alloc = minstd::max_of(arena->stats.largest_alloc, static_cast<USIZE>(size));
	arena->stats.high_water	   = minstd::max_of(arena->stats.high_water, arena->stats.allocated);

	U8* aligned_ptr_start = reinterpret_cast<U8*>(free_start + align_padding);
	unpoison_memory_region(aligned_ptr_start, size);
	return slice_from_ptr_and_len(aligned_ptr_start, size);
}

void xray::base::MemoryArena::free(const xrSlice_t<U8> range) noexcept {
	U8* ptr_start = range.s_ptr;
	U8* ptr_end	  = ptr_start + range.s_len;

	MemoryArena* arena = this;
	if (ptr_start < arena->start_ptr || ptr_end > arena->end_ptr) {
		XRAY_BREAK_IF_DEBUGGER_ATTACHED();
		// log.fatalf("Arena {} : request to free range {} <-> {} not from this arena!", arena, ptr_start, ptr_end)
		return;
	}

	if (ptr_end == (arena->start_ptr + arena->curr_offset)) {
		//
		// last allocation can be reclaimed
		arena->curr_offset = arena->prev_offset;
		arena->prev_offset = minstd::clamp(arena->prev_offset - range.s_len, ISIZE{0}, arena->curr_offset);
	}

	unpoison_memory_region(ptr_start, static_cast<USIZE>(range.s_len));
}

[[nodiscard]] xray::base::xrSlice_t<xray::U8> xray::base::MemoryArena::resize_align(
	void* ptr, const ISIZE prev_size, const ISIZE new_size, const ISIZE align
) noexcept {
	MemoryArena* arena = this;

	if (ptr == nullptr) {
		return arena->alloc_align(new_size, align);
	}

	if (new_size == 0) {
		arena->free(slice_from_ptr_and_len(static_cast<U8*>(ptr), prev_size));
		return xrSlice_t<U8>{};
	}

	U8* p = static_cast<U8*>(ptr);

	if (p < arena->start_ptr || p > arena->end_ptr) {
		// log.fatalf("Arena {}: trying to realloc pointer {:p} ({} bytes) from a different arena!", arena, ptr,
		// prev_size) runtime.debug_trap()
		XRAY_BREAK_IF_DEBUGGER_ATTACHED();
	}

	if ((p + prev_size) == (arena->start_ptr + arena->curr_offset)) {
		const ISIZE offset_adjust = new_size - prev_size;
		if (offset_adjust > 0) {
			//
			// grow
			arena->internal_commit_pages(offset_adjust);
			unpoison_memory_region(arena->start_ptr + arena->curr_offset, static_cast<USIZE>(offset_adjust));
		} else {
			//
			// shrink
			poison_memory_region(arena->start_ptr + arena->curr_offset + offset_adjust, -offset_adjust);
		}
		arena->curr_offset += offset_adjust;
		return slice_from_ptr_and_len(static_cast<U8*>(ptr), new_size);
	} else {
		xrSlice_t<U8> new_ptr = arena->alloc_align(new_size, align);
		if (new_ptr) {
			memcpy(new_ptr.s_ptr, ptr, prev_size);
			arena->free(ptr, prev_size);
		}
		return new_ptr;
	}
}

void xray::base::MemoryArena::free_all() noexcept {
	this->curr_offset		= 0;
	this->prev_offset		= 0;
	this->stats.allocated	= 0;
	this->stats.allocations = 0;
	poison_memory_region(this->start_ptr, this->end_ptr - this->start_ptr);
}
