#pragma once

#include "xray/xray.hpp"
#include "demo_base.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"

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
    xray::rendering::GraphicsPipeline _pipeline;
    float _angle{};

  public:
    TriangleDemo(PrivateConstructionToken,
                 const app::init_context_t& init_context,
                 xray::rendering::GraphicsPipeline pipeline);
};

} //
