#pragma once

#include <bitset>
#include <cstdint>
#include <cstddef>
#include <span>

#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <concurrencpp/forward_declarations.h>
#include <frozen/unordered_map.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyManager.h>

#include "xray/base/unique_pointer.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/memory.arena.unique.ptr.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/math/math.units.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera.controller.arcball.hpp"
#include "xray/scene/camera.controller.flight.hpp"
#include "xray/ui/events.gamepad.hpp"
#include "xray/ui/key_sym.hpp"

#include "flight.cam.hpp"

namespace xray::ui {
class user_interface;
struct GamepadAxisEvent;
struct GamepadButtonEvent;
struct mouse_button_event;
struct mouse_motion_event;
};

namespace xray::scene {
class GltfGeometryEntry;
struct EntityDrawableComponent;
};

namespace xray::rendering {
struct GeometryWithRenderData;
struct GeneratedGeometryWithRenderData;
};

namespace B5 {

struct RenderEvent;
struct InitContext;
class PhysicsSystem;
class Terrain;

namespace simulation_details {

struct Starfury
{
    uint32_t entity{};
    uint32_t geometry{};
    JPH::BodyID phys_body_id{};
    JPH::Body* phys_body{};
};

struct GameWorldState
{
    explicit GameWorldState(xray::base::MemoryArena& arena)
        : ent_gltf{ arena }
        , ent_basic{ arena }
        , ent_physics_bodies{ arena }
    {
    }

    xray::base::containers::vector<xray::scene::EntityDrawableComponent> ent_gltf;
    xray::base::containers::vector<xray::scene::EntityDrawableComponent> ent_basic;
    xray::base::containers::vector<JPH::BodyID> ent_physics_bodies;
    Starfury ent_player;
};

}

class GameSimulation
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    void event_handler(const xray::ui::window_event& evt);
    void loop_event(const RenderEvent&);

    static xray::base::unique_pointer<GameSimulation> create(const InitContext& init_ctx);
    const xray::scene::camera& camera() const noexcept { return _simstate.camera; }

  private:
    void user_interface(xray::ui::user_interface* ui, const RenderEvent& re);
    void handle_gamepad_axis_event(const xray::ui::GamepadAxisEvent& e);
    void handle_gamepad_button_event(const xray::ui::GamepadButtonEvent& e);
    void handle_mouse_button_event(const xray::ui::mouse_button_event& mbe);
    void handle_mouse_motion_event(const xray::ui::mouse_motion_event& mme);

    void process_gamepad_state();
    void process_keyboard_state();

    struct SimState
    {
        float angle{};
        xray::scene::camera camera{};
        xray::scene::ArcballCamera arcball_cam{};
        xray::scene::FlightCamera flight_cam{
            xray::math::RadiansF32{ 65.0_DEG2RADF32 },
            4.0f / 3.0f,
            0.1f,
            1000.0f,
        };
        std::bitset<8> lights_sync{ 0 };
        FlightCamera flightcam;

        SimState() = default;
        SimState(const InitContext& init_context);
    } _simstate{};

    xray::base::unique_pointer<PhysicsSystem> _physics;

    struct UIState
    {
        bool ui_opened{ false };
        static constexpr const size_t MAX_LIGHTS = 64;
        bool use_arcball_cam{ false };
        JPH::BodyManager::DrawSettings phys_draw{
            .mDrawShape = false,
        };
        bool draw_bbox{ false };
        bool draw_world_axis{ true };
        bool draw_sphere{ false };
        bool draw_nodes_spheres{ false };
        bool draw_nodes_bbox{ false };
        bool draw_ship{ true };
        std::bitset<32> shapes_draw{ 0x0 };
        std::bitset<MAX_LIGHTS> dbg_directional_lights{ 0 };
        std::bitset<MAX_LIGHTS> toggle_directional_lights{ std::bitset<MAX_LIGHTS>{}.set() };
        std::bitset<MAX_LIGHTS> dbg_point_lights{ 0 };
        std::bitset<MAX_LIGHTS> toggle_point_lights{ std::bitset<MAX_LIGHTS>{}.set() };
    } _uistate{};

    xray::base::MemoryArena _arena_perm;
    xray::base::MemoryArena _arena_temp;
    simulation_details::GameWorldState _world;
    xray::base::unique_arena_ptr<Terrain> _terrain;

    xray::base::timer_highp _timer{};
    xray::ui::user_interface* _ui{};

    enum class ForceType
    {
        Impulse,
        Torque
    };

    struct KeyStateData
    {
        xray::math::vec3f force_axis;
        ForceType force;
    };

    struct InputStateTracker
    {
        xray::base::containers::vector<xray::ui::GamepadAxisEvent> last_axis_events;
        xray::base::containers::vector<xray::ui::GamepadAxisInfo> axis_info;
        std::bitset<256> keyboard{ 0 };
        frozen::unordered_map<xray::ui::KeySymbol, KeyStateData, 10> keys_mapping{
            { xray::ui::KeySymbol::key_w, KeyStateData{ xray::math::vec3f::stdc::unit_z, ForceType::Impulse } },
            { xray::ui::KeySymbol::key_s, KeyStateData{ -xray::math::vec3f::stdc::unit_z, ForceType::Impulse } },
            { xray::ui::KeySymbol::key_a, KeyStateData{ -xray::math::vec3f::stdc::unit_x, ForceType::Impulse } },
            { xray::ui::KeySymbol::key_d, KeyStateData{ xray::math::vec3f::stdc::unit_x, ForceType::Impulse } },
            { xray::ui::KeySymbol::key_q, KeyStateData{ xray::math::vec3f::stdc::unit_z, ForceType::Torque } },
            { xray::ui::KeySymbol::key_e, KeyStateData{ -xray::math::vec3f::stdc::unit_z, ForceType::Torque } },
            { xray::ui::KeySymbol::up, KeyStateData{ xray::math::vec3f::stdc::unit_x, ForceType::Torque } },
            { xray::ui::KeySymbol::down, KeyStateData{ -xray::math::vec3f::stdc::unit_x, ForceType::Torque } },
            { xray::ui::KeySymbol::left, KeyStateData{ -xray::math::vec3f::stdc::unit_y, ForceType::Torque } },
            { xray::ui::KeySymbol::right, KeyStateData{ xray::math::vec3f::stdc::unit_y, ForceType::Torque } },
        };

        xray::math::vec2f32 screen_size_inv;
        tl::optional<xray::math::vec2f32> last_mouse_down{};

        InputStateTracker(xray::base::MemoryArena* arena,
                          std::span<const xray::ui::GamepadAxisInfo> ai,
                          xray::math::vec2f32 scr_size);

    } _inputstate;

  public:
    GameSimulation(PrivateConstructionToken,
                   const InitContext& init_context,
                   xray::base::unique_pointer<PhysicsSystem> phys,
                   Terrain terrain,
                   std::span<std::byte> arena_perm,
                   std::span<std::byte> arena_temp);
    ~GameSimulation();
};

}
