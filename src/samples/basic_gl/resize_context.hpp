#pragma once

#include "xray/xray.hpp"
#include <cstdint>

namespace app {

struct resize_context_t {
  uint32_t surface_width;
  uint32_t surface_height;
};

} // namespace app