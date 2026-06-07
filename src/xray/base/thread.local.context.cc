#include "xray/base/thread.local.context.hpp"

#include "xray/base/logger.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/xray.debug.hpp"

namespace xray::base {

thread_local ThreadLocalContext g_thread_local_context;

xray::base::ScratchPadArena ThreadLocalContext::acquire_scratchpad(
	std::initializer_list<xray::base::MemoryArena*> in_use_arenas
) {
	MemoryArena** curr_arena = nullptr;

	for (ISIZE idx = 0; idx < xr_array_size(g_thread_local_context.arena_store); ++idx) {
		curr_arena = &g_thread_local_context.arena_store[idx];

		for (MemoryArena* used_arena : in_use_arenas) {
			if (*curr_arena == used_arena) {
				curr_arena = nullptr;
				break;
			}
		}

		//
		// Available arena found
		if (curr_arena) {
			break;
		}
	}

	//
	// No available arenas
	if (!curr_arena) {
		XR_LOG_CRITICAL("Arena pool exhausted!");
		return ScratchPadArena{nullptr};
	}

	//
	// Found available arena, needs to be initialized
	if (!*curr_arena) {
		//
		// TODO: remove hard-coded values
		*curr_arena = MemoryArena::create(2 * 1024 * 1024, 16 * 1024 * 1024);
		if (!*curr_arena) {
			XR_LOG_CRITICAL("Failed to create memory arena!");
			XRAY_BREAK_IF_DEBUGGER_ATTACHED();
		}
	}

	return ScratchPadArena{*curr_arena};
}

}  // namespace xray::base
