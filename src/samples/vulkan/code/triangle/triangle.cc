#include "triangle.hpp"

#include <array>
#include <algorithm>

#include <concurrencpp/concurrencpp.h>
#include <Lz/Lz.hpp>
#include <imgui/imgui.h>

#include "xray/base/xray.misc.hpp"
#include "xray/base/small.vector.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.queue.submit.token.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/scene/scene.definition.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"

#include "bindless.pipeline.config.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;
using namespace xray::math;
using namespace xray::scene;

dvk::TriangleDemo::SimState::SimState(const app::init_context_t& init_context)
    : arcball_cam{ xray::math::vec3f::stdc::zero, 1.0f, { init_context.surface_width, init_context.surface_height } }
{
    //
    // projection
    const auto perspective_projection = perspective_symmetric(
        (float)init_context.surface_width / (float)init_context.surface_height, radians(65.0f), 0.1f, 1000.0f);

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

dvk::TriangleDemo::TriangleDemo(PrivateConstructionToken, const app::init_context_t& init_context)
    : app::DemoBase{ init_context }
    , _simstate{ init_context }
{
    _timer.start();
}

dvk::TriangleDemo::~TriangleDemo() {}

xray::base::unique_pointer<app::DemoBase>
dvk::TriangleDemo::create(const app::init_context_t& init_ctx)
{
    return xray::base::make_unique<TriangleDemo>(PrivateConstructionToken{}, init_ctx);
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

        // ImGui::SeparatorText("Geometric shapes");
        // for (size_t i = 0, count = std::min(_renderstate._shapes.objects.size(), _uistate.shapes_draw.size());
        //      i < count;
        //      ++i) {
        //     bool value = _uistate.shapes_draw[i];
        //
        //     auto out = fmt::format_to_n(txt_scratch_buff, size(txt_scratch_buff) - 1, "Object #{}", i);
        //     *out.out = 0;
        //
        //     ImGui::Checkbox(txt_scratch_buff, &value);
        //     _uistate.shapes_draw[i] = value;
        // }
        //
        // ImGui::SeparatorText("Geometry");
        // if (ImGui::CollapsingHeader("Nodes")) {
        //
        //     for (const GeometryNode& node : _world->geometry.nodes) {
        //         auto result = fmt::format_to_n(txt_scratch_buff,
        //                                        size(txt_scratch_buff),
        //                                        "{} ({} vertices, {} indices)",
        //                                        node.name,
        //                                        node.vertex_count,
        //                                        node.index_count);
        //         *result.out = 0;
        //         ImGui::Text("%s", txt_scratch_buff);
        //     }
        // }
    }
    ImGui::End();
}

void
dvk::TriangleDemo::loop_event(const app::RenderEvent& render_event)
{
    user_interface(render_event.ui);
    _simstate.arcball_cam.set_zoom_speed(4.0f * render_event.delta * 1.0e-3f);
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
        // render_event.dbg_draw->draw_sphere(_world->geometry.bounding_sphere.center,
        //                                    _world->geometry.bounding_sphere.radius,
        //                                    color_palette::material::orange);
    }

    if (_uistate.draw_nodes_spheres) {
        // for (const GeometryNode& node : _world->geometry.nodes) {
        //     if (node.index_count != 0) {
        //         render_event.dbg_draw->draw_sphere(
        //             node.bounding_sphere.center, node.bounding_sphere.radius, color_palette::material::orange);
        //     }
        // }
    }

    if (_uistate.draw_bbox) {
        // render_event.dbg_draw->draw_axis_aligned_box(
        //     _world->geometry.boundingbox.min, _world->geometry.boundingbox.max, color_palette::material::yellow50);
    }

    if (_uistate.draw_nodes_bbox) {
        // for (const GeometryNode& node : _world->geometry.nodes) {
        //     if (node.index_count != 0) {
        //         render_event.dbg_draw->draw_axis_aligned_box(
        //             node.boundingbox.min, node.boundingbox.max, color_palette::material::yellow900);
        //     }
        // }
    }

    const SceneDefinition* sdef = render_event.sdef;
    const SceneResources* sres = render_event.sres;

    {
        render_event.renderer->dbg_marker_begin(
            render_event.frame_data->cmd_buf, "Update UBO/SBO", color_palette::web::orange_red);

        app::FrameGlobalData* fgd = render_event.g_ubo_data;
        SimState* s = &_simstate;
        fgd->world_view_proj = s->camera.projection_view(); // identity for model -> world
        fgd->view = s->camera.view();
        fgd->eye_pos = s->camera.origin();
        fgd->projection = s->camera.projection();
        fgd->lights = app::LightingSetup{
            .sbo_directional_lights = destructure_bindless_resource_handle(
                                          bindless_subresource_handle_from_bindless_resource_handle(
                                              sres->sbo_directional_lights.first, render_event.frame_data->id))
                                          .first,
            .directional_lights_count = static_cast<uint32_t>(sdef->directional_lights.size()),
            .sbo_point_ligths =
                destructure_bindless_resource_handle(bindless_subresource_handle_from_bindless_resource_handle(
                                                         sres->sbo_point_lights.first, render_event.frame_data->id))
                    .first,
            .point_lights_count = static_cast<uint32_t>(sdef->point_lights.size()),
            .sbo_spot_ligths =
                destructure_bindless_resource_handle(bindless_subresource_handle_from_bindless_resource_handle(
                                                         sres->sbo_spot_lights.first, render_event.frame_data->id))
                    .first,
            .spot_lights_count = static_cast<uint32_t>(sdef->spot_lights.size()),
        };
        const uint32_t color_tex_handle = destructure_bindless_resource_handle(sres->color_tex.first).first;
        fgd->global_color_texture = color_tex_handle;

        const std::tuple<VkDeviceMemory, VkDeviceSize, std::span<const uint8_t>> light_sbos_data[] = {
            {
                sres->sbo_directional_lights.second.memory,
                sres->sbo_directional_lights.second.aligned_chunk_size,
                to_bytes_span(sdef->directional_lights),
            },
            {
                sres->sbo_point_lights.second.memory,
                sres->sbo_point_lights.second.aligned_chunk_size,
                to_bytes_span(sdef->point_lights),
            },
            {
                sres->sbo_spot_lights.second.memory,
                sres->sbo_spot_lights.second.aligned_chunk_size,
                to_bytes_span(sdef->spot_lights),
            },
        };

        for (const auto [buffer_mem, chunk_size, cpu_data] : light_sbos_data) {
            if (cpu_data.empty())
                continue;

            UniqueMemoryMapping::map_memory(
                render_event.renderer->device(), buffer_mem, render_event.frame_data->id * chunk_size, chunk_size)
                .map([=](UniqueMemoryMapping gpu_map) {
                    memcpy(gpu_map._mapped_memory, cpu_data.data(), cpu_data.size_bytes());
                });
        }

        render_event.renderer->dbg_marker_end(render_event.frame_data->cmd_buf);
    }

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

    struct DrawDataPushConst
    {
        union
        {
            uint32_t value;
            struct
            {
                uint32_t frame_id : 8;
                uint32_t instance_entry : 8;
                uint32_t instance_buffer_id : 16;
            };
        };

        DrawDataPushConst(const BindlessResourceHandle_StorageBuffer sbo,
                          const uint32_t instance_entry,
                          const uint32_t frame) noexcept
        {
            this->instance_buffer_id = destructure_bindless_resource_handle(sbo).first;
            this->instance_entry = instance_entry;
            this->frame_id = frame;
        }
    };

    auto instances_buffer =
        UniqueMemoryMapping::map_memory(render_event.renderer->device(),
                                        sres->sbo_instances.second.memory,
                                        sres->sbo_instances.second.aligned_chunk_size * render_event.frame_data->id,
                                        sres->sbo_instances.second.aligned_chunk_size);

    app::InstanceRenderInfo* iri = instances_buffer->as<app::InstanceRenderInfo>();
    uint32_t instance{};

    //
    // partition into gltf/nongltf

    using SmallVecType = SmallVec<const EntityDrawableComponent*, sizeof(void*) * 128>;
    SmallVecType::allocator_type::arena_type arena{};
    SmallVecType entities{ arena };
    lz::chain(sdef->entities).map([](const EntityDrawableComponent& e) { return &e; }).copyTo(back_inserter(entities));

    SmallVecType gltf_entities{ arena };
    SmallVecType procedural_entities{ arena };
    auto [last, out0, out1] =
        ranges::partition_copy(entities,
                               back_inserter(gltf_entities),
                               back_inserter(procedural_entities),
                               [](const EntityDrawableComponent* edc) { return !edc->material_id; });

    {
        render_event.renderer->dbg_marker_insert(
            render_event.frame_data->cmd_buf, "Rendering GLTF models", color_palette::web::sea_green);

        vkCmdBindPipeline(
            render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sres->pipelines.p_pbr_color.handle());

        const VkDeviceSize offsets[] = { 0 };
        const VkBuffer vertex_buffers[] = { sdef->gltf.vertex_buffer.buffer_handle() };
        vkCmdBindVertexBuffers(render_event.frame_data->cmd_buf, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(
            render_event.frame_data->cmd_buf, sdef->gltf.index_buffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);

        for (const EntityDrawableComponent* itr : gltf_entities) {
            const EntityDrawableComponent& edc = *itr;
            const auto itr_geom =
                ranges::find_if(sdef->gltf.entries,
                                [id = edc.geometry_id](const GltfGeometryEntry& ge) { return ge.hashed_name == id; });

            assert(itr_geom != cend(sdef->gltf.entries));

            iri->model = R4::translate(edc.orientation.origin) * rotation_matrix(edc.orientation.rotation) *
                         R4::scaling(edc.orientation.scale);
            iri->mtl_buffer = destructure_bindless_resource_handle(sres->sbo_pbr_materials.first).first;

            const DrawDataPushConst push_const = DrawDataPushConst{
                bindless_subresource_handle_from_bindless_resource_handle(sres->sbo_instances.first,
                                                                          render_event.frame_data->id),
                instance,
                render_event.frame_data->id,
            };

            vkCmdPushConstants(render_event.frame_data->cmd_buf,
                               sdef->pipelines.p_pbr_color.layout(),
                               VK_SHADER_STAGE_ALL,
                               0,
                               static_cast<uint32_t>(sizeof(push_const.value)),
                               &push_const.value);
            vkCmdDrawIndexed(render_event.frame_data->cmd_buf,
                             itr_geom->vertex_index_count.y,
                             1,
                             itr_geom->buffer_offsets.y,
                             static_cast<int32_t>(itr_geom->buffer_offsets.x),
                             0);

            ++iri;
            ++instance;
        }
        render_event.renderer->dbg_marker_end(render_event.frame_data->cmd_buf);
    }

    auto r0 = ranges::partition(procedural_entities, [](const EntityDrawableComponent* edc) {
        return swl::holds_alternative<ColorMaterialType>(*edc->material_id);
    });

    {
        render_event.renderer->dbg_marker_insert(
            render_event.frame_data->cmd_buf, "Rendering procedural models", color_palette::web::sea_green);

        auto draw_entities_fn = [&](const span<const EntityDrawableComponent*> ents) {
            for (const EntityDrawableComponent* e : ents) {
                const EntityDrawableComponent& edc = *e;

                iri->model = R4::translate(edc.orientation.origin) * rotation_matrix(edc.orientation.rotation) *
                             R4::scaling(edc.orientation.scale);

                if (swl::holds_alternative<TexturedMaterialType>(*e->material_id)) {
                    const TexturedMaterialType tex_mtl = swl::get<TexturedMaterialType>(*e->material_id);
                    auto itr_mtl = ranges::find_if(
                        sdef->materials_nongltf.materials_textured,
                        [id = tex_mtl.value_of()](const TexturedMaterial& tm) { return id == tm.hashed_name; });

                    iri->mtl_buffer_elem =
                        itr_mtl == ranges::end(sdef->materials_nongltf.materials_textured)
                            ? 0
                            : ranges::distance(ranges::begin(sdef->materials_nongltf.materials_textured), itr_mtl);
                    iri->mtl_buffer = destructure_bindless_resource_handle(sres->sbo_texture_materials.first).first;
                } else {
                    iri->mtl_buffer = destructure_bindless_resource_handle(sres->sbo_color_materials.first).first;
                    iri->mtl_buffer_elem = 0; // always 0 for color materials
                }

                const auto itr_geometry = ranges::find_if(
                    sdef->procedural.procedural_geometries,
                    [gid = edc.geometry_id](const ProceduralGeometryEntry& pge) { return pge.hashed_name == gid; });
                assert(itr_geometry != cend(sdef->procedural.procedural_geometries));

                const DrawDataPushConst push_const = DrawDataPushConst{
                    bindless_subresource_handle_from_bindless_resource_handle(sres->sbo_instances.first,
                                                                              render_event.frame_data->id),
                    instance,
                    render_event.frame_data->id,
                };

                vkCmdPushConstants(render_event.frame_data->cmd_buf,
                                   sres->pipelines.p_ads_color.layout(),
                                   VK_SHADER_STAGE_ALL,
                                   0,
                                   sizeof(push_const).value,
                                   &push_const.value);

                vkCmdDrawIndexed(render_event.frame_data->cmd_buf,
                                 itr_geometry->vertex_index_count.y,
                                 1,
                                 itr_geometry->buffer_offsets.y,
                                 static_cast<int32_t>(itr_geometry->buffer_offsets.x),
                                 0);

                ++instance;
                ++iri;
            }
        };

        const VkDeviceSize offsets[] = { 0 };
        const VkBuffer vertex_buffers[] = { sdef->procedural.vertex_buffer.buffer_handle() };
        vkCmdBindVertexBuffers(
            render_event.frame_data->cmd_buf, 0, static_cast<uint32_t>(size(vertex_buffers)), vertex_buffers, offsets);
        vkCmdBindIndexBuffer(
            render_event.frame_data->cmd_buf, sdef->procedural.index_buffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindPipeline(
            render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sres->pipelines.p_ads_color.handle());

        draw_entities_fn(span{ ranges::begin(procedural_entities), ranges::begin(r0) });
        // TODO: support for textured materials

        vkCmdBindPipeline(
            render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sres->pipelines.p_ads_textured.handle());
        draw_entities_fn(span{ r0 });

        render_event.renderer->dbg_marker_end(render_event.frame_data->cmd_buf);
    }

    if (_uistate.draw_ship) {
        // vkCmdBindPipeline(
        //     render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderstate.pipeline.handle());
        //
        // const uint32_t push_const{ bindless_subresource_handle_from_bindless_resource_handle(
        //                                _renderstate.g_instancebuffer.first, render_event.frame_data->id)
        //                                    .value_of()
        //                                << 16 |
        //                            render_event.frame_data->id };
        // vkCmdPushConstants(render_event.frame_data->cmd_buf,
        //                    _renderstate.pipeline.layout(),
        //                    VK_SHADER_STAGE_ALL,
        //                    0,
        //                    static_cast<uint32_t>(sizeof(render_event.frame_data->id)),
        //                    &push_const);
        //
        // const VkDeviceSize vtx_offsets[] = { 0 };
        // const VkBuffer vertex_buffers[] = { _renderstate.g_vertexbuffer.buffer_handle() };
        // vkCmdBindVertexBuffers(render_event.frame_data->cmd_buf, 0, 1, vertex_buffers, vtx_offsets);
        // vkCmdBindIndexBuffer(
        //     render_event.frame_data->cmd_buf, _renderstate.g_indexbuffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);
        //
        // vkCmdDrawIndexed(render_event.frame_data->cmd_buf, _world->geometry.vertex_index_counts.y, 1, 0, 0, 0);
    }

    //
    // geometry objects
    if (_uistate.shapes_draw.count() != 0) {
        //     const RenderState::GeometricShapes* g = &_renderstate._shapes;
        //     vkCmdBindPipeline(render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
        //     g->pipeline.handle());
        //
        //     const VkBuffer vtx_buffers[] = { g->vertexbuffer.buffer_handle() };
        //     const VkDeviceSize vtx_offsets[] = { 0 };
        //     vkCmdBindVertexBuffers(render_event.frame_data->cmd_buf, 0, 1, vtx_buffers, vtx_offsets);
        //     vkCmdBindIndexBuffer(render_event.frame_data->cmd_buf, g->indexbuffer.buffer_handle(), 0,
        //     VK_INDEX_TYPE_UINT32);
        //
        //     const uint32_t push_const{ render_event.frame_data->id };
        //
        //     vkCmdPushConstants(render_event.frame_data->cmd_buf,
        //                        g->pipeline.layout(),
        //                        VK_SHADER_STAGE_ALL,
        //                        0,
        //                        static_cast<uint32_t>(sizeof(render_event.frame_data->id)),
        //                        &push_const);
        //
        //     for (size_t idx = 0, count = std::min(_uistate.shapes_draw.size(), g->objects.size()); idx < count;
        //     ++idx) {
        //         if (!_uistate.shapes_draw[idx]) {
        //             continue;
        //         }
        //         const ObjectDrawData& dd = g->objects[idx];
        //         vkCmdDrawIndexed(render_event.frame_data->cmd_buf, dd.indices, 1, dd.index_offset, dd.vertex_offset,
        //         0);
        //     }
    }
}
