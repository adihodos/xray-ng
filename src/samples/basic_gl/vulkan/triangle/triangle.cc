#include "triangle.hpp"

#include <string_view>

#include "xray/rendering/vulkan.renderer/vulkan.pipeline.builder.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"

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
}

xray::base::unique_pointer<app::DemoBase>
dvk::TriangleDemo::create(const app::init_context_t& init_ctx)
{
    static constexpr const char* const VERTEX_SHADER = R"#(
#version 460 core

const vec2 TriangleVertices[] = vec2[](
	vec2(0.5, 0.0),
	vec2(1.0, 1.0),
	vec2(0.0, 1.0)
);

const vec3 TriangleColors[] = vec3[](
	vec3(1.0, 0.0, 0.0),
	vec3(0.0, 1.0, 0.0),
	vec3(0.0, 0.0, 1.0)
);

out gl_PerVertex {
	layout (location = 0) gl_Position;
};

out VS_OUT_FS_IN {
	vec3 color;
} vs_out;

void main() {
	gl_Position = vec4(TriangleVertices[gl_VertexIndex], 0.0, 1.0);
	vs_out.color = TriangleColors[gl_VertexIndex];
}
			)#";

    static constexpr const char* const FRAGMENT_SHADER = R"#(
#version 460 core

in VS_OUT_FS_IN {
	vec3 color;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
	FinalFragColor = fs_in.color;
}
    )#";

    tl::optional<GraphicsPipeline> pipeline{ GraphicsPipelineBuilder{}
                                                 .add_shader(ShaderStage::Vertex, string_view{ VERTEX_SHADER })
                                                 .add_shader(ShaderStage::Fragment, string_view{ FRAGMENT_SHADER })
                                                 .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
                                                 .create(*init_ctx.renderer) };

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
}
