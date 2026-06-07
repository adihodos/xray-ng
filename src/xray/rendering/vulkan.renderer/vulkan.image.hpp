#pragma once

#include "xray/xray.hpp"

#include <vulkan/vulkan_core.h>
#include <tl/expected.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.defs.hpp"

namespace xray::rendering {

class VulkanRenderer;

class VulkanImage {
public:
	xrUniqueImageWithMemoryAndView _image;
	VulkanTextureInfo _info;

	VulkanImage(xrUniqueImageWithMemoryAndView image, const VulkanTextureInfo& info) noexcept
		: _image{std::move(image)}, _info{info} {}

	VulkanImage(VulkanImage&&) = default;

	VkImage image() const noexcept { return _image.handle<VkImage>(); }
	VkDeviceMemory memory() const noexcept { return _image.handle<VkDeviceMemory>(); }
	VkImageView view() const noexcept { return _image.handle<VkImageView>(); }
	const VulkanTextureInfo& info() const noexcept { return _info; }

	tl::expected<xrUniqueVkImageView, VulkanError> create_image_view(const VulkanRenderer& renderer) noexcept;

	std::tuple<VkImage, VkDeviceMemory, VkImageView> release() noexcept { return _image.release(); }

	static tl::expected<VulkanImage, VulkanError> from_memory(
		VulkanRenderer& renderer, const VulkanImageCreateInfo& create_info
	);
	static tl::expected<VulkanImage, VulkanError> from_file(
		VulkanRenderer& renderer, const VulkanImageLoadInfo& load_info
	);
};

VkImageMemoryBarrier2 make_image_layout_memory_barrier(
	const VkImage image,
	const VkImageLayout previous_layout,
	const VkImageLayout new_layout,
	const VkImageSubresourceRange subresource_range
);

void set_image_layout(
	VkCommandBuffer cmd_buffer,
	VkImage image,
	const VkImageLayout initial_layout,
	const VkImageLayout final_layout,
	const VkImageSubresourceRange& subresource_range
);

}  // namespace xray::rendering
