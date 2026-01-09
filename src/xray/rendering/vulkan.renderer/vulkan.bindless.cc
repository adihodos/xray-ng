#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"

#include <Lz/map.hpp>
#include <Lz/procs/to.hpp>
#include <Lz/algorithm/find_if.hpp>
#include <Lz/algorithm/accumulate.hpp>

#include "xray/base/variant.helpers.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"

namespace {

constexpr const VkSamplerCreateInfo DEFAULT_SAMPLER_ATTRIBUTES{
	.sType					 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	.pNext					 = nullptr,
	.flags					 = 0,
	.magFilter				 = VK_FILTER_LINEAR,
	.minFilter				 = VK_FILTER_LINEAR,
	.mipmapMode				 = VK_SAMPLER_MIPMAP_MODE_LINEAR,
	.addressModeU			 = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	.addressModeV			 = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	.addressModeW			 = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	.mipLodBias				 = 0.0f,
	.anisotropyEnable		 = false,
	.maxAnisotropy			 = 1.0f,
	.compareEnable			 = false,
	.compareOp				 = VK_COMPARE_OP_NEVER,
	.minLod					 = 0.0f,
	.maxLod					 = VK_LOD_CLAMP_NONE,
	.borderColor			 = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
	.unnormalizedCoordinates = false,
};

}

xray::rendering::BindlessSystem::BindlessSystem(
	UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> bindless,
	std::vector<VkDescriptorSetLayout> set_layouts,
	std::vector<VkDescriptorSet> descriptors,
	ankerl::unordered_dense::map<VulkanResourceType, BindlessResourceTableEntry> resource_table,
	std::unordered_map<VkSamplerCreateInfo, VkSampler> sampler_table,
	const Kind kind
)
	: _bindless{std::move(bindless)},
	  _set_layouts{std::move(set_layouts)},
	  _descriptors{std::move(descriptors)},
	  _resource_table{std::move(resource_table)},
	  _sampler_table{std::move(sampler_table)},
	  _kind{kind} {}

xray::rendering::BindlessSystem::BindlessSystem(xray::rendering::BindlessSystem&& rhs) noexcept
	: _bindless{std::move(rhs._bindless)},
	  _set_layouts{std::move(rhs._set_layouts)},
	  _descriptors{std::move(rhs._descriptors)},
	  _resource_table{std::move(rhs._resource_table)},
	  _sampler_table{std::move(rhs._sampler_table)},
	  _kind{rhs._kind} {}

xray::rendering::BindlessSystem::~BindlessSystem() {
	VkDevice device{_bindless._owner};

	for (const auto& [resource_type, resource_type_entry] : _resource_table) {
		for (const BindlessVulkanResource& resource : resource_type_entry.resources) {
			switch (resource_type) {
				case VulkanResourceType::CombinedImageSampler:
				case VulkanResourceType::SampledImage: {
					if (resource.image.owned) {
						vkFreeMemory(device, resource.image.memory, nullptr);
						vkDestroyImage(device, resource.image.handle, nullptr);
						vkDestroyImageView(device, resource.image.image_view, nullptr);
					}
				} break;

				case VulkanResourceType::StorageImage: {
					if (resource.storage_image.owned) {
						vkFreeMemory(device, resource.storage_image.memory, nullptr);
						vkDestroyImage(device, resource.storage_image.handle, nullptr);
						vkDestroyImageView(device, resource.storage_image.image_view, nullptr);
					}
				} break;

				case VulkanResourceType::UniformBuffer: {
					if (resource.uniform_buffer.ubo.owned) {
						vkFreeMemory(device, resource.uniform_buffer.ubo.memory, nullptr);
						vkDestroyBuffer(device, resource.uniform_buffer.ubo.handle, nullptr);
					}
				} break;

				case VulkanResourceType::StorageBuffer: {
					if (resource.storage_buffer.sbo.owned) {
						vkFreeMemory(device, resource.storage_buffer.sbo.memory, nullptr);
						vkDestroyBuffer(device, resource.storage_buffer.sbo.handle, nullptr);
					}
				} break;

				default: {
					XR_LOG_ERR("free bindless resource of type {} not handled!", std::to_underlying(resource_type));
				} break;
			}
		}
	}

	free_multiple_resources(
		base::VariantVisitor{
			[device](const std::pair<VkSamplerCreateInfo, VkSampler>& r) noexcept {
				vkDestroySampler(device, r.second, nullptr);
			},
			[device](VkDescriptorSetLayout dsl) noexcept { vkDestroyDescriptorSetLayout(device, dsl, nullptr); },
		},
		_set_layouts,
		_sampler_table
	);
}

tl::expected<xray::rendering::BindlessSystem, xray::rendering::VulkanError> xray::rendering::BindlessSystem::create(
	xray::base::MemoryArena& arena,
	const Kind kind,
	VkDevice device,
	const VkPhysicalDeviceDescriptorIndexingProperties& descriptor_props,
	std::span<const LayoutBindingsByResourceType> descriptor_sets_layouts,
	std::span<const VkPushConstantRange> push_consts_ranges
) {
	using namespace xray::base;
	ScratchPadArena scratch_pad = ThreadLocalContext::acquire_scratchpad({&arena});

	containers::vector<VkDescriptorPoolSize> descriptor_pool_sizes{*scratch_pad.arena};
	descriptor_pool_sizes.reserve(descriptor_sets_layouts.size());

	std::vector<VkDescriptorSetLayout> set_layouts;
	set_layouts.reserve(descriptor_sets_layouts.size());

	struct DescriptorCountLimit {
		VkDescriptorType d_type;
		uint32_t d_limit;
	} const descriptor_limits_by_type[] = {
		{
			.d_type	 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.d_limit = descriptor_props.maxPerStageDescriptorUpdateAfterBindUniformBuffers,
		},
	};

	ankerl::unordered_dense::map<VulkanResourceType, BindlessResourceTableEntry> resource_table;
	uint32_t set_index			 = 0;
	uint32_t max_descriptor_sets = 0;
	for (const LayoutBindingsByResourceType& layout_template : descriptor_sets_layouts) {
		//
		// this makes the assumption that the configuration does only contain unique resource types
		resource_table.try_emplace(static_cast<VulkanResourceType>(layout_template.res_type), set_index++);

		uint32_t descriptor_count = layout_template.descriptor_count;
		if (const auto limit_itr = lz::find_if(
				descriptor_limits_by_type,
				[&](const DescriptorCountLimit& limit) { return layout_template.res_type == limit.d_type; }
			);
			limit_itr != std::end(descriptor_limits_by_type)) {
			descriptor_count = std::min(descriptor_count, limit_itr->d_limit);
		}

		descriptor_pool_sizes.emplace_back(layout_template.res_type, descriptor_count);
		max_descriptor_sets += descriptor_count;

		const VkDescriptorSetLayoutBinding layout_binding{
			.binding			= 0,
			.descriptorType		= layout_template.res_type,
			.descriptorCount	= descriptor_count,
			.stageFlags			= layout_template.stage_flags,
			.pImmutableSamplers = nullptr,
		};

		const VkDescriptorBindingFlags binding_flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

		const VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{
			.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.pNext		   = nullptr,
			.bindingCount  = 1,
			.pBindingFlags = &binding_flags,
		};

		const VkDescriptorSetLayoutCreateInfo set_layout_info = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext =
				layout_template.res_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ? nullptr : &binding_flags_create_info,
			.flags		  = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
			.bindingCount = 1,
			.pBindings	  = &layout_binding,
		};

		VkDescriptorSetLayout set_layout{};
		const VkResult create_res =
			WRAP_VULKAN_FUNC(vkCreateDescriptorSetLayout, device, &set_layout_info, nullptr, &set_layout);
		if (create_res != VK_SUCCESS) {
			return XR_MAKE_VULKAN_ERROR(create_res);
		}

		XR_LOG_INFO("DS layout: {} -> {}", layout_template.tag, fmt::ptr(set_layout));
		set_layouts.push_back(set_layout);
	}

	//
	// descriptor pool
	xrUniqueVkDescriptorPool dpool{
		[device, &descriptor_pool_sizes, max_descriptor_sets]() {
			const VkDescriptorPoolCreateInfo pool_create_info = {
				.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				.pNext		   = nullptr,
				.flags		   = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
				.maxSets	   = max_descriptor_sets,
				.poolSizeCount = static_cast<uint32_t>(std::size(descriptor_pool_sizes)),
				.pPoolSizes	   = descriptor_pool_sizes.data(),
			};

			VkDescriptorPool pool{nullptr};
			WRAP_VULKAN_FUNC(vkCreateDescriptorPool, device, &pool_create_info, nullptr, &pool);
			return pool;
		}(),
		VkResourceDeleter_VkDescriptorPool{device},
	};

	if (!dpool) {
		return XR_MAKE_VULKAN_ERROR(VK_ERROR_OUT_OF_POOL_MEMORY);
	}

	XR_LOG_INFO("Bindless descriptor pool created @ {}", static_cast<void*>(raw_ptr(dpool)));

	const VkPipelineLayoutCreateInfo bindless_pipeline_layout_create_info = {
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext					= nullptr,
		.flags					= 0,
		.setLayoutCount			= static_cast<uint32_t>(set_layouts.size()),
		.pSetLayouts			= set_layouts.data(),
		.pushConstantRangeCount = static_cast<uint32_t>(std::size(push_consts_ranges)),
		.pPushConstantRanges	= push_consts_ranges.data(),
	};

	VkPipelineLayout bindless_pipeline_layout{};
	const VkResult bindless_pcreate_result = WRAP_VULKAN_FUNC(
		vkCreatePipelineLayout, device, &bindless_pipeline_layout_create_info, nullptr, &bindless_pipeline_layout
	);

	if (bindless_pcreate_result != VK_SUCCESS) {
		return XR_MAKE_VULKAN_ERROR(bindless_pcreate_result);
	}

	XR_LOG_INFO("Bindless pipeline layout created @ {}", static_cast<void*>(bindless_pipeline_layout));

	//
	// allocate descriptor sets
	const VkDescriptorSetAllocateInfo set_allocate_info{
		.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext				= nullptr,
		.descriptorPool		= base::raw_ptr(dpool),
		.descriptorSetCount = static_cast<uint32_t>(set_layouts.size()),
		.pSetLayouts		= set_layouts.data(),
	};

	std::vector<VkDescriptorSet> descriptor_sets{set_layouts.size(), nullptr};
	const VkResult alloc_result =
		WRAP_VULKAN_FUNC(vkAllocateDescriptorSets, device, &set_allocate_info, descriptor_sets.data());

	if (alloc_result != VK_SUCCESS) {
		return XR_MAKE_VULKAN_ERROR(alloc_result);
	}

	VkSampler new_sampler{};
	const VkResult create_result =
		WRAP_VULKAN_FUNC(vkCreateSampler, device, &DEFAULT_SAMPLER_ATTRIBUTES, nullptr, &new_sampler);
	XR_VK_CHECK_RESULT(create_result);

	return BindlessSystem{
		UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout>{
			device,
			base::unique_pointer_release(dpool),
			bindless_pipeline_layout,
		},
		std::move(set_layouts),
		std::move(descriptor_sets),
		std::move(resource_table),
		std::unordered_map<VkSamplerCreateInfo, VkSampler>{{DEFAULT_SAMPLER_ATTRIBUTES, new_sampler}},
		kind,
	};
}

xray::rendering::BindlessStorageImageResourceHandleEntryPair xray::rendering::BindlessSystem::add_storage_image(
	BindlessResourceEntry_StorageImage img_entry, tl::optional<uint32_t> slot
) {
	auto itr = _resource_table.find(VulkanResourceType::StorageImage);
	if (itr == std::end(_resource_table)) {
		XR_LOG_ERR("Trying to add storage image to this bindless layout but it has not support for it");
		return std::pair{
			BindlessResourceHandle_StorageImage{
				0u,
			},
			BindlessResourceEntry_StorageImage{},
		};
	}

	BindlessResourceTableEntry* tbl_entry = &itr->second;

	const uint32_t handle = [&]() {
		if (slot) return *slot;

		return tbl_entry->handle_idx.fetch_add(1);
	}();

	XR_LOG_INFO("[[bindles]] - img {:#08x} -> {}", (uintptr_t)img_entry.handle, handle);

	if (tbl_entry->handle_idx > tbl_entry->resources.size()) {
		tbl_entry->resources.resize(
			tbl_entry->handle_idx,
			BindlessVulkanResource{
				.storage_image = {},
			}
		);
	}

	tbl_entry->resources[handle] = BindlessVulkanResource{
		.storage_image = img_entry,
	};

	tbl_entry->writes.push_back(BindlessResourceDescriptorWrite{
		.image =
			WriteDescriptorImageInfo{
				.dst_array = handle,
				.img_info =
					VkDescriptorImageInfo{
						.sampler	 = nullptr,
						.imageView	 = img_entry.image_view,
						.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
					},
			}
	});

	const BindlessResourceHandle_StorageImage bindless_handle{
		detail::BindlessResourceHandleHelper{handle, 1}.value,
	};

	return BindlessStorageImageResourceHandleEntryPair{
		bindless_handle,
		tbl_entry->resources[handle].storage_image,
	};
}

std::pair<xray::rendering::BindlessResourceHandle_Image, xray::rendering::BindlessResourceEntry_Image>
xray::rendering::BindlessSystem::add_image(
	BindlessResourceEntry_Image img_entry, VkSampler smp, tl::optional<uint32_t> slot
) {
	auto itr = _resource_table.find(VulkanResourceType::CombinedImageSampler);
	if (itr == std::end(_resource_table)) {
		XR_LOG_ERR("Trying to add sampled image to this bindless layout but it has not support for it");
		return std::pair{
			BindlessResourceHandle_Image{
				0u,
			},
			BindlessResourceEntry_Image{},
		};
	}

	BindlessResourceTableEntry* tbl_entry = &itr->second;

	const uint32_t handle = [&]() {
		if (slot) return *slot;

		return tbl_entry->handle_idx.fetch_add(1);
	}();

	XR_LOG_INFO("[[bindles]] - owned {}, img {:#08x} -> {}", img_entry.owned, (uintptr_t)img_entry.handle, handle);

	if (tbl_entry->handle_idx > tbl_entry->resources.size()) {
		tbl_entry->resources.resize(
			tbl_entry->handle_idx,
			BindlessVulkanResource{
				.image = {},
			}
		);
	}

	tbl_entry->resources[handle] = BindlessVulkanResource{
		.image = img_entry,
	};

	if (smp == nullptr) {
		auto def_sampler_entry = this->_sampler_table.find(DEFAULT_SAMPLER_ATTRIBUTES);
		assert(def_sampler_entry != std::cend(_sampler_table));
		smp = def_sampler_entry->second;
	}

	tbl_entry->writes.push_back(BindlessResourceDescriptorWrite{
		.image =
			WriteDescriptorImageInfo{
				.dst_array = handle,
				.img_info =
					VkDescriptorImageInfo{
						.sampler	 = smp,
						.imageView	 = img_entry.image_view,
						.imageLayout = img_entry.info.imageLayout,
					},
			}
	});

	const BindlessResourceHandle_Image bindless_handle{
		detail::BindlessResourceHandleHelper{handle, 1}.value,
	};

	return std::pair{
		bindless_handle,
		tbl_entry->resources[handle].image,
	};
}

std::pair<xray::rendering::BindlessResourceHandle_UniformBuffer, xray::rendering::BindlessResourceEntry_UniformBuffer>
xray::rendering::BindlessSystem::add_chunked_uniform_buffer(VulkanBuffer ubo, const uint32_t chunks) {
	auto itr = _resource_table.find(VulkanResourceType::UniformBuffer);
	if (itr == std::end(_resource_table)) {
		XR_LOG_ERR("Trying to add uniform buffer resource to this bindless layout but it has not support for it");
		return std::pair{
			BindlessResourceHandle_UniformBuffer{0u},
			BindlessResourceEntry_UniformBuffer{},
		};
	}

	BindlessResourceTableEntry* tbl_entry = &itr->second;

	const auto [ubo_handle, ubo_mem] = ubo.buffer.release();

	const uint32_t bindless_idx{tbl_entry->handle_idx.fetch_add(chunks)};
	tbl_entry->resources.emplace_back(BindlessVulkanResource{
		.uniform_buffer =
			UBOResourceEntry{
				BindlessResourceEntry_UniformBuffer{ubo_handle, ubo_mem, ubo.aligned_size},
				bindless_idx,
				chunks,
			}
	});

	const BindlessResourceHandle_UniformBuffer bindless_ubo_handle{
		detail::BindlessResourceHandleHelper{bindless_idx, chunks}.value
	};

	//
	// write ubo data for descriptor update
	for (uint32_t chunk_idx = 0; chunk_idx < chunks; ++chunk_idx) {
		tbl_entry->writes.push_back(BindlessResourceDescriptorWrite{
			.buffer =
				WriteDescriptorBufferInfo{
					.dst_array = bindless_idx + chunk_idx,
					.buff_info =
						VkDescriptorBufferInfo{
							.buffer = ubo_handle,
							.offset = chunk_idx * ubo.aligned_size,
							.range	= ubo.aligned_size,
						},
				},
		});
	}

	return std::pair{bindless_ubo_handle, tbl_entry->resources.back().uniform_buffer.ubo};
}

std::pair<xray::rendering::BindlessResourceHandle_StorageBuffer, xray::rendering::BindlessResourceEntry_StorageBuffer>
xray::rendering::BindlessSystem::add_chunked_storage_buffer(
	VulkanBuffer ssbo, const uint32_t chunks, const tl::optional<uint32_t> slot
) {
	auto itr = _resource_table.find(VulkanResourceType::StorageBuffer);
	if (itr == std::end(_resource_table)) {
		XR_LOG_ERR("Trying to add uniform buffer resource to this bindless layout but it has not support for it");
		return std::pair{
			BindlessResourceHandle_StorageBuffer{0u},
			BindlessResourceEntry_StorageBuffer{},
		};
	}

	BindlessResourceTableEntry* tbl_entry = &itr->second;

	const uint32_t handle = [&]() {
		if (slot) return *slot;

		return tbl_entry->handle_idx.fetch_add(chunks);
	}();

	if (handle >= tbl_entry->resources.size()) {
		tbl_entry->resources.resize(handle + 1, BindlessVulkanResource{.storage_buffer = {}});
	}

	const auto [ubo_handle, ubo_mem] = ssbo.buffer.release();

	tbl_entry->resources[handle] = BindlessVulkanResource{
		.storage_buffer =
			SBOResourceEntry{
				BindlessResourceEntry_StorageBuffer{ubo_handle, ubo_mem, ssbo.aligned_size},
				handle,
				chunks,
			}
	};

	const BindlessResourceHandle_StorageBuffer bindless_ubo_handle{
		detail::BindlessResourceHandleHelper{handle, chunks}.value,
	};

	//
	// write ubo data for descriptor update
	for (uint32_t chunk_idx = 0; chunk_idx < chunks; ++chunk_idx) {
		tbl_entry->writes.push_back(BindlessResourceDescriptorWrite{
			.buffer =
				WriteDescriptorBufferInfo{
					.dst_array = handle + chunk_idx,
					.buff_info =
						VkDescriptorBufferInfo{
							.buffer = ubo_handle,
							.offset = chunk_idx * ssbo.aligned_size,
							.range	= ssbo.aligned_size,
						},
				}
		});
	}

	return std::pair{bindless_ubo_handle, tbl_entry->resources[handle].storage_buffer.sbo};
}

xray::rendering::BindlessImageResourceHandleEntryPair xray::rendering::BindlessSystem::add_image(
	VulkanImage&& img, VkSampler smp, tl::optional<uint32_t> slot
) {
	const auto [image, image_memory, image_view] = img.release();
	return add_image(
		BindlessResourceEntry_Image{
			.handle		= image,
			.memory		= image_memory,
			.image_view = image_view,
			.info		= img._info,
			.owned		= true,
		},
		smp,
		slot
	);
}

xray::rendering::BindlessImageResourceHandleEntryPair xray::rendering::BindlessSystem::add_image(
	const VulkanImage& img, VkSampler smp, tl::optional<uint32_t> slot
) {
	return add_image(
		BindlessResourceEntry_Image{
			.handle		= img.image(),
			.memory		= img.memory(),
			.image_view = img.view(),
			.info		= img._info,
			.owned		= false,
		},
		smp,
		slot
	);
}

xray::rendering::BindlessStorageImageResourceHandleEntryPair xray::rendering::BindlessSystem::add_storage_image(
	VulkanImage&& img, tl::optional<uint32_t> slot
) {
	const auto [image, image_memory, image_view] = img.release();
	return add_storage_image(
		BindlessResourceEntry_StorageImage{
			.handle		= image,
			.memory		= image_memory,
			.image_view = image_view,
			.info		= img._info,
			.owned		= true,
		},
		slot
	);
}

xray::rendering::BindlessStorageImageResourceHandleEntryPair xray::rendering::BindlessSystem::add_storage_image(
	const VulkanImage& img, tl::optional<uint32_t> slot
) {
	return add_storage_image(
		BindlessResourceEntry_StorageImage{
			.handle		= img.image(),
			.memory		= img.memory(),
			.image_view = img.view(),
			.info		= img._info,
			.owned		= false,
		},
		slot
	);
}

void xray::rendering::BindlessSystem::flush_descriptors(const VulkanRenderer& renderer) {
	size_t descriptor_writes_count = 0;
	for (const auto& [resource_type, resource_entry] : _resource_table) {
		descriptor_writes_count += resource_entry.writes.size();
	}

	if (descriptor_writes_count == 0) {
		return;
	}

	using namespace xray::base;
	ScratchPadArena scratch_pad = ThreadLocalContext::acquire_scratchpad({});
	containers::vector<VkWriteDescriptorSet> descriptor_writes{*scratch_pad.arena};
	descriptor_writes.reserve(descriptor_writes_count);

	for (const auto& [resource_type, resource_entry] : _resource_table) {
		switch (resource_type) {
			case VulkanResourceType::CombinedImageSampler:
			case VulkanResourceType::SampledImage:
			case VulkanResourceType::StorageImage: {
				for (const BindlessResourceDescriptorWrite& d_write : resource_entry.writes) {
					descriptor_writes.push_back(VkWriteDescriptorSet{
						.sType			  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext			  = nullptr,
						.dstSet			  = _descriptors[resource_entry.set_index],
						.dstBinding		  = 0,
						.dstArrayElement  = d_write.image.dst_array,
						.descriptorCount  = 1,
						.descriptorType	  = static_cast<VkDescriptorType>(resource_type),
						.pImageInfo		  = &d_write.image.img_info,
						.pBufferInfo	  = nullptr,
						.pTexelBufferView = nullptr,
					});
				}
			} break;

			case VulkanResourceType::UniformBuffer:
			case VulkanResourceType::StorageBuffer: {
				for (const BindlessResourceDescriptorWrite& d_write : resource_entry.writes) {
					descriptor_writes.push_back(VkWriteDescriptorSet{
						.sType			  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext			  = nullptr,
						.dstSet			  = _descriptors[resource_entry.set_index],
						.dstBinding		  = 0,
						.dstArrayElement  = d_write.buffer.dst_array,
						.descriptorCount  = 1,
						.descriptorType	  = static_cast<VkDescriptorType>(resource_type),
						.pImageInfo		  = nullptr,
						.pBufferInfo	  = &d_write.buffer.buff_info,
						.pTexelBufferView = nullptr,
					});
				}
			} break;

			default: {
				XR_LOG_ERR("Resource type {} not handled for descriptor writes", std::to_underlying(resource_type));
			} break;
		}
	}

	WRAP_VULKAN_FUNC(
		vkUpdateDescriptorSets,
		renderer.device(),
		static_cast<uint32_t>(descriptor_writes.size()),
		descriptor_writes.data(),
		0,
		nullptr
	);

	for (auto& [resource_type, resource_entry] : _resource_table) {
		resource_entry.writes.clear();
	}
}

void xray::rendering::BindlessSystem::bind_descriptors(
	const xray::rendering::VulkanRenderer& renderer, VkCommandBuffer cmd_buffer
) noexcept {
	vkCmdBindDescriptorSets(
		cmd_buffer,
		_kind == Kind::Graphics ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
		_bindless.handle<VkPipelineLayout>(),
		0,
		static_cast<uint32_t>(_descriptors.size()),
		_descriptors.data(),
		0,
		nullptr
	);
}

tl::expected<VkSampler, xray::rendering::VulkanError> xray::rendering::BindlessSystem::get_sampler(
	const VkSamplerCreateInfo& sampler_info, const xray::rendering::VulkanRenderer& renderer
) {
	if (const auto table_entry = _sampler_table.find(sampler_info); table_entry != std::end(_sampler_table)) {
		return tl::expected<VkSampler, VulkanError>{table_entry->second};
	}

	VkSampler new_sampler{};
	const VkResult create_result =
		WRAP_VULKAN_FUNC(vkCreateSampler, renderer.device(), &sampler_info, nullptr, &new_sampler);
	XR_VK_CHECK_RESULT(create_result);

	const auto inserted_entry = _sampler_table.emplace(sampler_info, new_sampler);
	return inserted_entry.first->second;
}

tl::expected<VkSampler, xray::rendering::VulkanError> xray::rendering::BindlessSystem::default_sampler(
	const VulkanRenderer& renderer
) {
	return get_sampler(DEFAULT_SAMPLER_ATTRIBUTES, renderer);
}

const xray::rendering::BindlessResourceEntry_Image* xray::rendering::BindlessSystem::image_entry(
	const xray::rendering::BindlessResourceHandle_Image img
) const noexcept {
	const uint32_t idx = detail::BindlessResourceHandleHelper{img.value_of()}.array_start;

	auto itr = _resource_table.find(VulkanResourceType::SampledImage);
	if (itr != std::end(_resource_table)) {
		if (idx < itr->second.resources.size()) {
			return &itr->second.resources[idx].image;
		}
		XR_LOG_ERR("sampled image {} not found!", idx);
		return nullptr;
	}

	XR_LOG_ERR("Trying to get an image entry {} but this bindless layout does not have sampled images.", idx);
	return nullptr;
}

//
// TODO: this duplicates the code above, should extract it into a common function
const xray::rendering::BindlessResourceEntry_Image* xray::rendering::BindlessSystem::storage_image_entry(
	const xray::rendering::BindlessResourceHandle_Image img
) const noexcept {
	const uint32_t idx = detail::BindlessResourceHandleHelper{img.value_of()}.array_start;

	auto itr = _resource_table.find(VulkanResourceType::StorageImage);
	if (itr != std::end(_resource_table)) {
		if (idx < itr->second.resources.size()) {
			return &itr->second.resources[idx].image;
		}
		XR_LOG_ERR("sampled image {} not found!", idx);
		return nullptr;
	}

	XR_LOG_ERR("Trying to get storage image entry {} but this bindless layout does not have storage images.", idx);
	return nullptr;
}

uint32_t xray::rendering::BindlessSystem::reserve_resource_slots(
	const uint32_t reserve_count, const VulkanResourceType resource_type
) noexcept {
	auto itr = _resource_table.find(resource_type);
	if (itr == std::end(_resource_table)) {
		XR_LOG_ERR(
			"Trying to reserve {} slots for resource type {} but this layout does not have any",
			reserve_count,
			std::to_underlying(resource_type)
		);
		return 0xFFFFFFFFu;
	}

	return itr->second.handle_idx.fetch_add(reserve_count);
}
