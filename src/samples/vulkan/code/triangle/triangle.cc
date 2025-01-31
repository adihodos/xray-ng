#include "triangle.hpp"

#include <array>
#include <algorithm>

#include <concurrencpp/concurrencpp.h>
#include <Lz/Lz.hpp>
#include <imgui/imgui.h>

#include "xray/base/xray.misc.hpp"
#include "xray/base/xray.fmt.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/containers/arena.string.hpp"
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
dvk::TriangleDemo::user_interface(xray::ui::user_interface* ui, const app::RenderEvent& re)
{
    char scratch_buff[1024];

    if (ImGui::Begin("Demo options")) {

        ImGui::Checkbox("Draw world coordinate axis", &_uistate.draw_world_axis);
        ImGui::Checkbox("Draw ship", &_uistate.draw_ship);
        ImGui::Checkbox("Draw bounding box", &_uistate.draw_bbox);
        ImGui::Checkbox("Draw individual nodes bounding boxes", &_uistate.draw_nodes_bbox);
        ImGui::Checkbox("Draw bounding sphere", &_uistate.draw_sphere);
        ImGui::Checkbox("Draw individual nodes bounding sphere", &_uistate.draw_nodes_spheres);

        auto draw_light_colors_fn = [](const vec4f& ka, const vec4f& kd, const vec4f& ks) {
            const vec4f colors[] = { ka, kd, ks };
            const char* names[] = { "Ka", "Kd", "Ks" };

            for (size_t i = 0; i < 3; ++i) {
                const float sz = ImGui::GetTextLineHeight();
                ImGui::Text("%s", names[i]);
                ImGui::SameLine();
                const ImVec2 cursor = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(cursor, ImVec2{ cursor.x + sz, cursor.y + sz }, 0xFF00FF00);
                ImGui::Dummy(ImVec2{ sz, sz });
                if (i < 2)
                    ImGui::SameLine();
            }
        };

        auto display_lights_fn =
            [&]<typename LightType, typename LightStatsFN>(std::span<const LightType> lights,
                                                           std::string_view tag,
                                                           std::bitset<UIState::MAX_LIGHTS>& dbg_state,
                                                           std::bitset<UIState::MAX_LIGHTS>& toggle_state,
                                                           LightStatsFN display_light_stats_fn) {
                char scratch_buff[1024];

                for (size_t idx = 0, count = std::min(lights.size(), dbg_state.size()); idx < count; ++idx) {
                    const LightType* l = &lights[idx];

                    format_to_n(scratch_buff, "Light [{}] #{:2}", tag, idx);

                    if (ImGui::TreeNode(scratch_buff)) {

                        format_to_n(scratch_buff, "{}light##{}", tag, idx);
                        ImGui::PushID(scratch_buff);

                        bool dbg_draw = dbg_state[idx];
                        ImGui::Checkbox("draw", &dbg_draw);
                        ImGui::SameLine();
                        dbg_state[idx] = dbg_draw;

                        bool toggle = toggle_state[idx];
                        ImGui::Checkbox("enabled", &toggle);
                        toggle_state[idx] = toggle;

                        display_light_stats_fn(*l);

                        ImGui::PopID();

                        ImGui::TreePop();
                        ImGui::Spacing();
                    }
                }
            };

        auto display_directional_light_fn = [&](const DirectionalLight& dl) {
            ImGui::Text("Dir: [%3.3f, %3.3f, %3.3f]", dl.direction.x, dl.direction.y, dl.direction.z);
            draw_light_colors_fn(dl.ambient, dl.diffuse, dl.specular);
        };

        ImGui::SeparatorText("Directional lights");
        display_lights_fn(std::span<const DirectionalLight>{ re.sdef->directional_lights },
                          "D",
                          _uistate.dbg_directional_lights,
                          _uistate.toggle_directional_lights,
                          display_directional_light_fn);

        auto display_point_light_fn = [&](const PointLight& pl) {
            ImGui::Text("Pos: [%3.3f, %3.3f, %3.3f]", pl.position.x, pl.position.y, pl.position.z);
            draw_light_colors_fn(pl.ambient, pl.diffuse, pl.specular);
        };

        ImGui::SeparatorText("Point lights");
        display_lights_fn(std::span<const PointLight>{ re.sdef->point_lights },
                          "P",
                          _uistate.dbg_point_lights,
                          _uistate.toggle_point_lights,
                          display_point_light_fn);

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
    user_interface(render_event.ui, render_event);
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

    ScratchPadArena scratch_pad{ render_event.arena_temp };

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

        for (size_t start = sdef->directional_lights.size(), max = _uistate.toggle_directional_lights.size();
             start < max;
             ++start) {
            _uistate.toggle_directional_lights[start] = false;
        }

        for (size_t start = sdef->point_lights.size(), max = _uistate.toggle_point_lights.size(); start < max;
             ++start) {
            _uistate.toggle_point_lights[start] = false;
        }

        auto copy_lights_to_gpu_fn = [&]<typename LightType>(std::span<const LightType> lights,
                                                             std::bitset<UIState::MAX_LIGHTS>& toggle_bits,
                                                             BindlessStorageBufferResourceHandleEntryPair sbo_gpu) {
            auto buffer_mem = sbo_gpu.second.memory;
            auto chunk_size = sbo_gpu.second.aligned_chunk_size;

            UniqueMemoryMapping::map_memory(
                render_event.renderer->device(), buffer_mem, render_event.frame_data->id * chunk_size, chunk_size)
                .map([&](UniqueMemoryMapping gpu_map) {
                    LightType* gpu_ptr = gpu_map.as<LightType>();
                    for (size_t idx = 0; idx < toggle_bits.size(); ++idx) {
                        if (!toggle_bits[idx])
                            continue;

                        *gpu_ptr++ = lights[idx];
                    }
                });
        };

        if (_uistate.toggle_directional_lights.any()) {
            copy_lights_to_gpu_fn(std::span{ sdef->directional_lights },
                                  _uistate.toggle_directional_lights,
                                  sres->sbo_directional_lights);
            // auto buffer_mem = sres->sbo_directional_lights.second.memory;
            // auto chunk_size = sres->sbo_directional_lights.second.aligned_chunk_size;
            // UniqueMemoryMapping::map_memory(
            //     render_event.renderer->device(), buffer_mem, render_event.frame_data->id * chunk_size, chunk_size)
            //     .map([&](UniqueMemoryMapping gpu_map) {
            //         for (size_t idx = 0; idx < _uistate.toggle_directional_lights.size(); ++idx) {
            //             if (!_uistate.toggle_directional_lights[idx])
            //                 continue;
            //
            //             gpu_map.as<DirectionalLight>()[idx] = sdef->directional_lights[idx];
            //         }
            //     });
        }

        if (_uistate.toggle_point_lights.any()) {
            copy_lights_to_gpu_fn(
                std::span{ sdef->point_lights }, _uistate.toggle_point_lights, sres->sbo_point_lights);

            // auto buffer_mem = sres->sbo_point_lights.second.memory;
            // auto chunk_size = sres->sbo_point_lights.second.aligned_chunk_size;
            // UniqueMemoryMapping::map_memory(
            //     render_event.renderer->device(), buffer_mem, render_event.frame_data->id * chunk_size, chunk_size)
            //     .map([&](UniqueMemoryMapping gpu_map) {
            //         for (size_t idx = 0; idx < _uistate.toggle_point_lights.size(); ++idx) {
            //             if (!_uistate.toggle_point_lights[idx])
            //                 continue;
            //
            //             gpu_map.as<PointLight>()[idx] = sdef->point_lights[idx];
            //         }
            //     });
        }

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
            .directional_lights_count = static_cast<uint32_t>(_uistate.toggle_directional_lights.count()),
            .sbo_point_ligths =
                destructure_bindless_resource_handle(bindless_subresource_handle_from_bindless_resource_handle(
                                                         sres->sbo_point_lights.first, render_event.frame_data->id))
                    .first,
            .point_lights_count = static_cast<uint32_t>(_uistate.toggle_point_lights.count()),
            .sbo_spot_ligths =
                destructure_bindless_resource_handle(bindless_subresource_handle_from_bindless_resource_handle(
                                                         sres->sbo_spot_lights.first, render_event.frame_data->id))
                    .first,
            .spot_lights_count = static_cast<uint32_t>(sdef->spot_lights.size()),
        };

        const uint32_t color_tex_handle = destructure_bindless_resource_handle(sres->color_tex.first).first;
        fgd->global_color_texture = color_tex_handle;

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

    containers::vector<const EntityDrawableComponent*> entities{ scratch_pad };
    lz::chain(sdef->entities).map([](const EntityDrawableComponent& e) { return &e; }).copyTo(back_inserter(entities));

    containers::vector<const EntityDrawableComponent*> gltf_entities{ scratch_pad };
    containers::vector<const EntityDrawableComponent*> procedural_entities{ scratch_pad };

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
            iri->model_view = _simstate.camera.view() * iri->model;
            iri->normals_view = iri->model_view; // we only have rotations and translations, no scaling
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
                iri->model_view = _simstate.camera.view() * iri->model;
                iri->normals_view = iri->model_view; // we only have rotations and translations, no scaling

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

    for (const auto& [idx, dir_light] : lz::enumerate(render_event.sdef->directional_lights)) {
        if (!_uistate.dbg_directional_lights[idx])
            continue;

        render_event.dbg_draw->draw_directional_light(dir_light.direction, 8.0f, rgb_color{ dir_light.diffuse });
    }

    for (const auto& [idx, point_light] : lz::enumerate(render_event.sdef->point_lights)) {
        if (!_uistate.dbg_point_lights[idx])
            continue;

        render_event.dbg_draw->draw_point_light(point_light.position, 1.0f, rgb_color{ point_light.diffuse });
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
