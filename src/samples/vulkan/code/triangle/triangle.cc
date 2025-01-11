#include "triangle.hpp"

#include <array>
#include <random>

#include <concurrencpp/concurrencpp.h>
#include <Lz/Lz.hpp>
#include <itlib/small_vector.hpp>
#include <imgui/imgui.h>

#include "xray/base/app_config.hpp"
#include "xray/base/xray.misc.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.async.tasks.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/geometry.importer.gltf.hpp"
#include "xray/rendering/geometry.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"

#include "bindless.pipeline.config.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;
using namespace xray::math;

namespace xr = xray::rendering;
namespace xm = xray::math;

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

struct dvk::TriangleDemo::WorldState
{
    Geometry geometry;
};

dvk::TriangleDemo::SimState::SimState(const app::init_context_t& init_context)
    : arcball_cam{ xray::math::vec3f::stdc::zero, 1.0f, { init_context.surface_width, init_context.surface_height } }
{
    //
    // projection
    const auto perspective_projection = xm::perspective_symmetric(
        (float)init_context.surface_width / (float)init_context.surface_height, xm::radians(65.0f), 0.1f, 1000.0f);

    // TODO: check perspective function, doesnt seem to be correct
    // xray::math::perspective(0.0f,
    //                         static_cast<float>(init_context.surface_width),
    //                         static_cast<float>(init_context.surface_height),
    //                         0.0f,
    //                         0.1f,
    //                         1000.0f);
    // camera.set_view_matrix(xm::look_at({ 0.0f, 10.0f, -10.0f }, vec3f::stdc::zero, vec3f::stdc::unit_y));
    camera.set_projection(perspective_projection);
}

dvk::TriangleDemo::TriangleDemo(PrivateConstructionToken,
                                const app::init_context_t& init_context,
                                xray::rendering::VulkanBuffer&& g_vertexbuffer,
                                xray::rendering::VulkanBuffer&& g_indexbuffer,
                                xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_instancebuffer,
                                xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_materials,
                                xray::rendering::GraphicsPipeline pipeline,
                                xray::base::unique_pointer<WorldState> world,
                                RenderState::GeometricShapes&& grid)
    : app::DemoBase{ init_context }
    , _simstate{ init_context }
    , _renderstate{ std::move(g_vertexbuffer),
                    std::move(g_indexbuffer),
                    g_instancebuffer,
                    g_materials,
                    std::move(pipeline),
                    std::move(grid),
    }
    , _world{ std::move(world) }
{
    _timer.start();
}

dvk::TriangleDemo::~TriangleDemo() {}

xray::base::unique_pointer<app::DemoBase>
dvk::TriangleDemo::create(
    const app::init_context_t& init_ctx,
    concurrencpp::result<tl::expected<GeometryWithRenderData, VulkanError>> bundle,
    concurrencpp::result<tl::expected<GeneratedGeometryWithRenderData, VulkanError>> generated_objects)
{
    const RenderBufferingSetup rbs{ init_ctx.renderer->buffering_setup() };

    tl::expected<GraphicsPipeline, VulkanError> pipeline{
        GraphicsPipelineBuilder{}
            .add_shader(ShaderStage::Vertex, init_ctx.cfg->shader_root() / "triangle/tri.vert")
            .add_shader(ShaderStage::Fragment, init_ctx.cfg->shader_root() / "triangle/tri.frag")
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .rasterization_state({ .poly_mode = VK_POLYGON_MODE_FILL,
                                   .cull_mode = VK_CULL_MODE_BACK_BIT,
                                   .front_face = VK_FRONT_FACE_CLOCKWISE,
                                   .line_width = 1.0f })
            .create_bindless(*init_ctx.renderer),
    };

    if (!pipeline)
        return nullptr;

    const VulkanBufferCreateInfo inst_buf_info{
        .name_tag = "Global instances buffer",
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .bytes = 1024 * sizeof(app::InstanceRenderInfo),
        .frames = rbs.buffers,
        .initial_data = {},
    };

    auto g_instance_buffer{ VulkanBuffer::create(*init_ctx.renderer, inst_buf_info) };
    if (!g_instance_buffer)
        return nullptr;

    auto sampler = init_ctx.renderer->bindless_sys().get_sampler(
        VkSamplerCreateInfo{
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
        },
        *init_ctx.renderer);

    if (!sampler) {
        return nullptr;
    }

    //
    // generic geometry objects
    auto gen_objs = generated_objects.get();
    if (!gen_objs)
        return nullptr;

    //
    // wait for the resource loading task to complete
    auto loaded_resources = bundle.get();
    if (!loaded_resources) {
        return nullptr;
    }

    for (uint32_t idx = 0, count = static_cast<uint32_t>(loaded_resources->images.size()); idx < count; ++idx) {
        const auto [bindless_handle, bindless_data] = init_ctx.renderer->bindless_sys().add_image(
            std::move(loaded_resources->images[idx]), *sampler, idx + loaded_resources->images_slot);
        init_ctx.renderer->queue_image_ownership_transfer(bindless_handle);
    }

    return xray::base::make_unique<TriangleDemo>(
        PrivateConstructionToken{},
        init_ctx,
        std::move(loaded_resources->vertex_buffer),
        std::move(loaded_resources->index_buffer),
        init_ctx.renderer->bindless_sys().add_chunked_storage_buffer(std::move(*g_instance_buffer), rbs.buffers, tl::nullopt),
        init_ctx.renderer->bindless_sys().add_storage_buffer(std::move(loaded_resources->material_buffer), tl::nullopt),
        std::move(*pipeline),
        xray::base::make_unique<WorldState>(Geometry{
            .nodes = std::move(loaded_resources->geometry.nodes),
            .vertex_index_counts = { loaded_resources->vertexcount, loaded_resources->indexcount },
            .boundingbox = loaded_resources->geometry.bounding_box,
            .bounding_sphere = loaded_resources->geometry.bounding_sphere,
        }),
        RenderState::GeometricShapes{
            .vertexbuffer = std::move(gen_objs->vertex_buffer),
            .indexbuffer = std::move(gen_objs->index_buffer),
            .pipeline = std::move(gen_objs->pipeline),
            .objects = std::move(gen_objs->objects),
        });
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

    if (evt.type == xray::ui::event_type::configure) {
        const xray::ui::window_configure_event* wce = &evt.event.configure;
        if (wce->width != 0 && wce->height != 0) {
            _simstate.camera.set_projection(xray::math::perspective(
                0.0f, static_cast<float>(wce->width), 0.0f, static_cast<float>(wce->height), 0.1f, 1000.0f));
        }
    }

    _simstate.arcball_cam.input_event(evt);
}

void
dvk::TriangleDemo::user_interface(xray::ui::user_interface* ui)
{
    char txt_scratch_buff[1024];

    if (ImGui::Begin("Demo options")) {

        ImGui::Checkbox("Draw world coordinate axis", &_uistate.draw_world_axis);
        ImGui::Checkbox("Draw ship", &_uistate.draw_ship);
        ImGui::Checkbox("Draw bounding box", &_uistate.draw_bbox);
        ImGui::Checkbox("Draw individual nodes bounding boxes", &_uistate.draw_nodes_bbox);
        ImGui::Checkbox("Draw bounding sphere", &_uistate.draw_sphere);
        ImGui::Checkbox("Draw individual nodes bounding sphere", &_uistate.draw_nodes_spheres);

        ImGui::SeparatorText("Geometric shapes");
        for (size_t i = 0, count = std::min(_renderstate._shapes.objects.size(), _uistate.shapes_draw.size()); i < count;
             ++i) {
            bool value = _uistate.shapes_draw[i];

            auto out = fmt::format_to_n(txt_scratch_buff, size(txt_scratch_buff) - 1, "Object #{}", i);
            *out.out = 0;

            ImGui::Checkbox(txt_scratch_buff, &value);
            _uistate.shapes_draw[i] = value;
        }

        ImGui::SeparatorText("Geometry");
        if (ImGui::CollapsingHeader("Nodes")) {

            for (const GeometryNode& node : _world->geometry.nodes) {
                auto result = fmt::format_to_n(txt_scratch_buff,
                                               size(txt_scratch_buff),
                                               "{} ({} vertices, {} indices)",
                                               node.name,
                                               node.vertex_count,
                                               node.index_count);
                *result.out = 0;
                ImGui::Text("%s", txt_scratch_buff);
            }
        }
    }
    ImGui::End();
}

void
dvk::TriangleDemo::loop_event(const app::RenderEvent& render_event)
{
    user_interface(render_event.ui);

    _simstate.arcball_cam.update_camera(_simstate.camera);

    static constexpr const auto sixty_herz = std::chrono::duration<float, std::milli>{ 1000.0f / 60.0f };

    _timer.end();
    const auto elapsed_duration = std::chrono::duration<float, std::milli>{ _timer.ts_end() - _timer.ts_start() };

    if (elapsed_duration > sixty_herz) {
        _simstate.angle += 0.025f;
        if (_simstate.angle >= xray::math::two_pi<float>)
            _simstate.angle -= xray::math::two_pi<float>;
        _timer.update_and_reset();
    }

    if (_uistate.draw_world_axis) {
        render_event.dbg_draw->draw_coord_sys(vec3f::stdc::zero,
                                              vec3f::stdc::unit_x,
                                              vec3f::stdc::unit_y,
                                              vec3f::stdc::unit_z,
                                              2.0f,
                                              color_palette::material::red,
                                              color_palette::material::green,
                                              color_palette::material::blue);
    }

    if (_uistate.draw_sphere) {
        render_event.dbg_draw->draw_sphere(_world->geometry.bounding_sphere.center,
                                           _world->geometry.bounding_sphere.radius,
                                           color_palette::material::orange);
    }

    if (_uistate.draw_nodes_spheres) {
        for (const GeometryNode& node : _world->geometry.nodes) {
            if (node.index_count != 0) {
                render_event.dbg_draw->draw_sphere(
                    node.bounding_sphere.center, node.bounding_sphere.radius, color_palette::material::orange);
            }
        }
    }

    if (_uistate.draw_bbox) {
        render_event.dbg_draw->draw_axis_aligned_box(
            _world->geometry.boundingbox.min, _world->geometry.boundingbox.max, color_palette::material::yellow50);
    }

    if (_uistate.draw_nodes_bbox) {
        for (const GeometryNode& node : _world->geometry.nodes) {
            if (node.index_count != 0) {
                render_event.dbg_draw->draw_axis_aligned_box(
                    node.boundingbox.min, node.boundingbox.max, color_palette::material::yellow900);
            }
        }
    }

    render_event.renderer->dbg_marker_begin(
        render_event.frame_data->cmd_buf, "Update UBO & instances", color_palette::web::orange_red);

    {
        app::FrameGlobalData* fgd = render_event.g_ubo_data;
        SimState* s = &_simstate;
        fgd->world_view_proj = s->camera.projection_view(); // identity for model -> world
        fgd->view = s->camera.view();
        fgd->eye_pos = s->camera.origin();
        fgd->projection = s->camera.projection();
    }

    const auto [g_instance_buffer_handle, g_instance_buffer_entry] = _renderstate.g_instancebuffer;

    UniqueMemoryMapping::map_memory(render_event.renderer->device(),
                                    g_instance_buffer_entry.memory,
                                    render_event.frame_data->id * g_instance_buffer_entry.aligned_chunk_size,
                                    g_instance_buffer_entry.aligned_chunk_size)
        .map([angle = _simstate.angle,
              mtl = _renderstate.g_materials_buffer.first.value_of()](UniqueMemoryMapping inst_buf) {
            const xray::math::scalar2x3<float> r = xray::math::R2::rotate(angle);
            const std::array<float, 4> vkmtx{ r.a00, r.a01, r.a10, r.a11 };

            app::InstanceRenderInfo* inst = inst_buf.as<app::InstanceRenderInfo>();
            inst->mtl = mtl;
            inst->model = xray::math::mat4f::stdc::identity;

            inst->idx_buff = 0;
            inst->vtx_buff = 0;
            inst->mtl_coll = 0;
        });

    render_event.renderer->dbg_marker_end(render_event.frame_data->cmd_buf);

    render_event.renderer->dbg_marker_insert(
        render_event.frame_data->cmd_buf, "Rendering triangle", color_palette::web::sea_green);

    const VkViewport viewport{
        .x = 0.0f,
        .y = static_cast<float>(render_event.frame_data->fbsize.height),
        .width = static_cast<float>(render_event.frame_data->fbsize.width),
        .height = -static_cast<float>(render_event.frame_data->fbsize.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(render_event.frame_data->cmd_buf, 0, 1, &viewport);

    const VkRect2D scissor{
        .offset = VkOffset2D{ 0, 0 },
        .extent = render_event.frame_data->fbsize,
    };

    vkCmdSetScissor(render_event.frame_data->cmd_buf, 0, 1, &scissor);
    render_event.renderer->clear_attachments(render_event.frame_data->cmd_buf, 0.0f, 0.0f, 0.0f);

    if (_uistate.draw_ship) {
        vkCmdBindPipeline(
            render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderstate.pipeline.handle());

        const uint32_t push_const{ bindless_subresource_handle_from_bindless_resource_handle(
                                       _renderstate.g_instancebuffer.first, render_event.frame_data->id)
                                           .value_of()
                                       << 16 |
                                   render_event.frame_data->id };
        vkCmdPushConstants(render_event.frame_data->cmd_buf,
                           _renderstate.pipeline.layout(),
                           VK_SHADER_STAGE_ALL,
                           0,
                           static_cast<uint32_t>(sizeof(render_event.frame_data->id)),
                           &push_const);

        const VkDeviceSize vtx_offsets[] = { 0 };
        const VkBuffer vertex_buffers[] = { _renderstate.g_vertexbuffer.buffer_handle() };
        vkCmdBindVertexBuffers(render_event.frame_data->cmd_buf, 0, 1, vertex_buffers, vtx_offsets);
        vkCmdBindIndexBuffer(
            render_event.frame_data->cmd_buf, _renderstate.g_indexbuffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(render_event.frame_data->cmd_buf, _world->geometry.vertex_index_counts.y, 1, 0, 0, 0);
    }

    //
    // geometry objects
    if (_uistate.shapes_draw.count() != 0) {
        const RenderState::GeometricShapes* g = &_renderstate._shapes;
        vkCmdBindPipeline(render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, g->pipeline.handle());

        const VkBuffer vtx_buffers[] = { g->vertexbuffer.buffer_handle() };
        const VkDeviceSize vtx_offsets[] = { 0 };
        vkCmdBindVertexBuffers(render_event.frame_data->cmd_buf, 0, 1, vtx_buffers, vtx_offsets);
        vkCmdBindIndexBuffer(render_event.frame_data->cmd_buf, g->indexbuffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);

        const uint32_t push_const{ render_event.frame_data->id };

        vkCmdPushConstants(render_event.frame_data->cmd_buf,
                           g->pipeline.layout(),
                           VK_SHADER_STAGE_ALL,
                           0,
                           static_cast<uint32_t>(sizeof(render_event.frame_data->id)),
                           &push_const);

        for (size_t idx = 0, count = std::min(_uistate.shapes_draw.size(), g->objects.size()); idx < count; ++idx) {
            if (!_uistate.shapes_draw[idx]) {
                continue;
            }
            const ObjectDrawData& dd = g->objects[idx];
            vkCmdDrawIndexed(render_event.frame_data->cmd_buf, dd.indices, 1, dd.index_offset, dd.vertex_offset, 0);
        }
    }
}
