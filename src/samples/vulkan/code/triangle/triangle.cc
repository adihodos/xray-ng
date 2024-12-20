#include "triangle.hpp"

#include <string_view>
#include <array>
#include <random>
#include <Lz/Lz.hpp>

#include <itlib/small_vector.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/math/scalar2x3_math.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_base.hpp"
#include "xray/rendering/colors/hsv_color.hpp"
#include "xray/rendering/colors/color_cast_rgb_hsv.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/base/veneers/sequence_container_veneer.hpp"
#include "xray/base/app_config.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;
using namespace xray::math;

class random_engine
{
  public:
    random_engine() {}

    void set_float_range(const float minval, const float maxval) noexcept
    {
        _f32distribution = std::uniform_real_distribution<float>{ minval, maxval };
    }

    void set_integer_range(const int32_t minval, const int32_t maxval) noexcept
    {
        _i32distribution = std::uniform_int_distribution<int32_t>{ minval, maxval };
    }

    float next_float() noexcept { return _f32distribution(_engine); }
    int32_t next_int() noexcept { return _i32distribution(_engine); }

  private:
    std::random_device _device{};
    std::mt19937 _engine{ _device() };
    std::uniform_real_distribution<float> _f32distribution{ 0.0f, 1.0f };
    std::uniform_int_distribution<int32_t> _i32distribution{ 0, 1 };
};

dvk::TriangleDemo::TriangleDemo(PrivateConstructionToken,
                                const app::init_context_t& init_context,
                                xray::rendering::BindlessUniformBufferResourceHandleEntryPair g_ubo,
                                xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_vertexbuffer,
                                xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_indexbuffer,
                                xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_instancebuffer,
                                xray::rendering::GraphicsPipeline pipeline,
                                xray::rendering::BindlessImageResourceHandleEntryPair g_texture,
                                xray::rendering::xrUniqueVkImageView imageview,
                                xray::rendering::xrUniqueVkSampler sampler)
    : app::DemoBase{ init_context }
    , _renderstate{
        g_ubo,     g_vertexbuffer,       g_indexbuffer,      g_instancebuffer, std::move(pipeline),
        g_texture, std::move(imageview), std::move(sampler),
    }
{
    _timer.start();
}

struct FrameGlobalData
{
    mat4f world_view_proj;
    mat4f projection;
    mat4f inv_projection;
    mat4f view;
    mat4f ortho;
    vec3f eye_pos;
    uint32_t frame;
};

struct InstanceRenderInfo
{
    mat4f model;
    uint32_t vtx_buff;
    uint32_t idx_buff;
    uint32_t mtl_coll;
    uint32_t mtl;
};

struct VertexPTC
{
    vec2f pos;
    vec2f uv;
    vec4f color;
};

template<typename T, size_t N>
inline constexpr std::span<const uint8_t>
to_bytes_span(const T (&array_ref)[N]) noexcept
{
    return std::span{ reinterpret_cast<const uint8_t*>(&array_ref[0]), sizeof(array_ref) };
}

xray::base::unique_pointer<app::DemoBase>
dvk::TriangleDemo::create(const app::init_context_t& init_ctx)
{
    const RenderBufferingSetup rbs{ init_ctx.renderer->buffering_setup() };

    auto pkgs{ init_ctx.renderer->create_work_package() };
    if (!pkgs)
        return nullptr;

    const VulkanBufferCreateInfo bci{
        .name_tag = "UBO - FrameGlobalData",
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .bytes = sizeof(FrameGlobalData),
        .frames = rbs.buffers,
        .initial_data = {},
    };

    auto g_ubo{ VulkanBuffer::create(*init_ctx.renderer, bci) };
    if (!g_ubo)
        return nullptr;

    constexpr const VertexPTC tri_vertices[] = {
        { vec2f{ -1.0f, -1.0f }, vec2f{ 0.0f, 1.0f }, vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } },
        { vec2f{ 1.0f, -1.0f }, vec2f{ 1.0f, 1.0f }, vec4f{ 0.0f, 1.0f, 0.0f, 1.0f } },
        { vec2f{ 0.0f, 1.0f }, vec2f{ 0.5f, 0.0f }, vec4f{ 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    const VulkanBufferCreateInfo vbinfo{
        .name_tag = "Triangle vertices",
        .work_package = pkgs->pkg,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bytes = sizeof(tri_vertices),
        .frames = 1,
        .initial_data = { to_bytes_span(tri_vertices) },
    };

    auto g_vertexbuffer{ VulkanBuffer::create(*init_ctx.renderer, vbinfo) };
    if (!g_vertexbuffer)
        return nullptr;

    constexpr const uint32_t tri_indices[] = { 0, 1, 2 };
    const VulkanBufferCreateInfo ibinfo{
        .name_tag = "Triangle index buffer",
        .work_package = pkgs->pkg,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bytes = sizeof(tri_indices),
        .frames = 1,
        .initial_data = { to_bytes_span(tri_indices) },
    };

    auto g_indexbuffer{ VulkanBuffer::create(*init_ctx.renderer, ibinfo) };
    if (!g_indexbuffer)
        return nullptr;

    const VulkanBufferCreateInfo inst_buf_info{
        .name_tag = "Instances buffer",
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .bytes = 1024 * sizeof(InstanceRenderInfo),
        .frames = rbs.buffers,
        .initial_data = {},
    };

    auto g_instance_buffer{ VulkanBuffer::create(*init_ctx.renderer, inst_buf_info) };
    if (!g_instance_buffer)
        return nullptr;

    const char* const tex_file{ "uv_grids/ash_uvgrid01.ktx2" };

    auto pixel_buffer{
        VulkanImage::from_file(*init_ctx.renderer,
                               VulkanImageLoadInfo{
                                   .tag_name = tex_file,
                                   .wpkg = pkgs->pkg,
                                   .path = init_ctx.cfg->texture_path(tex_file),
                                   .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                                   .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                   .tiling = VK_IMAGE_TILING_OPTIMAL,
                               }),
    };

    if (!pixel_buffer) {
        return nullptr;
    }

    VulkanImage image{ std::move(*pixel_buffer) };
    init_ctx.renderer->submit_work_package(pkgs->pkg);

    tl::expected<GraphicsPipeline, VulkanError> pipeline{
        GraphicsPipelineBuilder{}
            .add_shader(ShaderStage::Vertex, init_ctx.cfg->shader_root() / "triangle/tri.vert")
            .add_shader(ShaderStage::Fragment, init_ctx.cfg->shader_root() / "triangle/tri.frag")
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .create_bindless(*init_ctx.renderer),
        // .create(*init_ctx.renderer,
        //         GraphicsPipelineCreateData{
        //             .uniform_descriptors = 16,
        //             .storage_buffer_descriptors = 256,
        //             .combined_image_sampler_descriptors = 256,
        //             .image_descriptors = 1,
        //         }),
    };

    if (!pipeline)
        return nullptr;

    xrUniqueVkSampler sampler{
        [&]() {
            const VkSamplerCreateInfo create_info{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .mipLodBias = 0.0f,
                .anisotropyEnable = false,
                .maxAnisotropy = 1.0f,
                .compareEnable = false,
                .compareOp = VK_COMPARE_OP_NEVER,
                .minLod = 0.0f,
                .maxLod = VK_LOD_CLAMP_NONE,
                .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalizedCoordinates = false,
            };

            VkSampler sampler{};
            WRAP_VULKAN_FUNC(vkCreateSampler, init_ctx.renderer->device(), &create_info, nullptr, &sampler);
            return sampler;
        }(),
        VkResourceDeleter_VkSampler{ init_ctx.renderer->device() },
    };

    xrUniqueVkImageView img_view{
        [&]() {
            const VkImageViewCreateInfo img_view_create{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = image.image(),
                .viewType = image._info.viewType,
                .format = image._info.imageFormat,
                .components =
                    VkComponentMapping{
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = image._info.levelCount,
                        .baseArrayLayer = 0,
                        .layerCount = image._info.layerCount,
                    },
            };
            VkImageView img_view{};
            WRAP_VULKAN_FUNC(vkCreateImageView, init_ctx.renderer->device(), &img_view_create, nullptr, &img_view);
            return img_view;
        }(),
        VkResourceDeleter_VkImageView{ init_ctx.renderer->device() }
    };

    //
    // wait for data to be uploaded to GPU
    init_ctx.renderer->wait_on_packages({ pkgs->pkg });

    return xray::base::make_unique<TriangleDemo>(
        PrivateConstructionToken{},
        init_ctx,
        init_ctx.renderer->bindless_sys().add_chunked_uniform_buffer(std::move(*g_ubo), rbs.buffers),
        init_ctx.renderer->bindless_sys().add_storage_buffer(std::move(*g_vertexbuffer)),
        init_ctx.renderer->bindless_sys().add_storage_buffer(std::move(*g_indexbuffer)),
        init_ctx.renderer->bindless_sys().add_chunked_storage_buffer(std::move(*g_instance_buffer), rbs.buffers),
        std::move(*pipeline),
        init_ctx.renderer->bindless_sys().add_image(std::move(image), raw_ptr(img_view), raw_ptr(sampler)),
        std::move(img_view),
        std::move(sampler));
}

void
dvk::TriangleDemo::event_handler(const xray::ui::window_event& evt)
{
    if (is_input_event(evt)) {
        if (evt.event.key.keycode == KeySymbol::escape) {
            _quit_receiver();
            return;
        }
    }
}

void
dvk::TriangleDemo::loop_event(const app::RenderEvent& render_event)
{
    static constexpr const auto sixty_herz = std::chrono::duration<float, std::milli>{ 1000.0f / 60.0f };

    _timer.end();
    const auto elapsed_duration = std::chrono::duration<float, std::milli>{ _timer.ts_end() - _timer.ts_start() };

    if (elapsed_duration > sixty_herz) {
        _simstate.angle += 0.025f;
        if (_simstate.angle >= xray::math::two_pi<float>)
            _simstate.angle -= xray::math::two_pi<float>;
        _timer.update_and_reset();
    }

    const FrameRenderData frt{ render_event.renderer->begin_rendering() };

    render_event.renderer->dbg_marker_begin(frt.cmd_buf, "Update UBO & instances", color_palette::web::orange_red);

    const auto [g_ubo_handle, g_ubo_entry] = _renderstate.g_ubo;

    UniqueMemoryMapping::create(render_event.renderer->device(),
                                g_ubo_entry.memory,
                                frt.id * g_ubo_entry.aligned_chunk_size,
                                g_ubo_entry.aligned_chunk_size,
                                0)
        .map([frt](UniqueMemoryMapping ubo) {
            FrameGlobalData* fgd = ubo.as<FrameGlobalData>();
            fgd->frame = frt.id;
            fgd->world_view_proj = mat4f::stdc::identity;
            fgd->view = mat4f::stdc::identity;
            fgd->eye_pos = vec3f::stdc::zero;
            fgd->ortho = mat4f::stdc::identity;
            fgd->projection = mat4f::stdc::identity;
        });

    const auto [g_instance_buffer_handle, g_instance_buffer_entry] = _renderstate.g_instancebuffer;
    const auto [vertex_buffer, evb] = destructure_bindless_resource_handle(_renderstate.g_vertexbuffer.first);
    const auto [index_buffer, eib] = destructure_bindless_resource_handle(_renderstate.g_indexbuffer.first);

    UniqueMemoryMapping::create(render_event.renderer->device(),
                                g_instance_buffer_entry.memory,
                                frt.id * g_instance_buffer_entry.aligned_chunk_size,
                                g_instance_buffer_entry.aligned_chunk_size,
                                0)
        .map([angle = _simstate.angle, vertex_buffer, index_buffer](UniqueMemoryMapping inst_buf) {
            const xray::math::scalar2x3<float> r = xray::math::R2::rotate(angle);
            const std::array<float, 4> vkmtx{ r.a00, r.a01, r.a10, r.a11 };

            InstanceRenderInfo* inst = inst_buf.as<InstanceRenderInfo>();
            inst->mtl = 0;
            inst->model = mat4f{
                // 1st row
                r.a00,
                r.a01,
                0.0f,
                0.0f,
                // 2nd row
                r.a10,
                r.a11,
                0.0f,
                0.0f,
                // 3rd row
                0.0f,
                0.0f,
                1.0f,
                0.0f,
                // 4th row
                0.0f,
                0.0f,
                0.0f,
                1.0f,
            };
            inst->idx_buff = index_buffer;
            inst->vtx_buff = vertex_buffer;
            inst->mtl_coll = 0;
        });

    render_event.renderer->dbg_marker_end(frt.cmd_buf);

    const VkViewport viewport{
        .x = 0.0f,
        .y = static_cast<float>(frt.fbsize.height),
        .width = static_cast<float>(frt.fbsize.width),
        .height = -static_cast<float>(frt.fbsize.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(frt.cmd_buf, 0, 1, &viewport);

    render_event.renderer->dbg_marker_insert(frt.cmd_buf, "Rendering triangle", color_palette::web::sea_green);

    const VkRect2D scissor{
        .offset = VkOffset2D{ 0, 0 },
        .extent = frt.fbsize,
    };

    vkCmdSetScissor(frt.cmd_buf, 0, 1, &scissor);
    render_event.renderer->clear_attachments(frt.cmd_buf, 1.0f, 0.0f, 1.0f);

    vkCmdBindPipeline(frt.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderstate.pipeline.handle());
    render_event.renderer->bindless_sys().flush_descriptors(*render_event.renderer);
    const std::span<const VkDescriptorSet> descriptors{ render_event.renderer->bindless_sys().descriptor_sets() };
    vkCmdBindDescriptorSets(frt.cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _renderstate.pipeline.layout(),
                            0,
                            static_cast<uint32_t>(descriptors.size()),
                            descriptors.data(),
                            0,
                            nullptr);

    vkCmdPushConstants(frt.cmd_buf,
                       _renderstate.pipeline.layout(),
                       VK_SHADER_STAGE_ALL,
                       0,
                       static_cast<uint32_t>(sizeof(frt.id)),
                       &frt.id);

    vkCmdDraw(frt.cmd_buf, 3, 1, 0, 0);
    render_event.renderer->end_rendering();
}
