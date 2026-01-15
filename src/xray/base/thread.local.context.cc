#include "xray/base/thread.local.context.hpp"

#include <cassert>
#include <ranges>
#include <tl/optional.hpp>

#include "xray/base/xray.misc.hpp"
#include "xray/base/memory.os.hpp"

namespace xray::base {

thread_local ThreadLocalContext g_thread_local_context;

xray::base::ScratchPadArena ThreadLocalContext::acquire_scratchpad(
	std::initializer_list<xray::base::MemoryArena*> in_use_arenas
) {
	tl::optional<size_t> available_arena;

	for (size_t idx = 0; idx < std::size(g_thread_local_context.arena_store); ++idx) {
		if (auto itr = std::ranges::find(in_use_arenas, g_thread_local_context.arena_store[idx].arena);
			itr == std::end(in_use_arenas)) {
			available_arena = idx;
			break;
		}
	}

	if (!available_arena) {
		return xray::base::ScratchPadArena{nullptr};
	}

	if (!g_thread_local_context.arena_store[*available_arena].arena) {
		assert(g_thread_local_context.arena_store[*available_arena].alloc_block == nullptr);

		const size_t block_size = round_up(sizeof(xray::base::MemoryArena) + megabytes(8), os_get_page_size());

		void* arena_block = os_alloc_mem(block_size);
		if (!arena_block) {
			// DEBUG_PRINT("Failed to allocate memory for new arena");
			return xray::base::ScratchPadArena{nullptr};
		}

		void* arena_store = reinterpret_cast<void*>(
			round_up(reinterpret_cast<uintptr_t>(arena_block), alignof(xray::base::MemoryArena))
		);

		std::byte* arena_mem =
			reinterpret_cast<std::byte*>(reinterpret_cast<uintptr_t>(arena_store) + sizeof(xray::base::MemoryArena));

		const size_t arena_size = block_size - (arena_mem - static_cast<std::byte*>(arena_block));

		xray::base::MemoryArena* new_arena = new (arena_store) xray::base::MemoryArena{arena_mem, arena_size};

		g_thread_local_context.arena_store[*available_arena] = ThreadLocalContext::ArenaContext{
			.arena		 = new_arena,
			.alloc_block = arena_block,
			.block_size	 = block_size,
		};
	}

	return xray::base::ScratchPadArena{g_thread_local_context.arena_store[*available_arena].arena};
}
}  // namespace xray::base
