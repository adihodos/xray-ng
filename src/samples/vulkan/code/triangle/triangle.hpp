#pragma once

#include <bitset>

#include <tl/expected.hpp>
#include <concurrencpp/forward_declarations.h>

#include "xray/base/unique_pointer.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera.controller.arcball.hpp"

namespace xray::ui {
class user_interface;
};

namespace xray::rendering {
struct GeometryWithRenderData;
struct GeneratedGeometryWithRenderData;
};

namespace B5 {

struct RenderEvent;
struct InitContext;

class TriangleDemo
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    void event_handler(const xray::ui::window_event& evt);
    void loop_event(const RenderEvent&);

    static xray::base::unique_pointer<TriangleDemo> create(const InitContext& init_ctx);

  private:
    void user_interface(xray::ui::user_interface* ui, const RenderEvent& re);

    struct SimState
    {
        float angle{};
        xray::scene::camera camera{};
        xray::scene::ArcballCamera arcball_cam{};
        std::bitset<8> lights_sync{ 0 };

        SimState() = default;
        SimState(const InitContext& init_context);

    } _simstate{};

    struct UIState
    {
        static constexpr const size_t MAX_LIGHTS = 64;
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

    xray::base::timer_highp _timer{};

  public:
    TriangleDemo(PrivateConstructionToken, const InitContext& init_context);
    ~TriangleDemo();
};

}
