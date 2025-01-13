#pragma once

#include "demo_base.hpp"

#include <bitset>
#include <vector>

#include <tl/expected.hpp>
#include <concurrencpp/forward_declarations.h>

#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera.controller.arcball.hpp"

#include "xray/rendering/vertex_format/vertex.format.pbr.hpp"
#include "xray/rendering/geometry.importer.gltf.hpp"
#include "xray/rendering/geometry.hpp"

namespace xray::ui {
class user_interface;
};

namespace xray::rendering {
struct GeometryWithRenderData;
struct GeneratedGeometryWithRenderData;
};

namespace dvk {

class TriangleDemo : public app::DemoBase
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    virtual void event_handler(const xray::ui::window_event& evt) override;
    virtual void loop_event(const app::RenderEvent&) override;

    static std::string_view short_desc() noexcept { return "Vulkan triangle."; }
    static std::string_view detailed_desc() noexcept { return "Rendering a simple triangle using Vulkan."; }

    static xray::base::unique_pointer<app::DemoBase> create(const app::init_context_t& init_ctx);

  private:
    void user_interface(xray::ui::user_interface* ui);

    struct SimState
    {
        float angle{};
        xray::scene::camera camera{};
        xray::scene::ArcballCamera arcball_cam{};

        SimState() = default;
        SimState(const app::init_context_t& init_context);

    } _simstate{};

    struct UIState
    {
        bool draw_bbox{ false };
        bool draw_world_axis{ true };
        bool draw_sphere{ false };
        bool draw_nodes_spheres{ false };
        bool draw_nodes_bbox{ false };
        bool draw_ship{ true };
        std::bitset<32> shapes_draw{ 0x0 };
    } _uistate{};

  public:
    TriangleDemo(PrivateConstructionToken, const app::init_context_t& init_context);
    ~TriangleDemo();
};

}
