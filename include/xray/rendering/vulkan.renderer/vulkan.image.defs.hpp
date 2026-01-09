#pragma once

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <span>
#include <vulkan/vulkan_core.h>
#include <tl/optional.hpp>

namespace xray::rendering {

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
    VkCommandBuffer cmd_buf;
    std::filesystem::path path;
    VkImageUsageFlags usage_flags;
    VkImageLayout final_layout;
    VkImageTiling tiling;
};

struct VulkanImageCreateInfo
{
    const char* tag_name{};
    tl::optional<VkCommandBuffer> wpkg;
    VkImageType type{ VK_IMAGE_TYPE_2D };
    VkImageUsageFlags usage_flags{ VK_IMAGE_USAGE_SAMPLED_BIT };
    VkMemoryPropertyFlags memory_flags{ VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	VkFormat format{VK_FORMAT_UNDEFINED};
	VkImageLayout initial_layout{VK_IMAGE_LAYOUT_UNDEFINED};
    bool cubemap{ false };
    bool create_view{ true };
    uint32_t width{};
    uint32_t height{};
    uint32_t depth{ 1 };
    uint32_t layers{ 1 };
    std::initializer_list<const std::span<const uint8_t>> pixels =
        std::initializer_list<const std::span<const uint8_t>>{};
};

} // namespace xray::rendering
