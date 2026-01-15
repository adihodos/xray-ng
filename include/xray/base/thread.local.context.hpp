#pragma once

#include <cstddef>
#include <initializer_list>

#include "xray/base/memory.arena.hpp"

namespace xray::base {

struct ThreadLocalContext {
	static constexpr uint32_t kMaxArenaCount = 4;

	struct ArenaContext {
		xray::base::MemoryArena* arena{};
		void* alloc_block{};
		size_t block_size{};
	};

	ArenaContext arena_store[kMaxArenaCount];

	static xray::base::ScratchPadArena acquire_scratchpad(std::initializer_list<xray::base::MemoryArena*> in_use);
};

}  // namespace xray::base
