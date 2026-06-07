#pragma once

#include <initializer_list>

#include "xray/xray.hpp"
#include "xray/base/memory.arena.hpp"

namespace xray::base {

struct ThreadLocalContext {
	static constexpr uint32_t kMaxArenaCount = 4;

	MemoryArena* arena_store[kMaxArenaCount];
	static xray::base::ScratchPadArena acquire_scratchpad(std::initializer_list<xray::base::MemoryArena*> in_use);
};

}  // namespace xray::base
