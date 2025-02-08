#pragma once

#include "xray/xray.hpp"

#include <atomic>
#include <cstdint>
#include <cassert>
#include <utility>
#include <functional>
#include <type_traits>
#include <unordered_map>

#include <vulkan/vulkan_core.h>
#include <tl/expected.hpp>
#include <tl/optional.hpp>

#include <strong_type/strong_type.hpp>
#include <strong_type/bitarithmetic.hpp>
#include <strong_type/convertible_to.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/formattable.hpp>
#include <strong_type/hashable.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"

namespace std {
template<>
struct hash<VkSamplerCreateInfo>
{
    template<typename... M>
    static size_t hash_combine(M&&... m) noexcept
    {
        size_t result{};
        (..., [&result](auto&& value) mutable {
            const size_t x = hash<decay_t<decltype(value)>>{}(value);
            result ^= x + 0x9e3779b9 + (result << 6) + (result >> 2);
        }(std::forward<M>(m)));

        return result;
    }

    size_t operator()(const VkSamplerCreateInfo& ci) const noexcept
    {
        return hash<VkSamplerCreateInfo>::hash_combine(ci.pNext,
                                                       ci.magFilter,
                                                       ci.minFilter,
                                                       ci.mipmapMode,
                                                       ci.addressModeU,
                                                       ci.addressModeV,
                                                       ci.addressModeW,
                                                       ci.anisotropyEnable,
                                                       ci.borderColor);
    }
};

template<>
struct equal_to<VkSamplerCreateInfo>
{
    constexpr bool operator()(const VkSamplerCreateInfo& lhs, const VkSamplerCreateInfo& rhs) const
    {
        return lhs.pNext == rhs.pNext && lhs.magFilter == rhs.magFilter && lhs.minFilter == rhs.minFilter &&
               lhs.mipmapMode == rhs.mipmapMode && lhs.addressModeU == rhs.addressModeU &&
               lhs.addressModeV == rhs.addressModeV && lhs.addressModeW == rhs.addressModeW &&
               lhs.anisotropyEnable == rhs.anisotropyEnable && lhs.borderColor == rhs.borderColor;
    }
};

}

namespace xray::rendering {

namespace detail {

struct BindlessResourceHandleHelper
{
    union
    {
        struct
        {
            uint32_t element_count : 16;
            uint32_t array_start : 16;
        };
        uint32_t value;
    };

    BindlessResourceHandleHelper(const uint32_t array_start, const uint32_t element_count) noexcept
    {
        this->array_start = array_start;
        this->element_count = element_count;
    }

    explicit BindlessResourceHandleHelper(const uint32_t packed_val) noexcept { this->value = packed_val; }
};

};

using BindlessResourceHandle_StorageBuffer =
    strong::type<uint32_t, struct StorageBuffer_tag, strong::equality, strong::formattable, strong::hashable>;

using BindlessResourceHandle_UniformBuffer =
    strong::type<uint32_t, struct UniformBuffer_tag, strong::equality, strong::formattable, strong::hashable>;

using BindlessResourceHandle_Image =
    strong::type<uint32_t, struct Image_tag, strong::equality, strong::formattable, strong::hashable>;

template<typename T>
T
bindless_subresource_handle_from_bindless_resource_handle(T bindless_resource, const uint32_t subresource_idx) noexcept
    requires std::is_same_v<T, BindlessResourceHandle_Image> ||
             std::is_same_v<T, BindlessResourceHandle_UniformBuffer> ||
             std::is_same_v<T, BindlessResourceHandle_StorageBuffer>
{
    const detail::BindlessResourceHandleHelper main_resource{ bindless_resource.value_of() };
    assert(subresource_idx < main_resource.element_count);

    const detail::BindlessResourceHandleHelper subresource{ main_resource.array_start + subresource_idx, 1 };
    return T{ subresource.value };
}

template<typename T>
std::pair<uint16_t, uint16_t>
destructure_bindless_resource_handle(T bindless_resource) noexcept
    requires std::is_same_v<T, BindlessResourceHandle_Image> ||
             std::is_same_v<T, BindlessResourceHandle_UniformBuffer> ||
             std::is_same_v<T, BindlessResourceHandle_StorageBuffer>
{
    const detail::BindlessResourceHandleHelper r{ bindless_resource.value_of() };
    return std::pair{ r.array_start, r.element_count };
}

struct VulkanBuffer;

struct BindlessResourceEntry_Image
{
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView image_view;
    VulkanTextureInfo info;
};

struct BindlessResourceEntry_UniformBuffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
    VkDeviceSize aligned_chunk_size;
};

struct BindlessResourceEntry_StorageBuffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
    VkDeviceSize aligned_chunk_size;
};

using BindlessUniformBufferResourceHandleEntryPair =
    std::pair<BindlessResourceHandle_UniformBuffer, BindlessResourceEntry_UniformBuffer>;

using BindlessStorageBufferResourceHandleEntryPair =
    std::pair<BindlessResourceHandle_StorageBuffer, BindlessResourceEntry_StorageBuffer>;

using BindlessImageResourceHandleEntryPair = std::pair<BindlessResourceHandle_Image, BindlessResourceEntry_Image>;

class VulkanRenderer;

class BindlessSystem
{
  public:
    struct SBOResourceEntry
    {
        BindlessResourceEntry_StorageBuffer sbo;
        uint32_t idx{};
        uint32_t cnt{};
    };

    struct UBOResourceEntry
    {
        BindlessResourceEntry_UniformBuffer ubo;
        uint32_t idx{};
        uint32_t cnt{};
    };

    ~BindlessSystem();
    BindlessSystem(const BindlessSystem&) = delete;
    BindlessSystem& operator=(const BindlessSystem&) = delete;
    BindlessSystem(BindlessSystem&&) noexcept;

    static tl::expected<BindlessSystem, VulkanError> create(VkDevice device,
                                                            const VkPhysicalDeviceDescriptorIndexingProperties& props);

    VkPipelineLayout pipeline_layout() const noexcept { return _bindless.handle<VkPipelineLayout>(); }
    std::span<const VkDescriptorSetLayout> descriptor_set_layouts() const noexcept { return _set_layouts; }
    std::span<const VkDescriptorSet> descriptor_sets() const noexcept { return _descriptors; }

    std::pair<BindlessResourceHandle_Image, BindlessResourceEntry_Image> add_image(VulkanImage img,
                                                                                   VkSampler smp,
                                                                                   tl::optional<uint32_t> slot);

    std::pair<BindlessResourceHandle_UniformBuffer, BindlessResourceEntry_UniformBuffer> add_uniform_buffer(
        VulkanBuffer ubo)
    {
        return add_chunked_uniform_buffer(std::move(ubo), 1);
    }

    std::pair<BindlessResourceHandle_StorageBuffer, BindlessResourceEntry_StorageBuffer> add_storage_buffer(
        VulkanBuffer sbo,
        tl::optional<uint32_t> slot)
    {
        return add_chunked_storage_buffer(std::move(sbo), 1, slot);
    }

    std::pair<BindlessResourceHandle_UniformBuffer, BindlessResourceEntry_UniformBuffer> add_chunked_uniform_buffer(
        VulkanBuffer ubo,
        const uint32_t chunks);

    std::pair<BindlessResourceHandle_StorageBuffer, BindlessResourceEntry_StorageBuffer>
    add_chunked_storage_buffer(VulkanBuffer ssbo, const uint32_t chunks, const tl::optional<uint32_t> slot);

    void flush_descriptors(const VulkanRenderer& renderer);
    void bind_descriptors(const VulkanRenderer& renderer, VkCommandBuffer cmd_buffer) noexcept;
    tl::expected<VkSampler, VulkanError> get_sampler(const VkSamplerCreateInfo& sampler_info,
                                                     const VulkanRenderer& renderer);

    uint32_t reserve_image_slots(const uint32_t num_images) noexcept { return _free_slot_images.fetch_add(num_images); }
    uint32_t reserve_sbo_slots(const uint32_t slots) noexcept { return _handle_idx_sbos.fetch_add(slots); }

    const BindlessResourceEntry_Image& image_entry(const BindlessResourceHandle_Image img) const noexcept;

  private:
    BindlessSystem(UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> bindless,
                   std::vector<VkDescriptorSetLayout> set_layouts,
                   std::vector<VkDescriptorSet> descriptors);

  private:
    static constexpr const uint32_t SET_UBOS_INDEX{ 0 };
    static constexpr const uint32_t SET_STORAGE_BUFFER_INDEX{ 1 };
    static constexpr const uint32_t SET_COMBINED_IMG_SAMPLERS_INDEX{ 2 };

    struct WriteDescriptorBufferInfo
    {
        uint32_t dst_array;
        VkDescriptorBufferInfo buff_info;
    };

    struct WriteDescriptorImageInfo
    {
        uint32_t dst_array;
        VkDescriptorImageInfo img_info;
    };

    UniqueVulkanResourcePack<VkDevice, VkDescriptorPool, VkPipelineLayout> _bindless;
    std::vector<VkDescriptorSetLayout> _set_layouts;
    std::vector<VkDescriptorSet> _descriptors;
    std::vector<BindlessResourceEntry_Image> _image_resources;
    std::vector<UBOResourceEntry> _ubo_resources;
    std::vector<SBOResourceEntry> _sbo_resources;
    std::vector<WriteDescriptorBufferInfo> _writes_ubo;
    std::vector<WriteDescriptorBufferInfo> _writes_sbo;
    std::vector<WriteDescriptorImageInfo> _writes_img;
    uint32_t _handle_idx_ubos{};
    std::atomic_uint32_t _handle_idx_sbos{};
    std::atomic_uint32_t _free_slot_images{};
    std::unordered_map<VkSamplerCreateInfo, VkSampler> _sampler_table;
};

} // namespace xray::rendering
