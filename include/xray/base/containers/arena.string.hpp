#pragma once

#include "xray/base/memory.arena.hpp"
#include <string>

namespace xray::base::containers {

template<class CharT, class Traits = std::char_traits<CharT>>
using basic_string = std::basic_string<CharT, Traits, MemoryArenaAllocator<CharT>>;

}

