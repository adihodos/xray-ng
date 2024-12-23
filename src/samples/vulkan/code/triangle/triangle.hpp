#pragma once

#include "xray/xray.hpp"

#include "demo_base.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera.controller.arcball.hpp"

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
    struct RenderState
    {
        // xray::rendering::BindlessStorageBufferResourceHandleEntryPair
        xray::rendering::VulkanBuffer g_vertexbuffer;
        // xray::rendering::BindlessStorageBufferResourceHandleEntryPair
        xray::rendering::VulkanBuffer g_indexbuffer;
        xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_instancebuffer;
        xray::rendering::GraphicsPipeline pipeline;
        xray::rendering::BindlessImageResourceHandleEntryPair g_texture;
        xray::rendering::xrUniqueVkImageView imageview;
        xray::rendering::xrUniqueVkSampler sampler;
    } _renderstate;

    struct SimState
    {
        float angle{};
        xray::scene::camera camera{};
        xray::scene::ArcballCamera arcball_cam{};

        SimState() = default;
        SimState(const app::init_context_t& init_context);

    } _simstate;

    struct WorldState;
    xray::base::unique_pointer<WorldState> _world;

  public:
    TriangleDemo(PrivateConstructionToken,
                 const app::init_context_t& init_context,
                 xray::rendering::VulkanBuffer&& g_vertexbuffer,
                 xray::rendering::VulkanBuffer&& g_indexbuffer,
                 xray::rendering::BindlessStorageBufferResourceHandleEntryPair g_instancebuffer,
                 xray::rendering::GraphicsPipeline pipeline,
                 xray::rendering::BindlessImageResourceHandleEntryPair g_texture,
                 xray::rendering::xrUniqueVkImageView imageview,
                 xray::rendering::xrUniqueVkSampler sampler,
                 xray::base::unique_pointer<WorldState> _world);

    ~TriangleDemo();
};

}
