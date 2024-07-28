#pragma once

#include "xray/xray.hpp"

#include <span>
#include <vulkan/vulkan_core.h>

namespace xray::rendering {

struct QueuableWorkPackage
{
    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    VkCommandBuffer command_buffer;
    std::span<VkBufferImageCopy> copy_region;
};

struct WorkPackageFuture
{
    VkFence fence;
};

} // namespace xray::rendering
