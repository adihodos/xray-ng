#include "xray/rendering/vulkan.renderer/vulkan.async.tasks.hpp"

#include <coroutine>
#include <string_view>

#include <Lz/Lz.hpp>
#include "xray/base/xray.misc.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/geometry/heightmap.generator.hpp"
#include "xray/rendering/vertex_format/vertex.format.pbr.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"

using namespace std;

namespace xray::rendering::detail {

struct JobWaitToken
{
  public:
    JobWaitToken(xrUniqueVkFence&& fence) noexcept
        : _fence{ std::move(fence) }
    {
    }

    JobWaitToken(JobWaitToken&& rhs) noexcept
        : _fence{ std::move(rhs._fence) }
    {
    }

    JobWaitToken(const JobWaitToken&) = delete;
    JobWaitToken& operator=(const JobWaitToken&&) = delete;
    JobWaitToken& operator=(JobWaitToken&&) = delete;

    ~JobWaitToken()
    {
        if (_fence) {
            VkDevice device = base::unique_pointer_get_deleter(_fence)._owner;
            const VkFence fences[] = { base::raw_ptr(_fence) };
            WRAP_VULKAN_FUNC(vkWaitForFences, device, 1, fences, true, 0xffffffff);
        }
    }

  private:
    xrUniqueVkFence _fence;
};

tl::expected<VkCommandBuffer, VulkanError>
create_job(VkDevice device, VkCommandPool cmd_pool) noexcept
{
    const VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer cmd_buffer{};
    const VkResult alloc_cmdbuffs_res = WRAP_VULKAN_FUNC(vkAllocateCommandBuffers, device, &alloc_info, &cmd_buffer);
    XR_VK_CHECK_RESULT(alloc_cmdbuffs_res);

    const VkCommandBufferBeginInfo cmd_buf_begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkBeginCommandBuffer(cmd_buffer, &cmd_buf_begin_info);

    return tl::expected<VkCommandBuffer, VulkanError>{ cmd_buffer };
}

tl::expected<JobWaitToken, xray::rendering::VulkanError>
submit_job(VkCommandBuffer cmd_buffer, VkDevice device, VkQueue transfer_queue)
{
    vkEndCommandBuffer(cmd_buffer);

    using namespace xray::rendering;
    using namespace xray::base;

    xrUniqueVkFence wait_fence{ nullptr, VkResourceDeleter_VkFence{ device } };
    const VkFenceCreateInfo fence_create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    const VkResult fence_create_status =
        WRAP_VULKAN_FUNC(vkCreateFence, device, &fence_create_info, nullptr, raw_ptr_ptr(wait_fence));
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
    const VkResult submit_result =
        WRAP_VULKAN_FUNC(vkQueueSubmit, transfer_queue, 1, &submit_info, raw_ptr(wait_fence));
    XR_VK_CHECK_RESULT(submit_result);

    return JobWaitToken{ std::move(wait_fence) };
}

constexpr const string_view GRID_VS{
    R"#(
#version 460 core

#include "core/bindless.core.glsl"

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 texcoords;

layout (location = 0) out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 1) out VS_OUT_FS_IN {
    vec4 color;
    vec2 uv;
} vs_out;

void main() {
    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
    gl_Position = fgd.world_view_proj * vec4(pos, 1.0);
    const vec4 colors[3] = { { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.65, 0.65, 0.65, 1.0} };
    const float r = float(pos.x > 0 && pos.z == 0);
    const float b = float(pos.x == 0 && pos.z > 0);
    vs_out.color = colors[0] * r + colors[1] * b + colors[2] * (1.0 - r - b);
    vs_out.uv = texcoords;
}
    )#"
};

constexpr const std::string_view GRID_FS{ R"#(
#version 460 core

#include "core/bindless.core.glsl"

layout (location = 1) in VS_OUT_FS_IN {
    vec4 color;
    vec2 uv;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    FinalFragColor = fs_in.color;
}
)#" };

}

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

        const uintptr_t staging_buffer_ptr = r->staging_buffer_memory();
        uintptr_t copy_offset = r->reserve_staging_buffer_memory(bytes_to_allocate);

        g.extract_data((void*)(staging_buffer_ptr + copy_offset),
                       (void*)(staging_buffer_ptr + copy_offset + vertex_bytes),
                       { 0, 0 },
                       0);

        const auto [queue, queue_idx, queue_pool] = r->queue_data(1);
        auto cmd_buffer = detail::create_job(r->device(), queue_pool);
        XR_VK_PROPAGATE_ERROR(cmd_buffer);

        auto vertex_buffer =
            VulkanBuffer::create(*r,
                                 VulkanBufferCreateInfo{
                                     .name_tag = "vertex buffer",
                                     .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     .bytes = vertex_bytes,
                                 });
        XR_VK_PROPAGATE_ERROR(vertex_buffer);

        const VkBufferCopy copy_vtx{ .srcOffset = copy_offset, .dstOffset = 0, .size = vertex_bytes };
        vkCmdCopyBuffer(*cmd_buffer, r->staging_buffer(), vertex_buffer->buffer_handle(), 1, &copy_vtx);
        copy_offset += vertex_bytes;

        auto index_buffer = VulkanBuffer::create(
            *r,
            VulkanBufferCreateInfo{ .name_tag = "index buffer",
                                    .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                    .bytes = index_bytes });
        XR_VK_PROPAGATE_ERROR(index_buffer);

        const VkBufferCopy copy_idx{ .srcOffset = copy_offset, .dstOffset = 0, .size = index_bytes };
        vkCmdCopyBuffer(*cmd_buffer, r->staging_buffer(), index_buffer->buffer_handle(), 1, &copy_idx);
        copy_offset += index_bytes;

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

        auto material_buffer =
            VulkanBuffer::create(*r,
                                 VulkanBufferCreateInfo{
                                     .name_tag = "material buffer",
                                     .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     .bytes = base::container_bytes_size(mtl_defs),
                                 });
        XR_VK_PROPAGATE_ERROR(material_buffer);

        memcpy((void*)(staging_buffer_ptr + copy_offset), mtl_defs.data(), material_def_bytes);
        const VkBufferCopy copy_mtls{ .srcOffset = copy_offset, .dstOffset = 0, .size = material_def_bytes };
        vkCmdCopyBuffer(*cmd_buffer, r->staging_buffer(), material_buffer->buffer_handle(), 1, &copy_mtls);
        copy_offset += material_def_bytes;

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

            memcpy((void*)(staging_buffer_ptr + copy_offset), img.pixels.data(), img.pixels.size_bytes());

            XR_VK_PROPAGATE_ERROR(image);
            set_image_layout(*cmd_buffer,
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

            const VkBufferImageCopy img_cpy{
                .bufferOffset = copy_offset,
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
                .imageExtent = { img.width, img.height, 1 },
            };

            copy_offset += img.pixels.size_bytes();
            vkCmdCopyBufferToImage(
                *cmd_buffer, r->staging_buffer(), image->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_cpy);
            set_image_layout(*cmd_buffer,
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

        const auto wait_token = detail::submit_job(*cmd_buffer, r->device(), queue);
        XR_VK_PROPAGATE_ERROR(wait_token);

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

concurrencpp::result<tl::expected<xray::rendering::GeneratedGeometryWithRenderData, xray::rendering::VulkanError>>
xray::rendering::RendererAsyncTasks::create_generated_geometry_resources_task(concurrencpp::executor_tag,
                                                                              concurrencpp::thread_pool_executor*,
                                                                              VulkanRenderer* r)
{
    vector<geometry_data_t> shapes;
    shapes.push_back(geometry_factory::grid(32, 32, 1.0f, 1.0f));
    HeightmapGenerator hmg{ shapes[0].vertex_data(), sizeof(vertex_pntt), 32, 32 };
    XR_LOG_INFO("Size = {}, align = {}", sizeof(vertex_pntt), alignof(vertex_pntt));

    shapes.push_back(geometry_factory::tetrahedron());
    shapes.push_back(geometry_factory::hexahedron());
    shapes.push_back(geometry_factory::octahedron());
    shapes.push_back(geometry_factory::dodecahedron());
    shapes.push_back(geometry_factory::icosahedron());
    shapes.push_back(geometry_factory::cylinder(16, 4, 4.0f, 2.0f));
    shapes.push_back(geometry_factory::geosphere(4.0f, 8));
    shapes.push_back(geometry_factory::torus(4.0f, 2.0f, 16, 16));
    shapes.push_back(geometry_factory::conical_section(1.0f, 2.5f, 2.0f, 32, 8));

    vector<ObjectDrawData> objects;
    uint32_t vertexcount{};
    uint32_t indexcount{};

    for (const geometry_data_t& g : shapes) {
        objects.push_back(ObjectDrawData{
            .vertices = static_cast<uint32_t>(g.vertex_count),
            .indices = static_cast<uint32_t>(g.index_count),
            .vertex_offset = vertexcount,
            .index_offset = indexcount,
        });

        vertexcount += g.vertex_count;
        indexcount += g.index_count;
    }

    const size_t bytes_needed = vertexcount * sizeof(vertex_pntt) + indexcount * sizeof(uint32_t);
    const uintptr_t staging_buffer_ptr{ r->staging_buffer_memory() };
    uintptr_t copy_staging_offset{ r->reserve_staging_buffer_memory(bytes_needed) };

    auto vtx_buffer = VulkanBuffer::create(*r,
                                           VulkanBufferCreateInfo{
                                               .name_tag = "shapes vertices",
                                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                               .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               .bytes = vertexcount * sizeof(vertex_pntt),
                                           });
    XR_VK_COR_PROPAGATE_ERROR(vtx_buffer);

    auto idx_buffer = VulkanBuffer::create(*r,
                                           VulkanBufferCreateInfo{
                                               .name_tag = "shapes indices",
                                               .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                               .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               .bytes = indexcount * sizeof(uint32_t),
                                           });
    XR_VK_COR_PROPAGATE_ERROR(idx_buffer);

    auto cmd_buf = r->new_transfer_command_buffer();
    XR_VK_COR_PROPAGATE_ERROR(cmd_buf);

    VkDeviceSize copy_vtx_offset{};
    VkDeviceSize copy_idx_offset{};
    vector<VkBufferCopy> copy_rgns_vtx;
    vector<VkBufferCopy> copy_rgns_idx;

    for (const geometry_data_t& g : shapes) {
        const span<const vertex_pntt> vtx = g.vertex_span();
        //
        // vertices to staging
        memcpy((void*)(staging_buffer_ptr + copy_staging_offset), vtx.data(), vtx.size_bytes());
        //
        // copy region for vertices to dest
        copy_rgns_vtx.push_back(VkBufferCopy{
            .srcOffset = copy_staging_offset,
            .dstOffset = copy_vtx_offset,
            .size = vtx.size_bytes(),
        });

        copy_vtx_offset += vtx.size_bytes();
        copy_staging_offset += vtx.size_bytes();

        //
        // indices to staging
        const span<const uint32_t> idx = g.index_span();
        memcpy((void*)(staging_buffer_ptr + copy_staging_offset), idx.data(), idx.size_bytes());
        //
        // copy regions for indices to dest
        copy_rgns_idx.push_back(VkBufferCopy{
            .srcOffset = copy_staging_offset,
            .dstOffset = copy_idx_offset,
            .size = idx.size_bytes(),
        });

        copy_idx_offset += idx.size_bytes();
        copy_staging_offset += idx.size_bytes();
    }

    vkCmdCopyBuffer(
        *cmd_buf, r->staging_buffer(), vtx_buffer->buffer_handle(), copy_rgns_vtx.size(), copy_rgns_vtx.data());
    vkCmdCopyBuffer(
        *cmd_buf, r->staging_buffer(), idx_buffer->buffer_handle(), copy_rgns_idx.size(), copy_rgns_idx.data());

    const auto [queue, queue_idx, queue_pool] = r->queue_data(1);
    const auto wait_token = detail::submit_job(*cmd_buf, r->device(), queue);
    XR_VK_COR_PROPAGATE_ERROR(wait_token);

    auto graphics_pipeline = GraphicsPipelineBuilder{}
                                 .add_shader(ShaderStage::Vertex, detail::GRID_VS)
                                 .add_shader(ShaderStage::Fragment, detail::GRID_FS)
                                 .dynamic_state({ VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT })
                                 .rasterization_state(RasterizationState{
                                     .poly_mode = VK_POLYGON_MODE_LINE,
                                     .cull_mode = VK_CULL_MODE_NONE,
                                 })
                                 .create_bindless(*r);

    XR_VK_COR_PROPAGATE_ERROR(graphics_pipeline);

    co_return tl::expected<GeneratedGeometryWithRenderData, VulkanError>{
        tl::in_place, std::move(*vtx_buffer), std::move(*idx_buffer), std::move(*graphics_pipeline), std::move(objects),
    };
}
