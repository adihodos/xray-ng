#pragma once

#include "xray/xray.hpp"

#include <utility>
#include <filesystem>

#include <tl/expected.hpp>

#include <vulkan/vulkan.h>

#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.work.package.hpp"

namespace xray::rendering {

class VulkanRenderer;

struct VulkanTextureInfo
{
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    VkImageLayout imageLayout;
    VkFormat imageFormat;
    uint32_t levelCount;
    uint32_t layerCount;
    VkImageViewType viewType;
};

struct VulkanImageLoadInfo
{
    const char* tag_name{};
    WorkPackageHandle wpkg;
    std::filesystem::path path;
    VkImageUsageFlags usage_flags;
    VkImageLayout final_layout;
    VkImageTiling tiling;
};

class VulkanImage
{
  public:
    xrUniqueImageWithMemory _image;
    VulkanTextureInfo _info;

    VkImage image() const noexcept { return _image.handle<VkImage>(); }
    VkDeviceMemory memory() const noexcept { return _image.handle<VkDeviceMemory>(); }

    std::tuple<VkImage, VkDeviceMemory> release() noexcept { return _image.release(); }

    // static tl::expected<ManagedImage, VulkanError> create(
    //     VulkanRenderer& renderer,
    //     const VkImageCreateInfo& img_create_info,
    //     const VkImageViewType view_type,
    //     const VkMemoryPropertyFlags image_memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    static tl::expected<VulkanImage, VulkanError> from_file(VulkanRenderer& renderer,
                                                            const VulkanImageLoadInfo& load_info);
};

VkImageMemoryBarrier2
make_image_layout_memory_barrier(const VkImage image,
                                 const VkImageLayout previous_layout,
                                 const VkImageLayout new_layout,
                                 const VkImageSubresourceRange subresource_range);

void
set_image_layout(VkCommandBuffer cmd_buffer,
                 VkImage image,
                 const VkImageLayout initial_layout,
                 const VkImageLayout final_layout,
                 const VkImageSubresourceRange& subresource_range);

}
