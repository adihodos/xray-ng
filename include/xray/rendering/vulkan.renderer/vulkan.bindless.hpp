#pragma once

#include "xray/xray.hpp"

#include <cassert>
#include <cstdint>
#include <functional>
#include <bitset>

#include <vulkan/vulkan.h>
#include <tl/expected.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"

namespace xray::rendering {

struct VulkanBuffer;

enum class BindlessResourceType : uint8_t
{
    UniformBuffer,
    StorageBuffer,
    Sampler,
    CombinedImageSampler,
};

inline constexpr VkDescriptorType
bindless_resource_to_vk_descriptor_type(const BindlessResourceType restype) noexcept
{
    switch (restype) {
        default:
        case BindlessResourceType::UniformBuffer:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case BindlessResourceType::StorageBuffer:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        case BindlessResourceType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case BindlessResourceType::CombinedImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
    }
}

struct BindlessResourceHandle
{
    union
    {
        struct
        {
            uint32_t res_type : 8;
            uint32_t entry_idx : 16;
        };
        uint32_t handle;
    };

    BindlessResourceHandle(const BindlessResourceType rtype, const uint32_t idx) noexcept
    {
        res_type = static_cast<uint8_t>(rtype);
        assert(idx < 65535);
        entry_idx = static_cast<uint16_t>(idx);
    }
};

inline bool
operator==(const BindlessResourceHandle a, const BindlessResourceHandle b) noexcept
{
    return a.handle == b.handle;
}

inline bool
operator!=(const BindlessResourceHandle a, const BindlessResourceHandle b) noexcept
{
    return !(a == b);
}

class BindlessSystem
{
  public:
    ~BindlessSystem();
    BindlessSystem(const BindlessSystem&) = delete;
    BindlessSystem& operator=(const BindlessSystem&) = delete;
    BindlessSystem(BindlessSystem&&) = default;

    static tl::expected<BindlessSystem, VulkanError> create(VkDevice device);

    VkPipelineLayout pipeline_layout() const noexcept { return _bindless.handle<VkPipelineLayout>(); }
    std::span<const VkDescriptorSetLayout> descriptor_set_layouts() const noexcept { return _set_layouts; }

    BindlessResourceHandle add_image(VulkanImage img);
    BindlessResourceHandle add_uniform_buffer(VulkanBuffer ubo);
    BindlessResourceHandle add_storage_buffer(VulkanBuffer sbo);

  private:
    BindlessSystem(UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> bindless,
                   std::vector<VkDescriptorSetLayout> set_layouts,
                   std::vector<VkDescriptorSet> descriptors);

    struct BindlessImageResource
    {
        VkImage handle;
        VkDeviceMemory memory;
        VulkanTextureInfo info;
    };

    struct BindlessUniformBufferResource
    {
        VkBuffer handle;
        VkDeviceMemory memory;
    };

    struct BindlessStorageBufferResource
    {
        VkBuffer handle;
        VkDeviceMemory memory;
    };

  private:
    UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> _bindless;
    std::vector<VkDescriptorSetLayout> _set_layouts;
    std::vector<VkDescriptorSet> _descriptors;
    std::vector<BindlessImageResource> _image_resources;
    std::vector<BindlessUniformBufferResource> _ubo_resources;
    std::bitset<1024> _flush_tbl_ubo;
    std::vector<BindlessStorageBufferResource> _sbo_resources;
    std::bitset<1024> _flush_tbl_sbo;
};

} // namespace xray::rendering

namespace std {
template<>
struct hash<xray::rendering::BindlessResourceHandle>
{
    size_t operator()(const xray::rendering::BindlessResourceHandle rh) const noexcept
    {
        return hash<uint32_t>{}(rh.handle);
    }
};
}
