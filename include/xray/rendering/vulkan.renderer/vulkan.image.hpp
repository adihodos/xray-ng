#pragma once

#include "xray/xray.hpp"

#include <filesystem>
#include <span>

#include <vulkan/vulkan.h>
#include <tl/expected.hpp>

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

struct VulkanImageCreateInfo
{
    const char* tag_name{};
    tl::optional<WorkPackageHandle> wpkg;
    VkImageType type{ VK_IMAGE_TYPE_2D };
    VkImageUsageFlags usage_flags{ VK_IMAGE_USAGE_SAMPLED_BIT };
    VkMemoryPropertyFlags memory_flags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
    VkFormat format{ VK_FORMAT_UNDEFINED };
    bool cubemap{ false };
    bool create_view{ true };
    uint32_t width{};
    uint32_t height{};
    uint32_t depth{ 1 };
    uint32_t layers{ 1 };
    std::span<const std::span<const uint8_t>> pixels{};
};

class VulkanImage
{
  public:
    xrUniqueImageWithMemoryAndView _image;
    VulkanTextureInfo _info;

    VkImage image() const noexcept { return _image.handle<VkImage>(); }
    VkDeviceMemory memory() const noexcept { return _image.handle<VkDeviceMemory>(); }

    tl::expected<xrUniqueVkImageView, VulkanError> create_image_view(const VulkanRenderer& renderer) noexcept;

    std::tuple<VkImage, VkDeviceMemory, VkImageView> release() noexcept { return _image.release(); }

    static tl::expected<VulkanImage, VulkanError> from_memory(VulkanRenderer& renderer,
                                                              const VulkanImageCreateInfo& create_info);
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
