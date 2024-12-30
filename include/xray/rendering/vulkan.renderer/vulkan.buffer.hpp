#pragma once

#include "xray/xray.hpp"

#include <initializer_list>
#include <span>

#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <vulkan/vulkan.h>

#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.handles.hpp"

namespace xray::rendering {

class VulkanRenderer;

struct VulkanBufferCreateInfo
{
    const char* name_tag{};
    tl::optional<WorkPackageHandle> work_package{};
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_properties;
    size_t bytes{ 1024 };
    size_t frames{ 1 };
    std::initializer_list<std::span<const uint8_t>> initial_data;
};

using xrUniqueBufferWithMemory = UniqueVulkanResourcePack<VkDevice, VkBuffer, VkDeviceMemory>;

struct VulkanBuffer
{
    static constexpr const uint32_t NO_FRAME{ 0xffffffffu };

    xrUniqueBufferWithMemory buffer;
    uint32_t aligned_size;
    uint32_t alignment;

    VkDeviceMemory memory_handle() const noexcept { return buffer.handle<VkDeviceMemory>(); }
    VkBuffer buffer_handle() const noexcept { return buffer.handle<VkBuffer>(); }

    auto release() noexcept { buffer.release(); }

    tl::expected<UniqueMemoryMapping, VulkanError> mmap(VkDevice device,
                                                        const tl::optional<uint32_t> frame_id) const noexcept
    {
        return UniqueMemoryMapping::map_memory(device,
                                               buffer.handle<VkDeviceMemory>(),
                                               aligned_size * frame_id.value_or(0),
                                               frame_id ? aligned_size : VK_WHOLE_SIZE);
    }

    static tl::expected<VulkanBuffer, VulkanError> create(VulkanRenderer& renderer,
                                                          const VulkanBufferCreateInfo& create_info) noexcept;
};

} // namespace xray::rendering
