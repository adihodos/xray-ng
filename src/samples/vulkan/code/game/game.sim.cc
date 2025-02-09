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
#include "xray/math/transforms_r4.hpp"
#include "xray/math/objects/aabb3_math.hpp"

#include "bindless.pipeline.config.hpp"
#include "events.hpp"
#include "system.physics.hpp"
#include "push.constant.packer.hpp"

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
                                   xray::base::unique_pointer<PhysicsSystem> physics,
                                   std::span<std::byte> arena_perm,
                                   std::span<std::byte> arena_temp)
    : _simstate{ init_context }
    , _physics{ std::move(physics) }
    , _arena_perm{ arena_perm }
    , _arena_temp{ arena_temp }
    , _world{ _arena_perm }
{
    _timer.start();

    ranges::copy_if(init_context.scene_def->entities,
                    back_inserter(_world.ent_gltf),
                    [](const EntityDrawableComponent& e) { return !e.material_id; });
    ranges::copy_if(init_context.scene_def->entities,
                    back_inserter(_world.ent_basic),
                    [](const EntityDrawableComponent& e) { return e.material_id.has_value(); });

    auto sa23geom = ranges::find_if(
        init_context.scene_def->gltf.entries,
        [id = GeometryHandleType{ FNV::fnv1a("sa23") }](const GltfGeometryEntry& g) { return g.hashed_name == id; });
    assert(sa23geom != ranges::cend(init_context.scene_def->gltf.entries));
    _world.ent_player.geometry = ranges::distance(ranges::cbegin(init_context.scene_def->gltf.entries), sa23geom);

    _world.ent_player.entity = 0;
    auto sa23ent = &_world.ent_gltf[0];

    const aabb3f box = xray::math::transform(R4::scaling(sa23ent->orientation.scale), sa23geom->bounding_box);
    const vec3f half_exts = box.extents();

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
    _world.ent_player.phys_body_id = body->GetID();
    body_ifc->AddBody(_world.ent_player.phys_body_id, EActivation::Activate);
}

B5::GameSimulation::~GameSimulation() {}

xray::base::unique_pointer<B5::GameSimulation>
B5::GameSimulation::create(const InitContext& init_ctx)
{
    auto physics_system = PhysicsSystem::create(init_ctx);
    if (!physics_system)
        return nullptr;

    std::span<std::byte> arena_perm = os_virtual_alloc(64 * 1024 * 1024);
    if (arena_perm.empty())
        return nullptr;
    std::span<std::byte> arena_temp = os_virtual_alloc(32 * 1024 * 1024);
    if (arena_temp.empty())
        return nullptr;

    return xray::base::make_unique<GameSimulation>(PrivateConstructionToken{},
                                                   init_ctx,
                                                   xray::base::make_unique<PhysicsSystem>(std::move(*physics_system)),
                                                   arena_perm,
                                                   arena_temp);
}

void
B5::GameSimulation::event_handler(const xray::ui::window_event& evt)
{
    if (is_input_event(evt)) {
        if (evt.event.key.keycode == KeySymbol::escape) {
            return;
        }

        if (evt.type == event_type::key) {
            const key_event* ke = &evt.event.key;
            using xray::ui::KeySymbol;
            using namespace xray::math;

            float force{};
            RadiansF32 roll{};
            RadiansF32 yaw{};
            switch (ke->keycode) {
                case KeySymbol::key_w:
                    force = +1.0f;
                    break;

                case KeySymbol::key_s:
                    force = -1.0f;
                    break;

                case KeySymbol::key_q:
                    roll += 15.0_DEG2RADF32;
                    break;

                case KeySymbol::key_e:
                    roll -= 15.0_DEG2RADF32;
                    break;

                case KeySymbol::left:
                    yaw -= 15.0_DEG2RADF32;
                    break;

                case KeySymbol::right:
                    yaw += 15.0_DEG2RADF32;
                    break;

                case KeySymbol::backspace: {
                    JPH::BodyInterface* ifc = &_physics->physics()->GetBodyInterface();
                    ifc->SetPositionRotationAndVelocity(_world.ent_player.phys_body_id,
                                                        JPH::Vec3::sZero(),
                                                        JPH::Quat::sIdentity(),
                                                        JPH::Vec3::sZero(),
                                                        JPH::Vec3::sZero());
                } break;

                default:
                    break;
            }

            constexpr float thruster_force{ 4500.0f };
            if (!is_zero(force)) {
                JPH::BodyInterface* ifc = &_physics->physics()->GetBodyInterface();
                JPH::RVec3 pos;
                JPH::Quat rot;
                ifc->GetPositionAndRotation(_world.ent_player.phys_body_id, pos, rot);
                const JPH::Vec3 applied_force = rot * (JPH::Vec3::sAxisZ() * force * thruster_force);
                ifc->AddForce(_world.ent_player.phys_body_id, applied_force);
                // XR_LOG_INFO("Applying force ({},{},{})", applied_force.GetX(), applied_force.GetY(),
                // applied_force.GetZ());
            }

            if (!is_zero(roll.value_of())) {
                JPH::BodyInterface* ifc = &_physics->physics()->GetBodyInterface();

                const JPH::Vec3 torque =
                    JPH::Quat::sRotation(JPH::Vec3::sAxisZ(), roll.value_of()) * JPH::Vec3::sAxisZ() * thruster_force;
                ifc->AddTorque(_world.ent_player.phys_body_id, torque);
            }

            if (!is_zero(yaw.value_of())) {
                JPH::BodyInterface* ifc = &_physics->physics()->GetBodyInterface();

                const JPH::Vec3 torque =
                    JPH::Quat::sRotation(JPH::Vec3::sAxisY(), roll.value_of()) * JPH::Vec3::sAxisY() * thruster_force;
                ifc->AddTorque(_world.ent_player.phys_body_id, torque);
            }
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

        if (ImGui::CollapsingHeader("::: Physics engine debug draw :::")) {
            ImGui::Checkbox("Draw the shapes of all bodies", &_uistate.phys_draw.mDrawShape);
            ImGui::Checkbox("Draw a bounding box per body", &_uistate.phys_draw.mDrawBoundingBox);
            ImGui::Checkbox("Draw the center of mass for each body", &_uistate.phys_draw.mDrawCenterOfMassTransform);
            ImGui::Checkbox("Draw the world transform for each body", &_uistate.phys_draw.mDrawWorldTransform);
            ImGui::Checkbox("Draw the mass and inertia (as the box equivalent) for each body",
                            &_uistate.phys_draw.mDrawMassAndInertia);
            ImGui::Checkbox("Draw shapes in wireframe instead of solid", &_uistate.phys_draw.mDrawShapeWireframe);
        }

        // ImGui::Checkbox("Draw individual nodes bounding boxes", &_uistate.draw_nodes_bbox);
        // ImGui::Checkbox("Draw bounding sphere", &_uistate.draw_sphere);
        // ImGui::Checkbox("Draw individual nodes bounding sphere", &_uistate.draw_nodes_spheres);

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

    ScratchPadArena scratch_pad{ &_arena_temp };

    JPH::BodyInterface* bdi = &_physics->physics()->GetBodyInterface();
    const JPH::RVec3 pos = bdi->GetCenterOfMassPosition(_world.ent_player.phys_body_id);
    const JPH::Quat rot = bdi->GetRotation(_world.ent_player.phys_body_id);

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

    const SceneDefinition* sdef = render_event.sdef;
    const SceneResources* sres = render_event.sres;

    {
        ZoneScopedNC("Lights setup", tracy::Color::GreenYellow);

        const auto vkmarker = render_event.renderer->dbg_marker_begin(
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

    const VkRect2D scissor{
        .offset = VkOffset2D{ 0, 0 },
        .extent = render_event.frame_data->fbsize,
    };
    vkCmdSetViewport(render_event.frame_data->cmd_buf, 0, 1, &viewport);
    vkCmdSetScissor(render_event.frame_data->cmd_buf, 0, 1, &scissor);

    auto instances_buffer =
        UniqueMemoryMapping::map_memory(render_event.renderer->device(),
                                        sres->sbo_instances.second.memory,
                                        sres->sbo_instances.second.aligned_chunk_size * render_event.frame_data->id,
                                        sres->sbo_instances.second.aligned_chunk_size);

    InstanceRenderInfo* iri = instances_buffer->as<InstanceRenderInfo>();
    uint32_t instance{};

    if (!_world.ent_gltf.empty()) {
        ZoneScopedNC("Render GLTF models", tracy::Color::Red);

        const auto vkmarker = render_event.renderer->dbg_marker_begin(
            render_event.frame_data->cmd_buf, "Rendering GLTF models", color_palette::web::sea_green);

        vkCmdBindPipeline(
            render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sres->pipelines.p_pbr_color.handle());

        const VkDeviceSize offsets[] = { 0 };
        const VkBuffer vertex_buffers[] = { sdef->gltf.vertex_buffer.buffer_handle() };
        vkCmdBindVertexBuffers(render_event.frame_data->cmd_buf, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(
            render_event.frame_data->cmd_buf, sdef->gltf.index_buffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);

        for (const EntityDrawableComponent& edc : _world.ent_gltf) {
            const auto itr_geom =
                ranges::find_if(sdef->gltf.entries,
                                [id = edc.geometry_id](const GltfGeometryEntry& ge) { return ge.hashed_name == id; });

            assert(itr_geom != cend(sdef->gltf.entries));

            //
            // TODO: need to sync with the physics
            _physics->physics()
                ->GetBodyInterface()
                .GetCenterOfMassTransform(_world.ent_player.phys_body_id)
                .Transposed()
                .StoreFloat4x4(reinterpret_cast<JPH::Float4*>(&iri->model.components));

            iri->model_view = _simstate.camera.view() * iri->model;
            iri->normals_view = iri->model_view; // we only have rotations and translations, no scaling
            iri->mtl_buffer = destructure_bindless_resource_handle(sres->sbo_pbr_materials.first).first;

            const PackedU32PushConstant push_const = PackedU32PushConstant{
                bindless_subresource_handle_from_bindless_resource_handle(sres->sbo_instances.first,
                                                                          render_event.frame_data->id),
                instance,
                render_event.frame_data->id,
            };

            vkCmdPushConstants(render_event.frame_data->cmd_buf,
                               sdef->pipelines.p_pbr_color.layout(),
                               VK_SHADER_STAGE_ALL,
                               0,
                               push_const.as_bytes().size(),
                               push_const.as_bytes().data());
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

    containers::vector<const EntityDrawableComponent*> ents_color_mtl{ scratch_pad };
    lz::chain(_world.ent_basic)
        .filterMap(
            [](const EntityDrawableComponent& e) { return swl::holds_alternative<ColorMaterialType>(*e.material_id); },
            [](const EntityDrawableComponent& e) { return &e; })
        .copyTo(back_inserter(ents_color_mtl));

    containers::vector<const EntityDrawableComponent*> ents_textured_mtl{ scratch_pad };
    lz::chain(_world.ent_basic)
        .filterMap(
            [](const EntityDrawableComponent& e) {
                return swl::holds_alternative<TexturedMaterialType>(*e.material_id);
            },
            [](const EntityDrawableComponent& e) { return &e; })
        .copyTo(back_inserter(ents_textured_mtl));

    {
        ZoneScopedNC("Render procedural geometry", tracy::Color::Yellow);

        const auto vkmarker = render_event.renderer->dbg_marker_begin(
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

                const PackedU32PushConstant push_const = PackedU32PushConstant{
                    bindless_subresource_handle_from_bindless_resource_handle(sres->sbo_instances.first,
                                                                              render_event.frame_data->id),
                    instance,
                    render_event.frame_data->id,
                };

                vkCmdPushConstants(render_event.frame_data->cmd_buf,
                                   sres->pipelines.p_ads_color.layout(),
                                   VK_SHADER_STAGE_ALL,
                                   0,
                                   push_const.as_bytes().size(),
                                   push_const.as_bytes().data());

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

        draw_entities_fn(ents_color_mtl);

        vkCmdBindPipeline(
            render_event.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, sres->pipelines.p_ads_textured.handle());
        draw_entities_fn(ents_textured_mtl);
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

    if (_uistate.draw_bbox) {
        const OrientationF32& orientation = _world.ent_gltf[_world.ent_player.entity].orientation;
        const mat4f xf =
            R4::translate(orientation.origin) * rotation_matrix(orientation.rotation) * R4::scaling(orientation.scale);
        const aabb3f bbox = xray::math::transform(xf, sdef->gltf.entries[_world.ent_player.geometry].bounding_box);
        render_event.dbg_draw->draw_axis_aligned_box(bbox.min, bbox.max, color_palette::material::cyan500);
    }

#if defined(JPH_DEBUG_RENDERER)
    const vec3f eye = _simstate.camera.origin();
    _physics->dbg_draw_render(render_event, JPH::RVec3{ eye.x, eye.y, eye.z }, _uistate.phys_draw);
#endif
}
