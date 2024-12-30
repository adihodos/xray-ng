#include "xray/rendering/vulkan.renderer/vulkan.async.tasks.hpp"

#include <coroutine>

#include <Lz/Lz.hpp>
#include "xray/base/xray.misc.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/rendering/vertex_format/vertex.format.pbr.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

using namespace std;

concurrencpp::result<tl::expected<xray::rendering::GeometryWithRenderData, xray::rendering::VulkanError>>
xray::rendering::RendererAsyncTasks::create_rendering_resources_task(concurrencpp::executor_tag,
                                                                     concurrencpp::thread_pool_executor*,
                                                                     xray::rendering::VulkanRenderer* r,
                                                                     xray::rendering::LoadedGeometry geometry)
{
    co_return [r, g = std::move(geometry)]() mutable -> tl::expected<GeometryWithRenderData, VulkanError> {
        const math::vec2ui32 buffer_sizes = g.compute_vertex_index_count();
        xray::rendering::ExtractedMaterialsWithImageSourcesBundle material_data = g.extract_materials(0);

        const size_t image_bytes = lz::chain(material_data.image_sources)
                                       .map([](const ExtractedImageData& img) { return img.pixels.size_bytes(); })
                                       .sum();

        const size_t material_def_bytes = material_data.materials.size() * sizeof(PBRMaterialDefinition);
        const size_t vertex_bytes = buffer_sizes.x * sizeof(VertexPBR);
        const size_t index_bytes = buffer_sizes.y * sizeof(uint32_t);

        const size_t bytes_to_allocate =
            base::align<size_t>(vertex_bytes + index_bytes + image_bytes + material_def_bytes,
                                r->physical().properties.base.properties.limits.nonCoherentAtomSize);

        const uintptr_t staging_buffer_start_addr = r->reserve_staging_buffer_memory(bytes_to_allocate);
        uintptr_t staging_buffer_offset = staging_buffer_start_addr;
        g.extract_data((void*)staging_buffer_offset, (void*)(staging_buffer_offset + vertex_bytes), { 0, 0 }, 0);

        const auto [queue, queue_idx, queue_pool] = r->queue_data(1);
        const VkCommandBufferAllocateInfo alloc_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                         .pNext = nullptr,
                                                         .commandPool = queue_pool,
                                                         .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                         .commandBufferCount = 1 };
        VkCommandBuffer cmd_buffer{};
        const VkResult alloc_cmdbuffs_res =
            WRAP_VULKAN_FUNC(vkAllocateCommandBuffers, r->device(), &alloc_info, &cmd_buffer);
        XR_VK_CHECK_RESULT(alloc_cmdbuffs_res);

        const VkCommandBufferBeginInfo cmd_buf_begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        vkBeginCommandBuffer(cmd_buffer, &cmd_buf_begin_info);

        auto vertex_buffer =
            VulkanBuffer::create(*r,
                                 VulkanBufferCreateInfo{
                                     .name_tag = "vertex buffer",
                                     .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     .bytes = vertex_bytes,
                                 });

        XR_VK_PROPAGATE_ERROR(vertex_buffer);

        const VkBufferCopy copy_vtx{ .srcOffset = staging_buffer_offset - staging_buffer_start_addr,
                                     .dstOffset = 0,
                                     .size = vertex_bytes };
        vkCmdCopyBuffer(cmd_buffer, r->staging_buffer(), vertex_buffer->buffer_handle(), 1, &copy_vtx);
        staging_buffer_offset += vertex_bytes;

        auto index_buffer = VulkanBuffer::create(
            *r,
            VulkanBufferCreateInfo{ .name_tag = "index buffer",
                                    .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    .bytes = index_bytes });
        XR_VK_PROPAGATE_ERROR(index_buffer);

        const VkBufferCopy copy_idx{ .srcOffset = staging_buffer_offset - staging_buffer_start_addr,
                                     .dstOffset = 0,
                                     .size = index_bytes };
        vkCmdCopyBuffer(cmd_buffer, r->staging_buffer(), index_buffer->buffer_handle(), 1, &copy_idx);
        staging_buffer_offset += index_bytes;

        const uint32_t mtl_offset =
            r->bindless_sys().reserve_image_slots(static_cast<uint32_t>(material_data.image_sources.size()));
        vector<PBRMaterialDefinition> mtl_defs;
        for (const ExtractedMaterialDefinition& mtdef : material_data.materials) {
            mtl_defs.push_back(PBRMaterialDefinition{
                .base_color_factor = mtdef.base_color_factor,
                .base_color = mtl_offset + mtdef.base_color,
                .metallic = mtl_offset + mtdef.metallic,
                .normal = mtl_offset + mtdef.normal,
                .metallic_factor = mtdef.metallic_factor,
                .roughness_factor = mtdef.roughness_factor,
            });
        }

        auto material_buffer = VulkanBuffer::create(
            *r,
            VulkanBufferCreateInfo{ .name_tag = "material buffer",
                                    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    .bytes = base::container_bytes_size(mtl_defs) });
        XR_VK_PROPAGATE_ERROR(material_buffer);

        memcpy((void*)staging_buffer_offset, mtl_defs.data(), material_def_bytes);
        const VkBufferCopy copy_mtls{ .srcOffset = staging_buffer_offset - staging_buffer_start_addr,
                                      .dstOffset = 0,
                                      .size = material_def_bytes };
        vkCmdCopyBuffer(cmd_buffer, r->staging_buffer(), material_buffer->buffer_handle(), 1, &copy_mtls);
        staging_buffer_offset += material_def_bytes;

        vector<VulkanImage> images;
        vector<VkBufferImageCopy> img_copies;
        for (const ExtractedImageData& img : material_data.image_sources) {
            auto image = VulkanImage::from_memory(
                *r,
                VulkanImageCreateInfo{
                    .tag_name = "some image",
                    .type = VK_IMAGE_TYPE_2D,
                    .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                    .memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    .format = VK_FORMAT_R8G8B8A8_UNORM,
                    .width = img.width,
                    .height = img.height,
                    .layers = 1,
                });

            memcpy((void*)staging_buffer_offset, img.pixels.data(), img.pixels.size_bytes());

            XR_VK_PROPAGATE_ERROR(image);
            set_image_layout(cmd_buffer,
                             image->image(),
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VkImageSubresourceRange{
                                 .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel = 0,
                                 .levelCount = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount = 1,
                             });

            const VkBufferImageCopy img_cpy{ .bufferOffset = staging_buffer_offset - staging_buffer_start_addr,
                                             .bufferRowLength = 0,
                                             .bufferImageHeight = 0,
                                             .imageSubresource =
                                                 VkImageSubresourceLayers{
                                                     .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                     .mipLevel = 0,
                                                     .baseArrayLayer = 0,
                                                     .layerCount = 1,
                                                 },
                                             .imageOffset = {},
                                             .imageExtent = { img.width, img.height, 1 }

            };

            staging_buffer_offset += img.pixels.size_bytes();
            vkCmdCopyBufferToImage(
                cmd_buffer, r->staging_buffer(), image->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_cpy);
            set_image_layout(cmd_buffer,
                             image->image(),
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

                             VkImageSubresourceRange{
                                 .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel = 0,
                                 .levelCount = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount = 1,
                             });

            images.push_back(std::move(*image));
        }

        vkEndCommandBuffer(cmd_buffer);

        xrUniqueVkFence wait_fence{ nullptr, VkResourceDeleter_VkFence{ r->device() } };
        const VkFenceCreateInfo fence_create_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        const VkResult fence_create_status =
            WRAP_VULKAN_FUNC(vkCreateFence, r->device(), &fence_create_info, nullptr, raw_ptr_ptr(wait_fence));
        XR_VK_CHECK_RESULT(fence_create_status);

        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        const VkResult submit_result = WRAP_VULKAN_FUNC(vkQueueSubmit, queue, 1, &submit_info, raw_ptr(wait_fence));
        XR_VK_CHECK_RESULT(submit_result);

        const VkFence fences[] = { raw_ptr(wait_fence) };
        const VkResult wait_result = WRAP_VULKAN_FUNC(vkWaitForFences, r->device(), 1, fences, true, 0xffffffff);
        XR_VK_CHECK_RESULT(wait_result);

        return tl::expected<GeometryWithRenderData, VulkanError>{
            tl::in_place,
            std::move(g),
            std::move(*vertex_buffer),
            std::move(*index_buffer),
            std::move(*material_buffer),
            std::move(images),
            mtl_offset,
            buffer_sizes.x,
            buffer_sizes.y,
        };
    }();
}
