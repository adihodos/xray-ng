#pragma once

#include "demo_base.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera.controller.arcball.hpp"

namespace xray::ui {
class user_interface;
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
    static xray::base::unique_pointer<app::DemoBase> create(const app::init_context_t& initContext);

  private:
    void user_interface(xray::ui::user_interface* ui);

    struct RenderState
    {
        xray::rendering::VulkanBuffer g_vertexbuffer;
        xray::rendering::VulkanBuffer g_indexbuffer;
        xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_instancebuffer;
        xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_materials_buffer;
        xray::rendering::GraphicsPipeline pipeline;
        xray::rendering::BindlessImageResourceHandleEntryPair g_texture;
    } _renderstate;

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
    } _uistate{};

    struct WorldState;
    xray::base::unique_pointer<WorldState> _world;

  public:
    TriangleDemo(PrivateConstructionToken,
                 const app::init_context_t& init_context,
                 xray::rendering::VulkanBuffer&& g_vertexbuffer,
                 xray::rendering::VulkanBuffer&& g_indexbuffer,
                 xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_instancebuffer,
                 xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_materials,
                 xray::rendering::GraphicsPipeline pipeline,
                 xray::rendering::BindlessImageResourceHandleEntryPair g_texture,
                 xray::base::unique_pointer<WorldState> _world);

    ~TriangleDemo();
};

}
