#pragma once

#include "xray/xray.hpp"

#include <span>
#include <vulkan/vulkan_core.h>

#include <strong_type/strong_type.hpp>
#include <strong_type/bitarithmetic.hpp>
#include <strong_type/convertible_to.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/formattable.hpp>
#include <strong_type/hashable.hpp>

namespace xray::rendering {

using WorkPackageHandle =
    strong::type<uint32_t, struct WorkPackageHandle_tag, strong::equality, strong::formattable, strong::hashable>;

} // namespace xray::rendering
