#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"

#include <itlib/small_vector.hpp>
#include <Lz/Lz.hpp>

#include "xray/base/variant_visitor.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

xray::rendering::BindlessSystem::BindlessSystem(
    UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> bindless,
    std::vector<VkDescriptorSetLayout> set_layouts,
    std::vector<VkDescriptorSet> descriptors)
    : _bindless{ std::move(bindless) }
    , _set_layouts{ std::move(set_layouts) }
    , _descriptors{ std::move(descriptors) }
{
}

xray::rendering::BindlessSystem::BindlessSystem(xray::rendering::BindlessSystem&& rhs) noexcept
    : _bindless{ std::move(rhs._bindless) }
    , _set_layouts{ std::move(rhs._set_layouts) }
    , _descriptors{ std::move(rhs._descriptors) }
    , _image_resources{ std::move(rhs._image_resources) }
    , _ubo_resources{ std::move(rhs._ubo_resources) }
    , _sbo_resources{ std::move(rhs._sbo_resources) }
    , _writes_ubo{ std::move(rhs._writes_ubo) }
    , _writes_sbo{ std::move(rhs._writes_sbo) }
    , _writes_img{ std::move(rhs._writes_img) }
    , _handle_idx_ubos{ rhs._handle_idx_ubos }
    , _handle_idx_sbos{ rhs._handle_idx_sbos.load() }
    , _free_slot_images{ rhs._free_slot_images.load() }
    , _sampler_table{ std::move(rhs._sampler_table) }
{
}

xray::rendering::BindlessSystem::~BindlessSystem()
{
    VkDevice device{ _bindless._owner };
    free_multiple_resources(
        base::VariantVisitor{
            [device](const xray::rendering::BindlessResourceEntry_Image& img) noexcept {
                vkFreeMemory(device, img.memory, nullptr);
                vkDestroyImage(device, img.handle, nullptr);
                vkDestroyImageView(device, img.image_view, nullptr);
            },
            [device](const std::pair<VkSamplerCreateInfo, VkSampler>& r) noexcept {
                vkDestroySampler(device, r.second, nullptr);
            },
            [device](VkDescriptorSetLayout dsl) noexcept { vkDestroyDescriptorSetLayout(device, dsl, nullptr); },
            [device](const xray::rendering::BindlessSystem::SBOResourceEntry& r) noexcept {
                vkFreeMemory(device, r.sbo.memory, nullptr);
                vkDestroyBuffer(device, r.sbo.handle, nullptr);
            },
            [device](const xray::rendering::BindlessSystem::UBOResourceEntry& r) noexcept {
                vkFreeMemory(device, r.ubo.memory, nullptr);
                vkDestroyBuffer(device, r.ubo.handle, nullptr);
            } },
        _set_layouts,
        _image_resources,
        _sampler_table,
        _sbo_resources,
        _ubo_resources);
}

tl::expected<xray::rendering::BindlessSystem, xray::rendering::VulkanError>
xray::rendering::BindlessSystem::create(VkDevice device, const VkPhysicalDeviceDescriptorIndexingProperties& props)
{
    //
    // descriptor pool
    xrUniqueVkDescriptorPool dpool{
        [device, p = &props]() {
            const VkDescriptorPoolSize pool_sizes[] = {
                { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                  .descriptorCount = std::min(p->maxPerStageDescriptorUpdateAfterBindUniformBuffers, uint32_t{ 16 }) },
                { .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 1024 },
                { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1024 },
                { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1024 },
            };

            const VkDescriptorPoolCreateInfo pool_create_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                .maxSets = 1024,
                .poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes)),
                .pPoolSizes = pool_sizes,
            };

            VkDescriptorPool pool{ nullptr };
            WRAP_VULKAN_FUNC(vkCreateDescriptorPool, device, &pool_create_info, nullptr, &pool);
            return pool;
        }(),
        VkResourceDeleter_VkDescriptorPool{ device },
    };

    if (!dpool) {
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_OUT_OF_POOL_MEMORY);
    }

    XR_LOG_INFO("Bindless descriptor pool created @ {}", static_cast<void*>(raw_ptr(dpool)));

    struct LayoutBindingsByResourceType
    {
        VkDescriptorType res_type;
        uint32_t descriptor_count;
        VkShaderStageFlags stage_flags;
    };

    const LayoutBindingsByResourceType layout_bindigs_by_res[] = {
        LayoutBindingsByResourceType{
            .res_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptor_count = std::min(props.maxPerStageDescriptorUpdateAfterBindUniformBuffers, uint32_t{ 16 }),
            .stage_flags = VK_SHADER_STAGE_ALL,
        },
        LayoutBindingsByResourceType{
            .res_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptor_count = 512,
            .stage_flags = VK_SHADER_STAGE_ALL,
        },
        LayoutBindingsByResourceType{
            .res_type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptor_count = 512,
            .stage_flags = VK_SHADER_STAGE_ALL,
        },
    };

    std::vector<VkDescriptorSetLayout> set_layouts;

    for (const LayoutBindingsByResourceType& layout_template : layout_bindigs_by_res) {
        const VkDescriptorSetLayoutBinding layout_binding{
            .binding = 0,
            .descriptorType = layout_template.res_type,
            .descriptorCount = layout_template.descriptor_count,
            .stageFlags = layout_template.stage_flags,
            .pImmutableSamplers = nullptr,
        };

        const VkDescriptorSetLayoutCreateInfo set_layout_info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .bindingCount = 1,
            .pBindings = &layout_binding,
        };

        VkDescriptorSetLayout set_layout{};
        const VkResult create_res =
            WRAP_VULKAN_FUNC(vkCreateDescriptorSetLayout, device, &set_layout_info, nullptr, &set_layout);
        if (create_res != VK_SUCCESS) {
            return XR_MAKE_VULKAN_ERROR(create_res);
        }

        set_layouts.push_back(set_layout);
    }

    const VkPushConstantRange push_constant_ranges[] = { VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(uint32_t)),
    } };

    const VkPipelineLayoutCreateInfo bindless_pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = static_cast<uint32_t>(set_layouts.size()),
        .pSetLayouts = set_layouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(std::size(push_constant_ranges)),
        .pPushConstantRanges = push_constant_ranges,
    };

    VkPipelineLayout bindless_pipeline_layout{};
    const VkResult bindless_pcreate_result = WRAP_VULKAN_FUNC(
        vkCreatePipelineLayout, device, &bindless_pipeline_layout_create_info, nullptr, &bindless_pipeline_layout);

    if (bindless_pcreate_result != VK_SUCCESS) {
        return XR_MAKE_VULKAN_ERROR(bindless_pcreate_result);
    }

    XR_LOG_INFO("Bindless pipeline layout created @ {}", static_cast<void*>(bindless_pipeline_layout));

    const VkDescriptorSetAllocateInfo set_allocate_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = base::raw_ptr(dpool),
        .descriptorSetCount = static_cast<uint32_t>(set_layouts.size()),
        .pSetLayouts = set_layouts.data(),
    };

    std::vector<VkDescriptorSet> descriptor_sets{ set_layouts.size(), nullptr };
    const VkResult alloc_result =
        WRAP_VULKAN_FUNC(vkAllocateDescriptorSets, device, &set_allocate_info, descriptor_sets.data());

    if (alloc_result != VK_SUCCESS) {
        return XR_MAKE_VULKAN_ERROR(alloc_result);
    }

    return BindlessSystem{
        UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout>{
            device,
            base::unique_pointer_release(dpool),
            bindless_pipeline_layout,
        },
        std::move(set_layouts),
        std::move(descriptor_sets),
    };
}

std::pair<xray::rendering::BindlessResourceHandle_Image, xray::rendering::BindlessResourceEntry_Image>
xray::rendering::BindlessSystem::add_image(xray::rendering::VulkanImage img,
                                           VkSampler smp,
                                           const tl::optional<uint32_t> slot)
{
    const uint32_t handle = [&]() {
        if (slot)
            return *slot;

        return _free_slot_images.fetch_add(1);
    }();

    if (_free_slot_images > _image_resources.size()) {
        _image_resources.resize(_free_slot_images);
    }

    const auto [image, image_memory, image_view] = img.release();
    _image_resources[handle] = BindlessResourceEntry_Image{
        .handle = image,
        .memory = image_memory,
        .image_view = image_view,
        .info = img._info,
    };

    _writes_img.push_back(WriteDescriptorImageInfo{
        .dst_array = handle,
        .img_info =
            VkDescriptorImageInfo{
                .sampler = smp,
                .imageView = image_view,
                .imageLayout = img._info.imageLayout,
            },
    });

    return std::pair{
        BindlessResourceHandle_Image{ handle },
        _image_resources.back(),
    };
}

std::pair<xray::rendering::BindlessResourceHandle_UniformBuffer, xray::rendering::BindlessResourceEntry_UniformBuffer>
xray::rendering::BindlessSystem::add_chunked_uniform_buffer(VulkanBuffer ubo, const uint32_t chunks)
{
    const auto [ubo_handle, ubo_mem] = ubo.buffer.release();

    const uint32_t bindless_idx{ _handle_idx_ubos };
    _handle_idx_ubos += chunks;
    _ubo_resources.emplace_back(
        BindlessResourceEntry_UniformBuffer{ ubo_handle, ubo_mem, ubo.aligned_size }, bindless_idx, chunks);

    const BindlessResourceHandle_UniformBuffer bindless_ubo_handle{
        detail::BindlessResourceHandleHelper{ bindless_idx, chunks }.value
    };

    //
    // write ubo data for descriptor update
    for (uint32_t chunk_idx = 0; chunk_idx < chunks; ++chunk_idx) {
        _writes_ubo.push_back(WriteDescriptorBufferInfo{
            .dst_array = bindless_idx + chunk_idx,
            .buff_info =
                VkDescriptorBufferInfo{
                    .buffer = ubo_handle,
                    .offset = chunk_idx * ubo.aligned_size,
                    .range = ubo.aligned_size,
                },
        });
    }

    return std::pair{ bindless_ubo_handle, _ubo_resources.back().ubo };
}

std::pair<xray::rendering::BindlessResourceHandle_StorageBuffer, xray::rendering::BindlessResourceEntry_StorageBuffer>
xray::rendering::BindlessSystem::add_chunked_storage_buffer(VulkanBuffer ssbo,
                                                            const uint32_t chunks,
                                                            const tl::optional<uint32_t> slot)
{
    const uint32_t handle = [&]() {
        if (slot)
            return *slot;

        return _handle_idx_sbos.fetch_add(1);
    }();

    if (_handle_idx_sbos > _sbo_resources.size()) {
        _sbo_resources.resize(_handle_idx_sbos);
    }

    const auto [ubo_handle, ubo_mem] = ssbo.buffer.release();

    const uint32_t bindless_idx{ handle };
    _sbo_resources.emplace_back(
        BindlessResourceEntry_StorageBuffer{ ubo_handle, ubo_mem, ssbo.aligned_size }, bindless_idx, chunks);

    const BindlessResourceHandle_StorageBuffer bindless_ubo_handle{
        detail::BindlessResourceHandleHelper{ bindless_idx, chunks }.value
    };

    //
    // write ubo data for descriptor update
    for (uint32_t chunk_idx = 0; chunk_idx < chunks; ++chunk_idx) {
        _writes_sbo.push_back(WriteDescriptorBufferInfo{
            .dst_array = bindless_idx + chunk_idx,
            .buff_info =
                VkDescriptorBufferInfo{
                    .buffer = ubo_handle,
                    .offset = chunk_idx * ssbo.aligned_size,
                    .range = ssbo.aligned_size,
                },
        });
    }

    return std::pair{ bindless_ubo_handle, _sbo_resources.back().sbo };
}

void
xray::rendering::BindlessSystem::flush_descriptors(const VulkanRenderer& renderer)
{
    itlib::small_vector<VkWriteDescriptorSet, 4> descriptor_writes;

    lz::chain(_writes_ubo)
        .transformTo(std::back_inserter(descriptor_writes),
                     [dst_set = _descriptors[SET_UBOS_INDEX]](const WriteDescriptorBufferInfo& wds) {
                         return VkWriteDescriptorSet{
                             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                             .pNext = nullptr,
                             .dstSet = dst_set,
                             .dstBinding = 0,
                             .dstArrayElement = wds.dst_array,
                             .descriptorCount = 1,
                             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                             .pImageInfo = nullptr,
                             .pBufferInfo = &wds.buff_info,
                             .pTexelBufferView = nullptr,
                         };
                     });

    lz::chain(_writes_sbo)
        .transformTo(std::back_inserter(descriptor_writes),
                     [dst_set = _descriptors[SET_STORAGE_BUFFER_INDEX]](const WriteDescriptorBufferInfo& wds) {
                         return VkWriteDescriptorSet{
                             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                             .pNext = nullptr,
                             .dstSet = dst_set,
                             .dstBinding = 0,
                             .dstArrayElement = wds.dst_array,
                             .descriptorCount = 1,
                             .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                             .pImageInfo = nullptr,
                             .pBufferInfo = &wds.buff_info,
                             .pTexelBufferView = nullptr,
                         };
                     });

    lz::chain(_writes_img)
        .transformTo(std::back_inserter(descriptor_writes),
                     [dst_set = _descriptors[SET_COMBINED_IMG_SAMPLERS_INDEX]](const WriteDescriptorImageInfo& imi) {
                         return VkWriteDescriptorSet{
                             .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                             .pNext = nullptr,
                             .dstSet = dst_set,
                             .dstBinding = 0,
                             .dstArrayElement = imi.dst_array,
                             .descriptorCount = 1,
                             .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                             .pImageInfo = &imi.img_info,
                             .pBufferInfo = nullptr,
                             .pTexelBufferView = nullptr,
                         };
                     });

    if (!descriptor_writes.empty()) {
        WRAP_VULKAN_FUNC(vkUpdateDescriptorSets,
                         renderer.device(),
                         static_cast<uint32_t>(descriptor_writes.size()),
                         descriptor_writes.data(),
                         0,
                         nullptr);
        _writes_ubo.clear();
        _writes_img.clear();
        _writes_sbo.clear();
    }
}

void
xray::rendering::BindlessSystem::bind_descriptors(const xray::rendering::VulkanRenderer& renderer,
                                                  VkCommandBuffer cmd_buffer) noexcept
{
    vkCmdBindDescriptorSets(cmd_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _bindless.handle<VkPipelineLayout>(),
                            0,
                            static_cast<uint32_t>(_descriptors.size()),
                            _descriptors.data(),
                            0,
                            nullptr);
}

tl::expected<VkSampler, xray::rendering::VulkanError>
xray::rendering::BindlessSystem::get_sampler(const VkSamplerCreateInfo& sampler_info,
                                             const xray::rendering::VulkanRenderer& renderer)
{
    if (const auto table_entry = _sampler_table.find(sampler_info); table_entry != std::end(_sampler_table)) {
        return tl::expected<VkSampler, VulkanError>{ table_entry->second };
    }

    VkSampler new_sampler{};
    const VkResult create_result =
        WRAP_VULKAN_FUNC(vkCreateSampler, renderer.device(), &sampler_info, nullptr, &new_sampler);
    XR_VK_CHECK_RESULT(create_result);

    const auto inserted_entry = _sampler_table.emplace(sampler_info, new_sampler);
    return inserted_entry.first->second;
}

const xray::rendering::BindlessResourceEntry_Image&
xray::rendering::BindlessSystem::image_entry(const xray::rendering::BindlessResourceHandle_Image img) const noexcept
{
    const uint32_t idx = detail::BindlessResourceHandleHelper{ img.value_of() }.array_start;
    assert(idx < _image_resources.size());
    return _image_resources[idx];
}
