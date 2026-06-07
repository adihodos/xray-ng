#pragma once

#include "xray/xray.hpp"
#include "xray/base/containers/arena.string.hpp"

namespace xray::base {

XRAY_ATTRIBUTE_FORMAT(2, 3)
void format_to(xray::base::containers::string& str, const char* fmt_spec, ...);

XRAY_ATTRIBUTE_FORMAT(1, 2)
std::string format_to_string(const char* fmt_spec, ...);

XRAY_ATTRIBUTE_FORMAT(2, 3)
containers::string format_to_string(MemoryArena& arena, const char* fmt_spec, ...);

}  // namespace xray::base
