#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include <itlib/small_vector.hpp>

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

    for (BindlessSystem::BindlessImageResource& img_res : _image_resources) {
        WRAP_VULKAN_FUNC(vkFreeMemory, device, img_res.memory, nullptr);
        WRAP_VULKAN_FUNC(vkDestroyImage, device, img_res.handle, nullptr);
    }
}

tl::expected<xray::rendering::BindlessSystem, xray::rendering::VulkanError>
xray::rendering::BindlessSystem::create(VkDevice device)
{
    //
    // descriptor pool
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
        BindlessResourceType res_type;
        uint32_t descriptor_count;
        VkShaderStageFlags stage_flags;
    };

    const LayoutBindingsByResourceType layout_bindigs_by_res[] = {
        LayoutBindingsByResourceType{
            .res_type = BindlessResourceType::UniformBuffer,
            .descriptor_count = 16,
            .stage_flags = VK_SHADER_STAGE_ALL,
        },
        LayoutBindingsByResourceType{
            .res_type = BindlessResourceType::StorageBuffer,
            .descriptor_count = 512,
            .stage_flags = VK_SHADER_STAGE_ALL,
        },
        LayoutBindingsByResourceType{
            .res_type = BindlessResourceType::CombinedImageSampler,
            .descriptor_count = 512,
            .stage_flags = VK_SHADER_STAGE_ALL,
        },
    };

    std::vector<VkDescriptorSetLayout> set_layouts;

    for (const LayoutBindingsByResourceType& layout_template : layout_bindigs_by_res) {
        const VkDescriptorSetLayoutBinding layout_binding{
            .binding = 0,
            .descriptorType = bindless_resource_to_vk_descriptor_type(layout_template.res_type),
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

xray::rendering::BindlessResourceHandle
xray::rendering::BindlessSystem::add_image(xray::rendering::ManagedImage img)
{
    const auto [image, image_memory] = img.release();
    _image_resources.emplace_back(image, image_memory, img._info);

    return BindlessResourceHandle{
        BindlessResourceType::CombinedImageSampler,
        static_cast<uint32_t>(_image_resources.size() - 1),
    };
}

xray::rendering::BindlessResourceHandle
xray::rendering::BindlessSystem::add_uniform_buffer(ManagedUniqueBuffer ubo)
{
    const auto [ubo_handle, ubo_mem] = ubo.buffer.release();
    _ubo_resources.emplace_back(ubo_handle, ubo_mem);

    return BindlessResourceHandle{ BindlessResourceType::UniformBuffer,
                                   static_cast<uint32_t>(_ubo_resources.size() - 1) };
}

xray::rendering::BindlessResourceHandle
xray::rendering::BindlessSystem::add_storage_buffer(ManagedUniqueBuffer sbo)
{
    const auto [sbo_handle, sbo_mem] = sbo.buffer.release();
    _sbo_resources.emplace_back(sbo_handle, sbo_mem);

    return BindlessResourceHandle{ BindlessResourceType::StorageBuffer,
                                   static_cast<uint32_t>(_sbo_resources.size() - 1) };
}
