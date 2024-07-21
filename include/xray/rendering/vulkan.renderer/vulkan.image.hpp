#pragma once

#include "xray/xray.hpp"

#include <tl/expected.hpp>

#include <vulkan/vulkan.h>

#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"

namespace xray::rendering {

class VulkanRenderer;

class ManagedImage
{
  public:
    xrUniqueImageWithMemory _image;
    uint32_t _width;
    uint32_t _height;
    uint32_t _depth;
    VkFormat _format;

    static tl::expected<ManagedImage, VulkanError> create(
        VulkanRenderer& renderer,
        const VkImageCreateInfo& img_create_info,
        const VkMemoryPropertyFlags image_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
};

}
