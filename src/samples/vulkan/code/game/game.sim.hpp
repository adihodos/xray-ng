#pragma once

#include <bitset>
#include <cstdint>
#include <cstddef>

#include <tl/expected.hpp>
#include <concurrencpp/forward_declarations.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/BodyManager.h>

#include "xray/base/unique_pointer.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/math/math.units.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera.controller.arcball.hpp"
#include "xray/scene/camera.controller.flight.hpp"

namespace xray::ui {
class user_interface;
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

namespace simulation_details {

struct Starfury
{
    uint32_t entity{};
    uint32_t geometry{};
    JPH::BodyID phys_body_id{};
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

  private:
    void user_interface(xray::ui::user_interface* ui, const RenderEvent& re);

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

        SimState() = default;
        SimState(const InitContext& init_context);
    } _simstate{};

    xray::base::unique_pointer<PhysicsSystem> _physics;

    struct UIState
    {
        static constexpr const size_t MAX_LIGHTS = 64;
        bool use_arcball_cam{ false };
        JPH::BodyManager::DrawSettings phys_draw{};
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

    xray::base::timer_highp _timer{};

  public:
    GameSimulation(PrivateConstructionToken,
                   const InitContext& init_context,
                   xray::base::unique_pointer<PhysicsSystem> phys,
                   std::span<std::byte> arena_perm,
                   std::span<std::byte> arena_temp);
    ~GameSimulation();
};

}
