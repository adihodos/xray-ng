#pragma once

#include "xray/base/memory.arena.hpp"
#include <vector>

namespace xray::base::containers {
template<typename T>
using vector = std::vector<T, xray::base::MemoryArenaAllocator<T>>;
}
