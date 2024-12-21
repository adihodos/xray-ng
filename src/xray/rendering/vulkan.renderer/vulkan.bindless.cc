#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"

#include <cassert>
#include <limits>

#include <itlib/small_vector.hpp>
#include <Lz/Lz.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

static inline uint32_t
make_bindless_resource_id(const uint32_t main_idx, const uint32_t chunk_idx) noexcept
{
    assert(main_idx < std::numeric_limits<uint16_t>::max());
    assert(chunk_idx < std::numeric_limits<uint16_t>::max());
    return chunk_idx << 16 | main_idx;
}

xray::rendering::BindlessSystem::BindlessSystem(
    UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> bindless,
    std::vector<VkDescriptorSetLayout> set_layouts,
    std::vector<VkDescriptorSet> descriptors)
    : _bindless{ std::move(bindless) }
    , _set_layouts{ std::move(set_layouts) }
    , _descriptors{ std::move(descriptors) }
{
}

xray::rendering::BindlessSystem::~BindlessSystem()
{
    VkDevice device{ _bindless._owner };

    for (VkDescriptorSetLayout sl : _set_layouts) {
        vkDestroyDescriptorSetLayout(device, sl, nullptr);
    }

    // if (!_descriptors.empty()) {
    //     WRAP_VULKAN_FUNC(vkFreeDescriptorSets,
    //                      device,
    //                      _bindless.handle<VkDescriptorPool>(),
    //                      static_cast<uint32_t>(_descriptors.size()),
    //                      _descriptors.data());
    // }

    for (BindlessResourceEntry_Image& img_res : _image_resources) {
        WRAP_VULKAN_FUNC(vkFreeMemory, device, img_res.memory, nullptr);
        WRAP_VULKAN_FUNC(vkDestroyImage, device, img_res.handle, nullptr);
    }

    lz::chain(_sbo_resources).forEach([device](SBOResourceEntry r) {
        WRAP_VULKAN_FUNC(vkFreeMemory, device, r.sbo.memory, nullptr);
        WRAP_VULKAN_FUNC(vkDestroyBuffer, device, r.sbo.handle, nullptr);
    });

    lz::chain(_ubo_resources).forEach([device](UBOResourceEntry r) {
        WRAP_VULKAN_FUNC(vkFreeMemory, device, r.ubo.memory, nullptr);
        WRAP_VULKAN_FUNC(vkDestroyBuffer, device, r.ubo.handle, nullptr);
    });
}

tl::expected<xray::rendering::BindlessSystem, xray::rendering::VulkanError>
xray::rendering::BindlessSystem::create(VkDevice device)
{
    //
    // descriptor pool
    // TODO: check against the limits 
    xrUniqueVkDescriptorPool dpool{
        [device]() {
            const VkDescriptorPoolSize pool_sizes[] = {
                { .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 16 },
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
            .descriptor_count = 16,
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
xray::rendering::BindlessSystem::add_image(xray::rendering::VulkanImage img, VkImageView view, VkSampler smp)
{
    const auto [image, image_memory] = img.release();
    _image_resources.emplace_back(image, image_memory, img._info);

    const uint32_t handle{ static_cast<uint32_t>(_image_resources.size() - 1) };

    _writes_img.push_back(WriteDescriptorImageInfo{
        .dst_array = handle,
        .img_info = VkDescriptorImageInfo{ .sampler = smp, .imageView = view, .imageLayout = img._info.imageLayout },
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
xray::rendering::BindlessSystem::add_chunked_storage_buffer(VulkanBuffer ssbo, const uint32_t chunks)
{
    const auto [ubo_handle, ubo_mem] = ssbo.buffer.release();

    const uint32_t bindless_idx{ _handle_idx_sbos };
    _handle_idx_sbos += chunks;
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

struct GezaEntry
{
    VkBuffer buffer;
    VkDeviceMemory mem;
    uint32_t start_id;
    uint32_t count;
};
