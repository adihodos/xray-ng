
#pragma once

#include "xray/base/memory.arena.hpp"
#include <unordered_set>

namespace xray::base::containers {

template<class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_set = std::unordered_set<Key, Hash, KeyEqual, MemoryArenaAllocator<Key>>;

}
