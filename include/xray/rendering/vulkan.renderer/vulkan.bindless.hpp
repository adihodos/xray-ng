#pragma once

#include "xray/xray.hpp"

#include <atomic>
#include <cstdint>
#include <cassert>
#include <utility>
#include <functional>
#include <type_traits>
#include <vector>
#include <unordered_map>
#include <span>

#include <vulkan/vulkan_core.h>
#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <ankerl/unordered_dense.h>

#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.defs.hpp"

namespace std {
template <>
struct hash<VkSamplerCreateInfo> {
	template <typename... M>
	static size_t hash_combine(M&&... m) noexcept {
		size_t result{};
		(..., [&result](auto&& value) mutable {
			const size_t x = hash<decay_t<decltype(value)>>{}(value);
			result ^= x + 0x9e3779b9 + (result << 6) + (result >> 2);
		}(std::forward<M>(m)));

		return result;
	}

	size_t operator()(const VkSamplerCreateInfo& ci) const noexcept {
		return hash<VkSamplerCreateInfo>::hash_combine(
			ci.pNext,
			ci.magFilter,
			ci.minFilter,
			ci.mipmapMode,
			ci.addressModeU,
			ci.addressModeV,
			ci.addressModeW,
			ci.anisotropyEnable,
			ci.borderColor
		);
	}
};

template <>
struct equal_to<VkSamplerCreateInfo> {
	constexpr bool operator()(const VkSamplerCreateInfo& lhs, const VkSamplerCreateInfo& rhs) const {
		return lhs.pNext == rhs.pNext && lhs.magFilter == rhs.magFilter && lhs.minFilter == rhs.minFilter &&
			   lhs.mipmapMode == rhs.mipmapMode && lhs.addressModeU == rhs.addressModeU &&
			   lhs.addressModeV == rhs.addressModeV && lhs.addressModeW == rhs.addressModeW &&
			   lhs.anisotropyEnable == rhs.anisotropyEnable && lhs.borderColor == rhs.borderColor;
	}
};

}  // namespace std

namespace xray::base {
struct MemoryArena;
}

namespace xray::rendering {

class VulkanImage;
class VulkanRenderer;

struct LayoutBindingsByResourceType {
	VkDescriptorType res_type;
	uint32_t descriptor_count;
	VkShaderStageFlags stage_flags{VK_SHADER_STAGE_ALL};
	const char* tag{nullptr};
};

class BindlessSystem {
public:
	enum class Kind : uint8_t {
		Graphics,
		Compute,
	};

	struct SBOResourceEntry {
		BindlessResourceEntry_StorageBuffer sbo;
		uint32_t idx{};
		uint32_t cnt{};
	};

	struct UBOResourceEntry {
		BindlessResourceEntry_UniformBuffer ubo;
		uint32_t idx{};
		uint32_t cnt{};
	};

	~BindlessSystem();
	BindlessSystem(const BindlessSystem&)			 = delete;
	BindlessSystem& operator=(const BindlessSystem&) = delete;
	BindlessSystem(BindlessSystem&&) noexcept;

	static tl::expected<BindlessSystem, VulkanError> create(
		xray::base::MemoryArena& arena,
		const Kind kind,
		VkDevice device,
		const VkPhysicalDeviceDescriptorIndexingProperties& props,
		std::span<const LayoutBindingsByResourceType> descriptor_sets_layouts,
		std::span<const VkPushConstantRange> push_consts_ranges
	);

	///
	/// @group properties
	VkPipelineLayout pipeline_layout() const noexcept { return _bindless.handle<VkPipelineLayout>(); }
	std::span<const VkDescriptorSetLayout> descriptor_set_layouts() const noexcept { return _set_layouts; }
	std::span<const VkDescriptorSet> descriptor_sets() const noexcept { return _descriptors; }

	std::pair<BindlessResourceHandle_Image, BindlessResourceEntry_Image> add_image(
		VulkanImage img, VkSampler smp, tl::optional<uint32_t> slot
	);

	std::pair<BindlessResourceHandle_UniformBuffer, BindlessResourceEntry_UniformBuffer> add_uniform_buffer(
		VulkanBuffer ubo
	) {
		return add_chunked_uniform_buffer(std::move(ubo), 1);
	}

	std::pair<BindlessResourceHandle_StorageBuffer, BindlessResourceEntry_StorageBuffer> add_storage_buffer(
		VulkanBuffer sbo, tl::optional<uint32_t> slot
	) {
		return add_chunked_storage_buffer(std::move(sbo), 1, slot);
	}

	std::pair<BindlessResourceHandle_UniformBuffer, BindlessResourceEntry_UniformBuffer> add_chunked_uniform_buffer(
		VulkanBuffer ubo, const uint32_t chunks
	);

	std::pair<BindlessResourceHandle_StorageBuffer, BindlessResourceEntry_StorageBuffer> add_chunked_storage_buffer(
		VulkanBuffer ssbo, const uint32_t chunks, const tl::optional<uint32_t> slot
	);

	void flush_descriptors(const VulkanRenderer& renderer);
	void bind_descriptors(const VulkanRenderer& renderer, VkCommandBuffer cmd_buffer) noexcept;
	tl::expected<VkSampler, VulkanError> get_sampler(
		const VkSamplerCreateInfo& sampler_info, const VulkanRenderer& renderer
	);
	tl::expected<VkSampler, VulkanError> default_sampler(const VulkanRenderer& renderer);

	uint32_t reserve_image_slots(const uint32_t num_images) noexcept {
		return reserve_resource_slots(num_images, VulkanResourceType::SampledImage);
	}
	uint32_t reserve_sbo_slots(const uint32_t slots) noexcept {
		return reserve_resource_slots(slots, VulkanResourceType::StorageBuffer);
	}
	uint32_t reserve_storage_image_slots(const uint32_t slots) noexcept {
		return reserve_resource_slots(slots, VulkanResourceType::StorageImage);
	}

	const BindlessResourceEntry_Image* image_entry(const BindlessResourceHandle_Image img) const noexcept;
	const BindlessResourceEntry_Image* storage_image_entry(const BindlessResourceHandle_Image img) const noexcept;

private:
	//
	// add more if needed
	enum class VulkanResourceType : int32_t {
		Sampler				 = VK_DESCRIPTOR_TYPE_SAMPLER,
		CombinedImageSampler = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		SampledImage		 = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		StorageImage		 = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		UniformBuffer		 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		StorageBuffer		 = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	};

	union BindlessVulkanResource {
		BindlessResourceEntry_Image image;
		SBOResourceEntry storage_buffer;
		UBOResourceEntry uniform_buffer;
		BindlessResourceEntry_StorageImage storage_image;
	};

	struct WriteDescriptorBufferInfo {
		uint32_t dst_array;
		VkDescriptorBufferInfo buff_info;
	};

	struct WriteDescriptorImageInfo {
		uint32_t dst_array;
		VkDescriptorImageInfo img_info;
	};

	union BindlessResourceDescriptorWrite {
		WriteDescriptorImageInfo image;
		WriteDescriptorBufferInfo buffer;
	};

	struct BindlessResourceTableEntry {
		uint32_t set_index{};
		std::atomic_uint32_t handle_idx{};
		std::vector<BindlessVulkanResource> resources{};
		std::vector<BindlessResourceDescriptorWrite> writes{};

		BindlessResourceTableEntry() = default;
		BindlessResourceTableEntry(const uint32_t set_index_) : set_index{set_index_} {}

		BindlessResourceTableEntry(BindlessResourceTableEntry&& rhs)
			: set_index{rhs.set_index},
			  handle_idx{rhs.handle_idx.load()},
			  resources{std::move(rhs.resources)},
			  writes{std::move(rhs.writes)} {}
	};

private:
	uint32_t reserve_resource_slots(const uint32_t reserve_count, const VulkanResourceType resource_type) noexcept;

	BindlessSystem(
		UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> bindless,
		std::vector<VkDescriptorSetLayout> set_layouts,
		std::vector<VkDescriptorSet> descriptors,
		ankerl::unordered_dense::map<VulkanResourceType, BindlessResourceTableEntry> resource_table,
		std::unordered_map<VkSamplerCreateInfo, VkSampler> sampler_table,
		const Kind kind
	);

	UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> _bindless;
	std::vector<VkDescriptorSetLayout> _set_layouts;
	std::vector<VkDescriptorSet> _descriptors;
	ankerl::unordered_dense::map<VulkanResourceType, BindlessResourceTableEntry> _resource_table;
	std::unordered_map<VkSamplerCreateInfo, VkSampler> _sampler_table;
	Kind _kind;
};

}  // namespace xray::rendering
