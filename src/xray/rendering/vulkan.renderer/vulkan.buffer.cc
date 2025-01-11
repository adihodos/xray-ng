#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"

#include <cassert>
#include <Lz/Lz.hpp>
#include <itlib/small_vector.hpp>

#include "xray/base/logger.hpp"
#include "xray/base/xray.misc.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"

tl::expected<xray::rendering::VulkanBuffer, xray::rendering::VulkanError>
xray::rendering::VulkanBuffer::create(xray::rendering::VulkanRenderer& renderer,
                                      const xray::rendering::VulkanBufferCreateInfo& create_info) noexcept
{
    constexpr const auto mem_props_host_access =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

    const VkDeviceSize alignment_by_usage =
        [&create_info, mem_props_host_access, limits = &renderer.physical().properties.base.properties.limits]() {
            if (create_info.usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
                return limits->minUniformBufferOffsetAlignment;
            } else if (create_info.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
                return limits->minStorageBufferOffsetAlignment;
            } else {
                return limits->nonCoherentAtomSize;
            }
        }();

    const VkDeviceSize alignment_by_mem_access =
        create_info.memory_properties & mem_props_host_access
            ? renderer.physical().properties.base.properties.limits.nonCoherentAtomSize
            : 0;
    const VkDeviceSize alignment = std::max(alignment_by_mem_access, alignment_by_usage);

    const size_t aligned_bytes = base::align(create_info.bytes, alignment);
    const size_t aligned_allocation_size = aligned_bytes * create_info.frames;

    const size_t initial_data_size = lz::chain(create_info.initial_data)
                                         .map([](const std::span<const uint8_t> data) { return data.size_bytes(); })
                                         .sum();

    assert(create_info.bytes >= initial_data_size);

    XR_LOG_TRACE(
        "Create buffer request, bytes size = {}, alignment = {}, aligned size = {}, aligned allocation size = {}",
        create_info.bytes,
        alignment,
        aligned_bytes,
        aligned_allocation_size);

    //
    // when creating an immutable buffer add TRANSFER_DST to usage flags
    const VkBufferUsageFlags usage_flags =
        create_info.usage |
        (create_info.memory_properties & mem_props_host_access ? 0 : VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    const VkBufferCreateInfo buffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = aligned_allocation_size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkBuffer buffer;
    const VkResult create_result =
        WRAP_VULKAN_FUNC(vkCreateBuffer, renderer.device(), &buffer_create_info, nullptr, &buffer);
    XR_VK_CHECK_RESULT(create_result);

    if (create_info.name_tag)
        renderer.dbg_set_object_name(buffer, create_info.name_tag);

    xrUniqueVkBuffer sb{ buffer, VkResourceDeleter_VkBuffer{ renderer.device() } };

    const VkBufferMemoryRequirementsInfo2 mem_req_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .pNext = nullptr,
        .buffer = raw_ptr(sb),
    };

    VkMemoryRequirements2 mem_req = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = nullptr,
    };

    vkGetBufferMemoryRequirements2(renderer.device(), &mem_req_info, &mem_req);

    const VkMemoryAllocateInfo mem_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.memoryRequirements.size,
        .memoryTypeIndex = renderer.find_allocation_memory_type(mem_req.memoryRequirements.memoryTypeBits,
                                                                create_info.memory_properties),
    };

    VkDeviceMemory buffer_mem;
    const VkResult alloc_mem_result =
        WRAP_VULKAN_FUNC(vkAllocateMemory, renderer.device(), &mem_alloc_info, nullptr, &buffer_mem);
    xrUniqueVkDeviceMemory bm{ buffer_mem, VkResourceDeleter_VkDeviceMemory{ renderer.device() } };
    XR_VK_CHECK_RESULT(alloc_mem_result);

    const VkResult bind_buffer_mem_result{ WRAP_VULKAN_FUNC(
        vkBindBufferMemory, renderer.device(), raw_ptr(sb), raw_ptr(bm), 0) };
    XR_VK_CHECK_RESULT(bind_buffer_mem_result);

    if (create_info.memory_properties & mem_props_host_access) {
        //
        // not immutable so just map + copy everything there is to copy
        if (initial_data_size != 0) {
            UniqueMemoryMapping::map_memory(renderer.device(), buffer_mem, 0, aligned_bytes)
                .map([ci = &create_info](UniqueMemoryMapping mapping) {
                    uint8_t* copy_target = static_cast<uint8_t*>(mapping._mapped_memory);

                    for (const std::span<const uint8_t> region : ci->initial_data) {
                        memcpy(copy_target, region.data(), region.size_bytes());
                        copy_target += region.size_bytes();
                    }
                });
        }
    } else {
        //
        // immutable buffer, copy data to staging buffer then issue a copy command
        if (create_info.job_cmd_buf && initial_data_size != 0) {
            const uintptr_t staging_buffer_offset = renderer.reserve_staging_buffer_memory(initial_data_size);
            uintptr_t staging_buff_ptr = renderer.staging_buffer_memory() + staging_buffer_offset;

            itlib::small_vector<VkBufferCopy> buffer_copies;
            VkDeviceSize bytes_count{};

            for (std::span<const uint8_t> copy_rgn : create_info.initial_data) {
                memcpy(reinterpret_cast<void*>(staging_buff_ptr + bytes_count), copy_rgn.data(), copy_rgn.size_bytes());

                buffer_copies.push_back(VkBufferCopy{
                    .srcOffset = staging_buffer_offset + bytes_count,
                    .dstOffset = bytes_count,
                    .size = static_cast<VkDeviceSize>(copy_rgn.size_bytes()),
                });
                bytes_count += static_cast<uint32_t>(copy_rgn.size_bytes());
            }

            vkCmdCopyBuffer(*create_info.job_cmd_buf,
                            renderer.staging_buffer(),
                            buffer,
                            static_cast<uint32_t>(buffer_copies.size()),
                            buffer_copies.data());
        }
    }

    return VulkanBuffer{
        xrUniqueBufferWithMemory{ renderer.device(), unique_pointer_release(sb), unique_pointer_release(bm) },
        static_cast<uint32_t>(aligned_bytes),
        static_cast<uint32_t>(alignment),
    };
}
