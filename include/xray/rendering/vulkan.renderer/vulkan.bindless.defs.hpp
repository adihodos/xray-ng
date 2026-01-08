#pragma once

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

#include <vulkan/vulkan_core.h>
#include <strong_type/strong_type.hpp>
#include <strong_type/bitarithmetic.hpp>
#include <strong_type/convertible_to.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/formattable.hpp>
#include <strong_type/hashable.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.image.defs.hpp"

namespace xray::rendering {

namespace detail {

struct BindlessResourceHandleHelper {
	union {
		struct {
			uint32_t element_count : 16;
			uint32_t array_start   : 16;
		};
		uint32_t value;
	};

	BindlessResourceHandleHelper(const uint32_t array_start, const uint32_t element_count) noexcept {
		this->array_start	= array_start;
		this->element_count = element_count;
	}

	explicit BindlessResourceHandleHelper(const uint32_t packed_val) noexcept { this->value = packed_val; }
};

};	// namespace detail

using BindlessResourceHandle_StorageBuffer =
	strong::type<uint32_t, struct StorageBuffer_tag, strong::equality, strong::formattable, strong::hashable>;

using BindlessResourceHandle_UniformBuffer =
	strong::type<uint32_t, struct UniformBuffer_tag, strong::equality, strong::formattable, strong::hashable>;

using BindlessResourceHandle_Image =
	strong::type<uint32_t, struct Image_tag, strong::equality, strong::formattable, strong::hashable>;

template <typename T>
T bindless_subresource_handle_from_bindless_resource_handle(
	T bindless_resource, const uint32_t subresource_idx
) noexcept
	requires std::is_same_v<T, BindlessResourceHandle_Image> ||
			 std::is_same_v<T, BindlessResourceHandle_UniformBuffer> ||
			 std::is_same_v<T, BindlessResourceHandle_StorageBuffer>
{
	const detail::BindlessResourceHandleHelper main_resource{bindless_resource.value_of()};
	assert(subresource_idx < main_resource.element_count);

	const detail::BindlessResourceHandleHelper subresource{main_resource.array_start + subresource_idx, 1};
	return T{subresource.value};
}

template <typename T>
std::pair<uint16_t, uint16_t> destructure_bindless_resource_handle(T bindless_resource) noexcept
	requires std::is_same_v<T, BindlessResourceHandle_Image> ||
			 std::is_same_v<T, BindlessResourceHandle_UniformBuffer> ||
			 std::is_same_v<T, BindlessResourceHandle_StorageBuffer>
{
	const detail::BindlessResourceHandleHelper r{bindless_resource.value_of()};
	return std::pair{r.array_start, r.element_count};
}

struct VulkanBuffer;

struct BindlessResourceEntry_Image {
	VkImage handle{};
	VkDeviceMemory memory{};
	VkImageView image_view{};
	VulkanTextureInfo info{};
};

struct BindlessResourceEntry_StorageImage {
	VkImage handle{};
	VkDeviceMemory memory{};
	VkImageView image_view{};
	VulkanTextureInfo info{};
};

struct BindlessResourceEntry_UniformBuffer {
	VkBuffer handle{};
	VkDeviceMemory memory{};
	VkDeviceSize aligned_chunk_size{};
};

struct BindlessResourceEntry_StorageBuffer {
	VkBuffer handle{};
	VkDeviceMemory memory{};
	VkDeviceSize aligned_chunk_size{};
};

using BindlessUniformBufferResourceHandleEntryPair =
	std::pair<BindlessResourceHandle_UniformBuffer, BindlessResourceEntry_UniformBuffer>;

using BindlessStorageBufferResourceHandleEntryPair =
	std::pair<BindlessResourceHandle_StorageBuffer, BindlessResourceEntry_StorageBuffer>;

using BindlessImageResourceHandleEntryPair = std::pair<BindlessResourceHandle_Image, BindlessResourceEntry_Image>;

}  // namespace xray::rendering
