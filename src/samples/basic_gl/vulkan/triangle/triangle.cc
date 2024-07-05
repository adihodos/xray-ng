#include "triangle.hpp"

#include <string_view>

#include "xray/rendering/vulkan.renderer/vulkan.pipeline.builder.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/math/scalar2x3_math.hpp"
#include "xray/math/constants.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;

dvk::TriangleDemo::TriangleDemo(PrivateConstructionToken,
                                const app::init_context_t& init_context,
                                xray::rendering::GraphicsPipeline pipeline)
    : app::DemoBase{ init_context }
    , _pipeline{ std::move(pipeline) }
{
    _timer.start();
}

xray::base::unique_pointer<app::DemoBase>
dvk::TriangleDemo::create(const app::init_context_t& init_ctx)
{
    static constexpr const char* const VERTEX_SHADER = R"#(
#version 460 core

const vec2 TriangleVertices[] = vec2[](
    vec2(-1.0, -1.0),
    vec2(1.0, -1.0),
    vec2(0.0, 1.0)
);

const vec3 TriangleColors[] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

out gl_PerVertex {
    vec4 gl_Position;
};

out VS_OUT_FS_IN {
    layout (location = 0) vec3 color;
} vs_out;

layout (push_constant, row_major) uniform ShaderGlobalData {
    mat2 rotation;
} g_data;

void main() {
    const vec2 pos = g_data.rotation * TriangleVertices[gl_VertexIndex];
    gl_Position = vec4(pos.xy, 0.0, 1.0);
    vs_out.color = TriangleColors[gl_VertexIndex];
}
    )#";

    static constexpr const char* const FRAGMENT_SHADER = R"#(
#version 460 core

in VS_OUT_FS_IN {
    layout (location = 0) vec3 color;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    FinalFragColor = vec4(fs_in.color, 1.0);
}
    )#";

    tl::optional<GraphicsPipeline> pipeline{
        GraphicsPipelineBuilder{}
            .add_shader(ShaderStage::Vertex, string_view{ VERTEX_SHADER })
            .add_shader(ShaderStage::Fragment, string_view{ FRAGMENT_SHADER })
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .create(*init_ctx.renderer),
    };

    if (!pipeline)
        return nullptr;

    return xray::base::make_unique<TriangleDemo>(PrivateConstructionToken{}, init_ctx, std::move(*pipeline.take()));
}

void
dvk::TriangleDemo::event_handler(const xray::ui::window_event& evt)
{
    if (is_input_event(evt)) {
        if (evt.event.key.keycode == KeySymbol::escape) {
            _quit_receiver();
            return;
        }
    }
}

void
dvk::TriangleDemo::loop_event(const app::RenderEvent& render_event)
{
    static constexpr const auto sixty_herz = std::chrono::duration<float, std::milli>{ 1000.0f / 60.0f };

    _timer.end();
    const auto elapsed_duration = std::chrono::duration<float, std::milli>{ _timer.ts_end() - _timer.ts_start() };

    if (elapsed_duration > sixty_herz) {
        _angle += 0.025f;
        if (_angle >= xray::math::two_pi<float>)
            _angle -= xray::math::two_pi<float>;
        _timer.update_and_reset();
    }

    const FrameRenderData frt{ render_event.renderer->begin_rendering() };

    const VkViewport viewport{
        .x = 0.0f,
        .y = static_cast<float>(frt.fbsize.height),
        .width = static_cast<float>(frt.fbsize.width),
        .height = -static_cast<float>(frt.fbsize.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(frt.cmd_buf, 0, 1, &viewport);

    const VkRect2D scissor{
        .offset = VkOffset2D{ 0, 0 },
        .extent = frt.fbsize,
    };

    vkCmdSetScissor(frt.cmd_buf, 0, 1, &scissor);

    render_event.renderer->clear_attachments(frt.cmd_buf, 1.0f, 0.0f, 1.0f, frt.fbsize.width, frt.fbsize.height);

    const xray::math::scalar2x3<float> r = xray::math::R2::rotate(_angle);
    const std::array<float, 4> vkmtx{ r.a00, r.a01, r.a10, r.a11 };

    vkCmdBindPipeline(frt.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeline_handle());
    vkCmdPushConstants(frt.cmd_buf,
                       _pipeline.layout_handle(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       static_cast<uint32_t>(vkmtx.size() * sizeof(float)),
                       vkmtx.data());
    vkCmdDraw(frt.cmd_buf, 3, 1, 0, 0);

    render_event.renderer->end_rendering();
}
