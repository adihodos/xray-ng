#include "game.sim.hpp"

#include <array>
#include <algorithm>

#include <rfl.hpp>
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

#include "xray/base/app_config.hpp"
#include "xray/base/xray.misc.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/xray.fmt.hpp"
#include "xray/base/variant.helpers.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/sprite.system/sprite.system.hpp"
#include "xray/scene/scene.definition.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/window.hpp"
#include "xray/ui/events.gamepad.hpp"
#include "xray/ui/events.pretty.print.hpp"
#include "init_context.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/math.units.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar2_string_cast.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/scalar4x4_string_cast.hpp"

#include "bindless.pipeline.config.hpp"
#include "events.hpp"
#include "system.physics.hpp"
#include "push.constant.packer.hpp"
#include "terrain.hpp"
#include "sprite.ids.crosshairs.hpp"
#include "hud.config.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;
using namespace xray::math;
using namespace xray::scene;

XR_DISABLE_OPTIMIZATIONS

B5::GameSimulation::SimState::SimState(const InitContext& init_context)
    : arcball_cam{ xray::math::vec3f::stdc::zero, 1.0f, { init_context.surface_width, init_context.surface_height } }
    , flightcam{
        .params = FlightCameraParams::from_file(init_context.config_sys->config_path("flight.camera.params.conf")),
    }
{
    const auto perspective_projection = perspective_symmetric(static_cast<float>(init_context.surface_width) /
                                                                  static_cast<float>(init_context.surface_height),
                                                              65.0_DEG2RADF32,
                                                              0.1f,
                                                              1000.0f);
    camera.set_projection(perspective_projection);
}

B5::GameSimulation::InputStateTracker::InputStateTracker(xray::base::MemoryArena* arena,
                                                         std::span<const xray::ui::GamepadAxisInfo> ai,
                                                         const vec2f32 scr_size)
    : last_axis_events{ *arena }
    , axis_info{ ai.begin(), ai.end(), *arena }
    , screen_size_inv{ scr_size }
{
    last_axis_events.reserve(rfl::get_underlying_enumerator_array<xray::ui::GamepadAxis>().size());
    for (const auto [e_name, e_value] : rfl::get_enumerator_array<xray::ui::GamepadAxis>()) {
        last_axis_events.push_back(GamepadAxisEvent{ .axis = e_value, .i32 = 0, .f32 = 0.0f, .timestamp = 0 });
    }

    assert(last_axis_events.size() >= axis_info.size());
}

B5::GameSimulation::GameSimulation(PrivateConstructionToken,
                                   const InitContext& init_context,
                                   xray::base::unique_pointer<PhysicsSystem> physics,
                                   Terrain terrain,
                                   std::span<std::byte> arena_perm,
                                   std::span<std::byte> arena_temp)
    : _simstate{ init_context }
    , _physics{ std::move(physics) }
    , _arena_perm{ arena_perm }
    , _arena_temp{ arena_temp }
    , _world{ _arena_perm }
    , _terrain{ xray::base::make_unique<Terrain>(_arena_perm, std::move(terrain)) }
    , _ui{ init_context.ui }
    , _inputstate{
        &_arena_perm,
        init_context.win->gamepad_axis_info(),
        vec2f32{
            1.0f / static_cast<float>(init_context.surface_width),
            1.0f / static_cast<float>(init_context.surface_height),
        },
    }
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

    auto phys = _physics->sim();
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
    _world.ent_player.phys_body = body;
    body_ifc->AddBody(_world.ent_player.phys_body_id, EActivation::Activate);
    _world.ent_player.data.linear_velocity = JPH::Vec3::sZero();
    _world.ent_player.data.angular_velocity = JPH::Vec3::sZero();
    _world.ent_player.data.position = _world.ent_player.phys_body->GetCenterOfMassPosition();
    _world.ent_player.data.throttle = 0.0f;
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

    auto terrain = Terrain::create(init_ctx);
    if (!terrain)
        return nullptr;

    return xray::base::make_unique<GameSimulation>(PrivateConstructionToken{},
                                                   init_ctx,
                                                   xray::base::make_unique<PhysicsSystem>(std::move(*physics_system)),
                                                   std::move(*terrain),
                                                   arena_perm,
                                                   arena_temp);
}

struct FlightModel
{
    float thruster_large{ 24000.0f };
    float thruster_small{ 12000.0f };
};

void
B5::GameSimulation::handle_gamepad_axis_event(const xray::ui::GamepadAxisEvent& e)
{
    assert(static_cast<size_t>(e.axis) < _inputstate.last_axis_events.size());
    // XR_LOG_INFO("Gamepad axis {}, {}, {}", e.axis, e.i32, e.f32);
    _inputstate.last_axis_events[static_cast<size_t>(e.axis)] = e;
}

void
B5::GameSimulation::handle_gamepad_button_event(const xray::ui::GamepadButtonEvent& e)
{
    XR_LOG_INFO("Button event {}, {}", e.button, e.i32);
}

void
B5::GameSimulation::handle_mouse_button_event(const xray::ui::mouse_button_event& mbe)
{

    if (mbe.type == event_action_type::press) {
        if (mbe.button == mouse_button::button3) {
            // XR_LOG_INFO("Mouse button {}, type {}, pos @ {}x{}", mbe.button, mbe.type, mbe.pointer_x, mbe.pointer_y);
            _inputstate.last_mouse_down = vec2f32{ mbe.pointer_x, mbe.pointer_y };
            _simstate.flightcam.pivot_mode = true;
        }
    } else {
        if (mbe.button == mouse_button::button3) {
            _inputstate.last_mouse_down = {};
            _simstate.flightcam.pivot_mode = false;
            _simstate.flightcam.look_input = vec2f32{ 0 };
        }
    }
}

void
B5::GameSimulation::handle_mouse_motion_event(const xray::ui::mouse_motion_event& mme)
{
    if (_inputstate.last_mouse_down) {
        const vec2f32 mouse_pos{ mme.pointer_x, mme.pointer_y };

        const vec2f32 initial_input = vec2f{
            xray::math::clamp(
                _inputstate.last_mouse_down->x * 2.0f * _inputstate.screen_size_inv.x - 1.0f, -1.0f, 1.0f),
            xray::math::clamp(
                1.0f - 2.0f * _inputstate.last_mouse_down->y * _inputstate.screen_size_inv.y, -1.0f, 1.0f),
        };

        const vec2f32 current_input = vec2f{
            xray::math::clamp(mouse_pos.x * 2.0f * _inputstate.screen_size_inv.x - 1.0f, -1.0f, 1.0f),
            xray::math::clamp(1.0f - 2.0f * mouse_pos.y * _inputstate.screen_size_inv.y, -1.0f, 1.0f),
        };

        const vec2f32 delta_input = current_input - initial_input;
        // XR_LOG_INFO("mouse motion {}, delta {}", mouse_pos, delta_input);

        _simstate.flightcam.look_input = delta_input;
        // XR_LOG_INFO("Input {}", _simstate.flightcam.look_input);
    }
}

void
B5::GameSimulation::event_handler(const xray::ui::window_event& evt)
{
    if (evt.type == event_type::gamepad_axis) {
        handle_gamepad_axis_event(evt.event.gamepad_axis);
        return;
    }

    if (evt.type == event_type::gamepad_button) {
        handle_gamepad_button_event(evt.event.gamepad_button);
        return;
    }

    if (is_input_event(evt)) {
        if (_uistate.ui_opened) {
            _ui->input_event(evt);
        }

        if (_uistate.use_arcball_cam) {
            _simstate.arcball_cam.input_event(evt);
        }

        if (evt.type == event_type::mouse_button) {
            handle_mouse_button_event(evt.event.button);
            return;
        }

        if (evt.type == event_type::mouse_motion) {
            handle_mouse_motion_event(evt.event.motion);
            return;
        }

        const bool is_key_press_event = evt.type == event_type::key && evt.event.key.type == event_action_type::press;
        if (is_key_press_event && evt.event.key.keycode == KeySymbol::f10) {
            _uistate.ui_opened = !_uistate.ui_opened;
            return;
        }

        if (!_ui->wants_input() && evt.type == event_type::key) {
            _inputstate.keyboard[static_cast<size_t>(evt.event.key.keycode)] =
                evt.event.key.type == event_action_type::press;
        }

        return;
    }

    if (evt.type == xray::ui::event_type::configure) {
        const xray::ui::window_configure_event* wce = &evt.event.configure;
        if (wce->width != 0 && wce->height != 0) {
            const auto perspective_projection = perspective_symmetric(
                static_cast<float>(wce->width) / static_cast<float>(wce->height), 65.0_DEG2RADF32, 0.1f, 1000.0f);
            _simstate.camera.set_projection(perspective_projection);
        }
    }

    // _ui->input_event(evt);
}

void
B5::GameSimulation::user_interface(xray::ui::user_interface* ui, const RenderEvent& re)
{
    ZoneScopedNCS("UI", tracy::Color::GreenYellow, 16);

    draw_hud_text(ui, re);
    if (!_uistate.ui_opened) {
        return;
    }

    if (ImGui::Begin("Demo options")) {
        char scratch_buff[1024];
        // ui->push_font("TerminessNerdFontMono-Regular_24");

        ui->push_font("ZedMonoNerdFontMono-Medium_24");

        _terrain->user_interface(ui, re);

        if (ImGui::CollapsingHeader("::: Ship :::", ImGuiTreeNodeFlags_DefaultOpen)) {
            const JPH::Vec3 com_pos = _world.ent_player.phys_body->GetCenterOfMassPosition();
            const JPH::Vec3 pos = _world.ent_player.phys_body->GetPosition();

            format_to_n(scratch_buff, "COM Pos: ({},{},{})", com_pos.GetX(), com_pos.GetY(), com_pos.GetZ());
            ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%s", scratch_buff);

            format_to_n(scratch_buff, "Pos: ({},{},{})", pos.GetX(), pos.GetY(), pos.GetZ());
            ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%s", scratch_buff);
        }

        if (ImGui::CollapsingHeader("::: Gamepad axis state :::")) {
            for (const GamepadAxisEvent& e : _inputstate.last_axis_events) {
                format_to_n(scratch_buff, "{} - {}", e.axis, e.i32);
                ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%s", scratch_buff);
            }
        }

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

        ui->pop_font();
    }
    ImGui::End();
}

void
B5::GameSimulation::loop_event(const RenderEvent& render_event)
{
    ZoneScopedNCS("scene loop", tracy::Color::Orange, 32);

    user_interface(render_event.ui, render_event);
    process_gamepad_state();
    process_keyboard_state();
    _physics->update();

    simulation_details::SpacecraftData* sd = &_world.ent_player.data;
    sd->position = _world.ent_player.phys_body->GetCenterOfMassPosition();
    sd->linear_velocity = _world.ent_player.phys_body->GetLinearVelocity();
    sd->angular_velocity = _world.ent_player.phys_body->GetAngularVelocity();
    sd->rotation = _world.ent_player.phys_body->GetRotation();

    const JPH::Mat44 ship_world_transform = _world.ent_player.phys_body->GetWorldTransform();
    sd->direction = ship_world_transform.GetAxisZ();
    sd->up = ship_world_transform.GetAxisY();

    ScratchPadArena scratch_pad{ &_arena_temp };

    if (_uistate.use_arcball_cam) {
        _simstate.arcball_cam.set_zoom_speed(4.0f * render_event.delta * 1.0e-3f);
        _simstate.arcball_cam.update_camera(_simstate.camera);
    } else {
        // mat4f rotation;
        // player_tf.GetRotation().Transposed().StoreFloat4x4(reinterpret_cast<JPH::Float4*>(&rotation.components));
        // vec3f translation;
        // player_tf.GetTranslation().StoreFloat3(reinterpret_cast<JPH::Float3*>(&translation.components));
        // _simstate.flight_cam.update(rotation, translation);
        // _simstate.camera.set_view_matrix(_simstate.flight_cam.view_matrix,
        // _simstate.flight_cam.inverse_of_view_matrix);

        // JPH::BodyInterface* bdi = &_physics->sim()->GetBodyInterface();
        const MatrixWithInvertedMatrixPair4f view_transform = _simstate.flightcam.update(*_world.ent_player.phys_body);
        _simstate.camera.set_view_matrix(view_transform);
    }

    //
    // set this here, will be used by other object further down
    FrameGlobalData* frame_global_data = render_event.g_ubo_data;
    SimState* s = &_simstate;
    frame_global_data->world_view_proj = s->camera.projection_view();
    frame_global_data->view = s->camera.view();
    frame_global_data->eye_pos = s->camera.origin();
    frame_global_data->projection = s->camera.projection();
    frame_global_data->ortho = orthographic(0.0f,
                                            static_cast<float>(render_event.frame_data->fbsize.width),
                                            0.0f,
                                            static_cast<float>(render_event.frame_data->fbsize.height),
                                            0.1f,
                                            100.0f);

    // const auto m0 = orthographic_symmetric(static_cast<float>(render_event.frame_data->fbsize.width),
    //                                        static_cast<float>(render_event.frame_data->fbsize.height),
    //                                        0.1f,
    //                                        100.0f);
    //
    // XR_LOG_INFO("Ortho: {}\nOrtho symmetric {}", frame_global_data->ortho, m0);

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

        //
        // lights setup
        frame_global_data->lights = LightingSetup{
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
        frame_global_data->global_color_texture = color_tex_handle;

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

    _terrain->loop_event(render_event);

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
            _physics->sim()
                ->GetBodyInterface()
                .GetCenterOfMassTransform(_world.ent_player.phys_body_id)
                .Transposed()
                .StoreFloat4x4(reinterpret_cast<JPH::Float4*>(&iri->model.components));

            iri->model_view = _simstate.camera.view() * iri->model;
            iri->normals_view = iri->model_view;
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

    //
    // HUD + anything else 2D related
    draw_hud(render_event);

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

void
B5::GameSimulation::process_keyboard_state()
{
    const FlightModel fm{};
    const JPH::BodyID ship_body = _world.ent_player.phys_body_id;

    for (const KeySymbol ks : {
             KeySymbol::key_w,
             KeySymbol::key_s,
             KeySymbol::key_a,
             KeySymbol::key_d,
             KeySymbol::key_q,
             KeySymbol::key_e,
             KeySymbol::up,
             KeySymbol::down,
             KeySymbol::left,
             KeySymbol::right,
         }) {
        if (!_inputstate.keyboard[static_cast<size_t>(ks)]) {
            continue;
        }

        const KeyStateData& key_state = _inputstate.keys_mapping.at(ks);

        if (key_state.force == ForceType::Impulse) {
            JPH::BodyInterface* ifc = &_physics->sim()->GetBodyInterface();
            const JPH::RMat44 ship_rotation = ifc->GetCenterOfMassTransform(ship_body).GetRotation();
            const JPH::Vec3 applied_force =
                ship_rotation * JPH::Vec3{ key_state.force_axis.x, key_state.force_axis.y, key_state.force_axis.z } *
                fm.thruster_large;
            ifc->AddForce(ship_body, applied_force);
        } else {
            JPH::BodyInterface* ifc = &_physics->sim()->GetBodyInterface();
            const JPH::RMat44 ship_rotation = ifc->GetCenterOfMassTransform(ship_body).GetRotation();
            const JPH::Vec3 applied_torque =
                ship_rotation * JPH::Vec3{ key_state.force_axis.x, key_state.force_axis.y, key_state.force_axis.z } *
                fm.thruster_small;
            ifc->AddTorque(ship_body, applied_torque);
        }
    }

    if (_inputstate.keyboard[static_cast<size_t>(KeySymbol::backspace)]) {
        JPH::BodyInterface* ifc = &_physics->sim()->GetBodyInterface();
        ifc->SetPositionRotationAndVelocity(_world.ent_player.phys_body_id,
                                            JPH::Vec3::sZero(),
                                            JPH::Quat::sIdentity(),
                                            JPH::Vec3::sZero(),
                                            JPH::Vec3::sZero());
    }
}

struct HudTextElement
{
    float x;
    float y;
    float w;
    float h;
    uint32_t color;
};

float
compute_compass_bearing(const vec2f32 pos) noexcept
{
    //
    // https://stackoverflow.com/questions/31838855/how-do-i-easily-convert-a-line-angle-to-a-navigational-bearing-scale-i-e-with
    // (A1 - atan2(y2-y1,x2-x1) * 180/pi ) %%360
    // North is 0 degrees, East 90, South 180, West 270
    return std::fmod(450.0f - atan2(pos.y, pos.x) * F32::OneEightyOverPi, 360.0f);
}

void
B5::GameSimulation::draw_hud_text(xray::ui::user_interface* ui, const RenderEvent& re)
{
    ImGui::SetNextWindowPos({ 0.0f, 0.0f });
    ImGui::Begin("HUD",
                 nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);

    const uint32_t hud_color = static_cast<uint32_t>(color_palette::flat::greensea300);
    constexpr const string_view hud_font_name{ "B612-Bold_48" };
    const auto hud_font = ui->find_font(hud_font_name.data());
    const auto hud_font_small = ui->find_font("B612-Bold_32");
    ui->push_font(hud_font_name.data());

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    const simulation_details::SpacecraftData* sd = &_world.ent_player.data;
    char scratch_buffer[1024];

    const bool singularity = is_zero(sd->direction.Cross(JPH::Vec3::sAxisY()).LengthSq());
    const JPH::Vec3 dir = singularity ? sd->up : sd->direction;

    const HudConfigDefinition* hud_cfg = re.hud_cfg;

    const vec2f32 compass_origin{ static_cast<float>(re.frame_data->fbsize.width) * 0.5f, hud_cfg->compass.ypos };
    const float compass_bearing = compute_compass_bearing(vec2f32{ sd->direction.GetX(), dir.GetZ() });

    format_to_n(scratch_buffer, "ALT: {}", static_cast<int32_t>(sd->position.GetY()));

    vec2f32 cursor_xy{ 256.0f, 512.0f };
    draw_list->AddText({ cursor_xy.x, cursor_xy.y }, hud_color, scratch_buffer);
    cursor_xy.y += hud_font->font->Ascent;

    format_to_n(scratch_buffer, "{}", static_cast<int32_t>(std::round(sd->linear_velocity.Length())));
    draw_list->AddText({ cursor_xy.x, cursor_xy.y }, hud_color, scratch_buffer);
    cursor_xy.y += hud_font->font->Ascent + 4.0f;

    auto draw_compass_markers = [draw_list, hud_color, hud_font_small, hud_cfg](const float start_angle,
                                                                                const float range_degrees,
                                                                                const float angle_increment,
                                                                                const float line_length,
                                                                                const vec2f32 line_origin) {
        const float angle_start = start_angle - (range_degrees * 0.5f);
        const float angle_end = start_angle + (range_degrees * 0.5f);

        float current_angle = angle_start;
        while (current_angle <= angle_end) {
            //
            // find next multiple of angle_increment
            const float marker_angle =
                ((static_cast<int32_t>(current_angle) - 1) / static_cast<int32_t>(angle_increment) + 1) *
                angle_increment;

            if (marker_angle > angle_end)
                break;

            const float marker_x = line_origin.x + ((marker_angle - angle_start) / (range_degrees)) * line_length;
            const char* marker_symbol = hud_cfg->compass.glyph_minor.data();

            if (is_zero(std::fmod(marker_angle, angle_increment * 2.0f))) {
                marker_symbol = hud_cfg->compass.glyph_major.data();
            }

            draw_list->AddText(hud_font_small->font,
                               hud_font_small->pixel_size,
                               { marker_x, line_origin.y },
                               hud_color,
                               marker_symbol);

            float displayed_angle = marker_angle;
            if (displayed_angle < 0.0f)
                displayed_angle += 360.0f;
            if (displayed_angle > 360.0f)
                displayed_angle -= 360.0f;

            char scratch_buf[64];
            format_to_n(scratch_buf, "{:3.0f}°", displayed_angle);
            draw_list->AddText(hud_font_small->font,
                               hud_font_small->pixel_size,
                               { marker_x, line_origin.y + hud_font_small->font->Ascent },
                               hud_color,
                               scratch_buf);
            current_angle += angle_increment;
        }
    };

    draw_compass_markers(compass_bearing,
                         hud_cfg->compass.arc_degrees,
                         hud_cfg->compass.angle_increment,
                         hud_cfg->compass.bar_width,
                         compass_origin - vec2f32{ hud_cfg->compass.bar_width * 0.5f, 0.0f });

    const vec2f32 v2{ compass_origin.x, compass_origin.y + hud_font_small->font->Ascent * 2.0f };
    const vec2f32 p{ v2 + vec2f32{ 0.0f, 32.0f } };
    const vec2f32 v0{ compass_origin.x - 24.0f, p.y };
    const vec2f32 v1{ compass_origin.x + 24.0f, p.y };

    draw_list->AddTriangleFilled({ v0.x, v0.y }, { v2.x, v2.y }, { v1.x, v1.y }, hud_color);
    // draw_list->AddTriangle({ v0.x, v0.y }, { v2.x, v2.y }, { v1.x, v1.y }, hud_color, 4.0f);

    // re.sprites->draw(compass_origin.x - 32.0f,
    //                  compass_origin.y + hud_font_small->font->Ascent * 2.0f,
    //                  64.0f,
    //                  64.0f,
    //                  SpriteIds::WHITE_RETINA_CROSSHAIR024,
    //                  hud_color);

    // format_to_n(scratch_buffer, "{:3.0f}", compass_bearing);
    // draw_list->AddText(hud_font_small->font,
    //                    hud_font_small->pixel_size,
    //                    { compass_origin.x + 4.0f, compass_origin.y + hud_font_small->font->Ascent * 2.0f + 32.0f },
    //                    hud_color,
    //                    scratch_buffer);

    //
    // altitude
    auto draw_altitude_markers = [draw_list, hud_color, hud_font_small, hud_cfg](const float altitude,
                                                                                 const float altitude_range,
                                                                                 const float altitude_increment,
                                                                                 const float line_length,
                                                                                 const vec2f32 line_origin) {
        const float altitude_start = altitude - (altitude_range * 0.5f);
        const float altitude_end = altitude + (altitude_range * 0.5f);

        for (float current_altitude = altitude_start; current_altitude <= altitude_end;
             current_altitude += altitude_increment) {
            //
            // find next multiple of angle_increment
            const float altitude_marker = std::floor(current_altitude / altitude_increment) * altitude_increment;

            if (altitude_marker > altitude_end)
                break;

            const float marker_y =
                line_origin.y + (1.0f - (altitude_marker - altitude_start) / (altitude_range)) * line_length;

            if (marker_y <= (line_origin.y + hud_cfg->altimeter.marker_height))
                continue;

            if (marker_y >= (line_origin.y - hud_cfg->altimeter.marker_height + line_length))
                continue;

            const bool is_marker_large = is_zero(std::fmod(altitude_marker, hud_cfg->altimeter.marker_big_meters));
            const float line_len =
                is_marker_large ? hud_cfg->altimeter.marker_big_len : hud_cfg->altimeter.marker_small_len;

            draw_list->AddRectFilled({ line_origin.x, marker_y - hud_cfg->altimeter.marker_height * 0.5f },
                                     { line_origin.x + line_len, marker_y + hud_cfg->altimeter.marker_height * 0.5f },
                                     hud_color);

            if (is_marker_large) {
                char scratch_buf[64];
                format_to_n(scratch_buf, "{: >3.0f}", altitude_marker);
                draw_list->AddText(hud_font_small->font,
                                   hud_font_small->pixel_size,
                                   { line_origin.x + hud_cfg->altimeter.marker_big_len + 4.0f,
                                     marker_y - hud_font_small->font->Ascent * 0.5f },
                                   hud_color,
                                   scratch_buf);
            }
        }
    };

    const float altimeter_bar_height =
        static_cast<float>(re.frame_data->fbsize.height) - hud_cfg->altimeter.ymargin * 2.0f;
    const vec2f32 altimeter_origin{
        static_cast<float>(re.frame_data->fbsize.width) - hud_cfg->altimeter.xmargin,
        hud_cfg->altimeter.ymargin,
    };

    draw_altitude_markers(sd->position.GetY(),
                          hud_cfg->altimeter.range,
                          hud_cfg->altimeter.increment,
                          altimeter_bar_height,
                          altimeter_origin);

    draw_list->AddRectFilled({ altimeter_origin.x - hud_cfg->altimeter.bar_width, altimeter_origin.y },
                             { altimeter_origin.x, altimeter_origin.y + altimeter_bar_height },
                             hud_color);
    draw_list->AddRectFilled(
        { altimeter_origin.x, altimeter_origin.y },
        { altimeter_origin.x + hud_cfg->altimeter.bar_ends_len, altimeter_origin.y + hud_cfg->altimeter.bar_width },
        hud_color);

    draw_list->AddRectFilled(
        { altimeter_origin.x, altimeter_origin.y + altimeter_bar_height - hud_cfg->altimeter.bar_width },
        { altimeter_origin.x + hud_cfg->altimeter.bar_ends_len, altimeter_origin.y + altimeter_bar_height },
        hud_color);

    draw_list->AddLine(
        { 0.0f, altimeter_origin.y + altimeter_bar_height * 0.5f },
        { static_cast<float>(re.frame_data->fbsize.width), altimeter_origin.y + altimeter_bar_height * 0.5f },
        static_cast<uint32_t>(color_palette::web::orange_red));

    const vec2f32 alt_arrowpos{
        altimeter_origin.x + hud_cfg->altimeter.bar_ends_len,
        altimeter_origin.y + altimeter_bar_height * 0.5f - 64.0f,
    };

    re.sprites->draw_scaled_rotated(alt_arrowpos.x,
                                    alt_arrowpos.y,
                                    128.0f,
                                    128.0f,
                                    1.0f,
                                    radians(-90.0f),
                                    SpriteIds::WHITE_RETINA_CROSSHAIR127,
                                    hud_color);

    format_to_n(scratch_buffer, "{: >5.0f}", sd->position.GetY());
    const vec2f32 alt_hight_text_pos{ alt_arrowpos + vec2f32{ 32.0f, 32.0f } };
    draw_list->AddText(hud_font_small->font,
                       hud_font_small->pixel_size,
                       { alt_hight_text_pos.x, alt_hight_text_pos.y },
                       hud_color,
                       scratch_buffer);

    auto add_text_element = [&](const string_view label, auto&& v) {
        using value_type = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::is_floating_point_v<value_type>) {
            format_to_n(scratch_buffer, "{}: {:3.3f}", label, v);
        } else if constexpr (std::is_integral_v<value_type>) {
            format_to_n(scratch_buffer, "{}: {}", label, v);
        } else if constexpr (std::is_same_v<value_type, JPH::Vec3>) {
            format_to_n(scratch_buffer, "{}: {:3.3f}, {:3.3f}, {:3.3f}", label, v.GetX(), v.GetY(), v.GetZ());
        } else {
            static_assert(false, "Unhandled value type");
        }

        draw_list->AddText({ cursor_xy.x, cursor_xy.y }, hud_color, scratch_buffer);
        cursor_xy.y += hud_font->font->Ascent + 4.0f;
    };

    add_text_element("ROLL", sd->rotation.GetRotationAngle(JPH::Vec3::sAxisZ()));
    add_text_element("PITCH", sd->rotation.GetRotationAngle(JPH::Vec3::sAxisX()));
    add_text_element("RollPitchYaw", sd->rotation.GetEulerAngles());

    ImGui::Dummy({ static_cast<float>(re.frame_data->fbsize.width), static_cast<float>(re.frame_data->fbsize.height) });

    ui->pop_font();
    ImGui::End();
}

void
B5::GameSimulation::draw_hud(const RenderEvent& render_evt)
{
    const float size = 128.0f;
    vec2f32 coords{ 0 };
    // for (const SpriteHandleType sprite_id : {
    //          SpriteIds::WHITE_RETINA_CROSSHAIR126,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR127,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR128,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR129,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR049,
    //      }) {
    //     render_evt.sprites->draw(
    //         coords.x, coords.y, size, size, sprite_id, static_cast<uint32_t>(color_palette::flat::greensea400));
    //     coords.x += size;
    // }

    // coords.x = 0;
    // coords.y += size;
    // for (const SpriteHandleType sprite_id : {
    //          SpriteIds::WHITE_RETINA_CROSSHAIR126,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR127,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR128,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR129,
    //          SpriteIds::WHITE_RETINA_CROSSHAIR049,
    //      }) {
    //     render_evt.sprites->draw_scaled_rotated(coords.x,
    //                                             coords.y,
    //                                             size,
    //                                             size,
    //                                             2.0f,
    //                                             radians(45.0f),
    //                                             sprite_id,
    //                                             static_cast<uint32_t>(color_palette::flat::greensea600));
    //     coords.x += size * 2.0f;
    // }

    // coords.x = 0.0f;
    // coords.y += size * 2.0f;
    //
    // for (const auto [rotation, sprite_id] : { std::tuple{ radians(45.0f), SpriteIds::WHITE_RETINA_CROSSHAIR126 },
    //                                           std::tuple{ radians(-45.0f), SpriteIds::WHITE_RETINA_CROSSHAIR127 } })
    //                                           {
    //     render_evt.sprites->draw_scaled_rotated_with_origin(512.0f,
    //                                                         512.0f,
    //                                                         size,
    //                                                         size,
    //                                                         3.0f,
    //                                                         rotation,
    //                                                         sprite_id,
    //                                                         static_cast<uint32_t>(color_palette::flat::emerald50));
    // }
}

void
B5::GameSimulation::process_gamepad_state()
{
    const FlightModel fm{};
    const JPH::BodyID ship_body = _world.ent_player.phys_body_id;

    if (!_inputstate.axis_info.empty() && !_inputstate.last_mouse_down) {
        vec2f32 input_vec{ 0 };

        for (const auto [idx, axis] : { std::tuple{ 0, GamepadAxis::RightX }, std::tuple{ 1, GamepadAxis::RightY } }) {
            const GamepadAxisEvent& axis_event = _inputstate.last_axis_events[static_cast<size_t>(axis)];
            const GamepadAxisInfo& axis_info = _inputstate.axis_info[static_cast<size_t>(axis)];

            if (std::abs(axis_event.i32) <= axis_info.deadzone) {
                continue;
            }

            input_vec[idx] = axis_event.f32;
        }

        _simstate.flightcam.look_input = input_vec;
    }

    lz::chain(lz::zip(_inputstate.last_axis_events, _inputstate.axis_info))
        .forEach([this, ship_body, &fm](const std::tuple<GamepadAxisEvent, GamepadAxisInfo>& evt_bundle) {
            const auto& [axis_event, axis_info] = evt_bundle;

            if (std::abs(axis_event.i32) <= axis_info.deadzone) {
                return;
            }

            tl::optional<tuple<int32_t, JPH::Vec3>> applied_force;
            tl::optional<tuple<int32_t, JPH::Vec3>> applied_torque;

            switch (axis_event.axis) {
                case GamepadAxis::LeftX:
                    // XR_LOG_INFO("LeftX");
                    applied_force = tuple{ axis_event.i32, JPH::Vec3::sAxisX() };
                    break;

                case GamepadAxis::LeftY:
                    // XR_LOG_INFO("LeftY");
                    applied_force = tuple{ -axis_event.i32, JPH::Vec3::sAxisZ() };
                    break;

                case GamepadAxis::RightX:
                    //
                    // pitch
                    // applied_torque = tuple{ axis_event.i32, JPH::Vec3::sAxisZ() };
                    break;

                case GamepadAxis::RightY:
                    //
                    // roll
                    // applied_torque = tuple{ axis_event.i32, JPH::Vec3::sAxisX() };
                    break;

                case GamepadAxis::LeftZ:
                    //
                    // yaw
                    applied_torque = tuple{ axis_event.i32, JPH::Vec3::sAxisY() };
                    break;

                case GamepadAxis::RightZ:
                    applied_torque = tuple{ -axis_event.i32, JPH::Vec3::sAxisY() };
                    break;

                default:
                    break;
            }

            applied_force.map([&, this](const tuple<int, JPH::Vec3> force_with_axis) {
                const auto [force, force_axis] = force_with_axis;
                const float force_amount = static_cast<float>(force) / static_cast<float>(axis_info.max_val);

                JPH::BodyInterface* ifc = &_physics->sim()->GetBodyInterface();
                const JPH::RMat44 ship_rotation = ifc->GetCenterOfMassTransform(ship_body).GetRotation();
                const JPH::Vec3 applied_force = ship_rotation * force_axis * force_amount * fm.thruster_large;
                ifc->AddForce(ship_body, applied_force);
            });

            applied_torque.map([&, this](const tuple<int, JPH::Vec3> force_with_axis) {
                const auto [torque, torque_axis] = force_with_axis;
                const float torque_amount = static_cast<float>(-torque) / static_cast<float>(axis_info.max_val);

                JPH::BodyInterface* ifc = &_physics->sim()->GetBodyInterface();
                const JPH::RMat44 ship_rotation = ifc->GetCenterOfMassTransform(ship_body).GetRotation();
                const JPH::Vec3 applied_torque = ship_rotation * torque_axis * torque_amount * fm.thruster_small;
                ifc->AddTorque(ship_body, applied_torque);
            });
        });
}
