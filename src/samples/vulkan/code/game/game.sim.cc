#include "game.sim.hpp"

#include <array>
#include <algorithm>

#include <concurrencpp/concurrencpp.h>
#include <Lz/Lz.hpp>
#include <imgui/imgui.h>
#include <imgui/IconsFontAwesome.h>

#include <tracy/Tracy.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Real.h>

#include "xray/base/xray.misc.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/xray.fmt.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/scene/scene.definition.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/math.units.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar3_string_cast.hpp"

#include "bindless.pipeline.config.hpp"
#include "events.hpp"
#include "system.physics.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;
using namespace xray::math;
using namespace xray::scene;

B5::GameSimulation::SimState::SimState(const InitContext& init_context)
    : arcball_cam{ xray::math::vec3f::stdc::zero, 1.0f, { init_context.surface_width, init_context.surface_height } }
{
    const auto rads = RadiansF32{ 0.123f };
    const auto rads2 = rads + RadiansF32{ 1.2f };
    const auto perspective_projection = perspective_symmetric(static_cast<float>(init_context.surface_width) /
                                                                  static_cast<float>(init_context.surface_height),
                                                              static_cast<RadiansF32>(DegreesF32{ 65.0f }),
                                                              0.1f,
                                                              1000.0f);

    camera.set_projection(perspective_projection);
}

B5::GameSimulation::GameSimulation(PrivateConstructionToken,
                                   const InitContext& init_context,
                                   xray::base::unique_pointer<PhysicsSystem> physics)
    : _simstate{ init_context }
    , _physics{ std::move(physics) }
{
    _timer.start();

    auto sa23geom = ranges::find_if(
        init_context.scene_def->gltf.entries,
        [id = GeometryHandleType{ FNV::fnv1a("sa23") }](const GltfGeometryEntry& g) { return g.hashed_name == id; });
    assert(sa23geom != ranges::cend(init_context.scene_def->gltf.entries));
    _starfury.geometry = ranges::distance(ranges::cbegin(init_context.scene_def->gltf.entries), sa23geom);

    auto sa23ent =
        ranges::find_if(init_context.scene_def->entities,
                        [id = FNV::fnv1a("main_ship")](EntityDrawableComponent& e) { return e.hashed_name == id; });
    _starfury.entity = ranges::distance(ranges::cbegin(init_context.scene_def->entities), sa23ent);
    assert(sa23ent != ranges::cend(init_context.scene_def->entities));
    const vec3f half_exts = sa23geom->bounding_box.extents();

    XR_LOG_INFO("Half extents {}", half_exts);

    using namespace JPH::literals;
    using namespace JPH;

    auto phys = _physics->physics();
    JPH::BodyInterface* body_ifc = &phys->GetBodyInterface();

    BoxShapeSettings box_shape{ Vec3{ half_exts.x, half_exts.y, half_exts.z } };
    box_shape.SetEmbedded();

    BodyCreationSettings body_settings{
        &box_shape,
        RVec3{ sa23ent->orientation.origin.x, sa23ent->orientation.origin.y, sa23ent->orientation.origin.z },
        Quat{
            sa23ent->orientation.rotation.x,
            sa23ent->orientation.rotation.y,
            sa23ent->orientation.rotation.z,
            sa23ent->orientation.rotation.w,
        }
            .Normalized(),
        EMotionType::Dynamic,
        B5::PhysicsSystem::ObjectLayers::MOVING
    };
    body_settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
    body_settings.mMassPropertiesOverride.mMass = 48000.0f;

    JPH::Body* body = body_ifc->CreateBody(body_settings);
    assert(body != nullptr);
    _starfury.phys_body_id = body->GetID();
    body_ifc->AddBody(_starfury.phys_body_id, EActivation::Activate);
}

B5::GameSimulation::~GameSimulation() {}

xray::base::unique_pointer<B5::GameSimulation>
B5::GameSimulation::create(const InitContext& init_ctx)
{
    auto physics_system = PhysicsSystem::create();
    if (!physics_system)
        return nullptr;

    return xray::base::make_unique<GameSimulation>(
        PrivateConstructionToken{}, init_ctx, xray::base::make_unique<PhysicsSystem>(std::move(*physics_system)));
}

void
B5::GameSimulation::event_handler(const xray::ui::window_event& evt)
{
    if (is_input_event(evt)) {
        if (evt.event.key.keycode == KeySymbol::escape) {
            return;
        }
    }

    if (evt.type == xray::ui::event_type::configure) {
        const xray::ui::window_configure_event* wce = &evt.event.configure;
        if (wce->width != 0 && wce->height != 0) {
            const auto perspective_projection =
                perspective_symmetric(static_cast<float>(wce->width) / static_cast<float>(wce->height),
                                      RadiansF32(radians(65.0f)),
                                      0.1f,
                                      1000.0f);
            _simstate.camera.set_projection(perspective_projection);
        }
    }

    if (_uistate.use_arcball_cam) {
        _simstate.arcball_cam.input_event(evt);
    } else {
    }
}

void
B5::GameSimulation::user_interface(xray::ui::user_interface* ui, const RenderEvent& re)
{
    ZoneScopedNCS("UI", tracy::Color::GreenYellow, 16);

    char scratch_buff[1024];

    if (ImGui::Begin("Demo options")) {

        ImGui::Checkbox("Draw world coordinate axis", &_uistate.draw_world_axis);
        format_to_n(scratch_buff, "Use arcball {}", fonts::awesome::ICON_FA_CAMERA);
        ImGui::Checkbox(scratch_buff, &_uistate.use_arcball_cam);
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
                ImGui::GetWindowDrawList()->AddRectFilled(
                    cursor, ImVec2{ cursor.x + sz, cursor.y + sz }, static_cast<uint32_t>(rgb_color{ colors[i] }));
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
                char scratch_buff[512];

                for (size_t idx = 0, count = std::min(lights.size(), dbg_state.size()); idx < count; ++idx) {
                    const LightType* l = &lights[idx];

                    ImGui::PushID(static_cast<const void*>(l));
                    format_to_n(scratch_buff, "Light [{}] #{:2}", tag, idx);

                    if (ImGui::TreeNode(scratch_buff)) {

                        bool dbg_draw = dbg_state[idx];
                        ImGui::Checkbox("draw", &dbg_draw);
                        ImGui::SameLine();
                        dbg_state[idx] = dbg_draw;

                        bool toggle = toggle_state[idx];
                        ImGui::Checkbox("enabled", &toggle);
                        toggle_state[idx] = toggle;

                        display_light_stats_fn(*l);

                        ImGui::TreePop();
                        ImGui::Spacing();
                    }

                    ImGui::PopID();
                }
            };

        auto display_directional_light_fn = [&](const DirectionalLight& dl) {
            ImGui::Text("Dir: [%3.3f, %3.3f, %3.3f]", dl.direction.x, dl.direction.y, dl.direction.z);
            draw_light_colors_fn(dl.ambient, dl.diffuse, dl.specular);
        };

        format_to_n(scratch_buff, "Directional {}", fonts::awesome::ICON_FA_LIGHTBULB_O);
        ImGui::SeparatorText(scratch_buff);
        display_lights_fn(std::span<const DirectionalLight>{ re.sdef->directional_lights },
                          fonts::awesome::ICON_FA_ARROW_RIGHT,
                          _uistate.dbg_directional_lights,
                          _uistate.toggle_directional_lights,
                          display_directional_light_fn);

        auto display_point_light_fn = [&](const PointLight& pl) {
            ImGui::Text("Pos: [%3.3f, %3.3f, %3.3f]", pl.position.x, pl.position.y, pl.position.z);
            draw_light_colors_fn(pl.ambient, pl.diffuse, pl.specular);
        };

        format_to_n(scratch_buff, "Point {}", fonts::awesome::ICON_FA_LIGHTBULB_O);
        ImGui::SeparatorText(scratch_buff);
        display_lights_fn(std::span<const PointLight>{ re.sdef->point_lights },
                          fonts::awesome::ICON_FA_ARROW_RIGHT,
                          _uistate.dbg_point_lights,
                          _uistate.toggle_point_lights,
                          display_point_light_fn);
    }
    ImGui::End();
}

void
B5::GameSimulation::loop_event(const RenderEvent& render_event)
{
    ZoneScopedNCS("scene loop", tracy::Color::Orange, 32);

    user_interface(render_event.ui, render_event);

    _physics->update();

    JPH::BodyInterface* bdi = &_physics->physics()->GetBodyInterface();
    const JPH::RVec3 pos =
        // JPH::Vec3::sZero();
        bdi->GetCenterOfMassPosition(_starfury.phys_body_id);
    const JPH::Quat rot =
        // JPH::Quat::sIdentity();
        bdi->GetRotation(_starfury.phys_body_id);

    if (_uistate.use_arcball_cam) {
        _simstate.arcball_cam.set_zoom_speed(4.0f * render_event.delta * 1.0e-3f);
        _simstate.arcball_cam.update_camera(_simstate.camera);
    } else {
        _simstate.flight_cam.update(rotation_matrix(quatf{ rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ() }),
                                    vec3f{ pos.GetY(), pos.GetY(), pos.GetZ() });
        _simstate.camera.set_view_matrix(_simstate.flight_cam.view_matrix, _simstate.flight_cam.inverse_of_view_matrix);
    }

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
        ZoneScopedNC("Lights setup", tracy::Color::GreenYellow);

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

        auto copy_lights_to_gpu_fn =
            [&]<typename LightType, typename LightFN>(std::span<const LightType> lights,
                                                      std::bitset<UIState::MAX_LIGHTS>& toggle_bits,
                                                      BindlessStorageBufferResourceHandleEntryPair sbo_gpu,
                                                      LightFN light_fn) {
                auto buffer_mem = sbo_gpu.second.memory;
                auto chunk_size = sbo_gpu.second.aligned_chunk_size;

                UniqueMemoryMapping::map_memory(
                    render_event.renderer->device(), buffer_mem, render_event.frame_data->id * chunk_size, chunk_size)
                    .map([&](UniqueMemoryMapping gpu_map) {
                        LightType* gpu_ptr = gpu_map.as<LightType>();
                        for (size_t idx = 0; idx < toggle_bits.size(); ++idx) {
                            if (!toggle_bits[idx])
                                continue;

                            *gpu_ptr++ = light_fn(lights[idx]);
                        }
                    });
            };

        if (_uistate.toggle_directional_lights.any()) {
            copy_lights_to_gpu_fn(std::span{ sdef->directional_lights },
                                  _uistate.toggle_directional_lights,
                                  sres->sbo_directional_lights,
                                  [c = &_simstate.camera](const DirectionalLight& dl) {
                                      DirectionalLight result{ dl };
                                      result.direction = normalize(mul_vec(c->view(), dl.direction));
                                      return result;
                                  });
        }

        if (_uistate.toggle_point_lights.any()) {
            copy_lights_to_gpu_fn(std::span{ sdef->point_lights },
                                  _uistate.toggle_point_lights,
                                  sres->sbo_point_lights,
                                  [c = &_simstate.camera](const PointLight& pl) {
                                      PointLight result{ pl };
                                      result.position = mul_point(c->view(), pl.position);
                                      return result;
                                  });
        }

        FrameGlobalData* fgd = render_event.g_ubo_data;
        SimState* s = &_simstate;
        fgd->world_view_proj = s->camera.projection_view(); // identity for model -> world
        fgd->view = s->camera.view();
        fgd->eye_pos = s->camera.origin();
        fgd->projection = s->camera.projection();

        //
        // lights setup
        fgd->lights = LightingSetup{
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

    // render_event.renderer->clear_attachments(render_event.frame_data->cmd_buf, 0.0f, 0.0f, 0.0f);
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

    InstanceRenderInfo* iri = instances_buffer->as<InstanceRenderInfo>();
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
        ZoneScopedNC("Render GLTF models", tracy::Color::Red);

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
        ZoneScopedNC("Render procedural geometry", tracy::Color::Yellow);

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
}
