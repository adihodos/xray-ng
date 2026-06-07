
#pragma once

#include "xray/base/memory.arena.hpp"
#include <unordered_map>

namespace xray::base::containers {

template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using unordered_map = std::unordered_map<Key, T, Hash, KeyEqual, MemoryArenaAllocator<std::pair<const Key, T>>>;

}
