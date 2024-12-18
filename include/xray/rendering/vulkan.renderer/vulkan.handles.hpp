#pragma once

#include <strong_type/strong_type.hpp>
#include <strong_type/bitarithmetic.hpp>
#include <strong_type/convertible_to.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/formattable.hpp>
#include <strong_type/hashable.hpp>

#include <vulkan/vulkan_core.h>

namespace xray::rendering {

using WorkPackageHandle =
    strong::type<uint32_t, struct WorkPackageHandle_tag, strong::equality, strong::formattable, strong::hashable>;

using StagingBufferHandle =
    strong::type<uint32_t, struct StagingBufferHandle_tag, strong::equality, strong::formattable, strong::hashable>;

using StagingBufferMemoryHandle = strong::
    type<uint32_t, struct StagingBufferMemoryHandle_tag, strong::equality, strong::formattable, strong::hashable>;

using CommandBufferHandle =
    strong::type<uint32_t, struct CommandBufferHandle_tag, strong::equality, strong::formattable, strong::hashable>;

using FenceHandle =
    strong::type<uint32_t, struct FenceHandle_tag, strong::equality, strong::formattable, strong::hashable>;

using SubmitHandle =
    strong::type<uint32_t, struct SubmitHandle_tag, strong::equality, strong::formattable, strong::hashable>;

} // namespace xray::rendering
