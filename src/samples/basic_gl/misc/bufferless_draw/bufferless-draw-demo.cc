#include "misc/bufferless_draw/bufferless-draw-demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <imgui/imgui.h>
#include <iterator>
#include <opengl/opengl.hpp>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::bufferless_draw_demo::bufferless_draw_demo()
{
    gl::CreateBuffers(1, raw_handle_ptr(_vb));
    gl::NamedBufferStorage(raw_handle(_vb), 4, nullptr, 0);

    gl::CreateVertexArrays(1, raw_handle_ptr(_vao));
    gl::VertexArrayVertexBuffer(raw_handle(_vao), 0, raw_handle(_vb), 0, 4);
    gl::VertexArrayAttribFormat(raw_handle(_vao), 0, 1, gl::FLOAT, gl::FALSE_, 0);
    gl::VertexArrayAttribBinding(raw_handle(_vao), 0, 0);

    _vs = gpu_program_builder{}.add_file("shaders/misc/bufferless_draw/vs.glsl").build<render_stage::e::vertex>();

    if (!_vs) {
        return;
    }

    _gs = gpu_program_builder{}.add_file("shaders/misc/bufferless_draw/gs.glsl").build<render_stage::e::geometry>();

    if (!_gs) {
        return;
    }

    _fs = gpu_program_builder{}.add_file("shaders/misc/bufferless_draw/fs.glsl").build<render_stage::e::fragment>();

    if (!_fs) {
        return;
    }

    _pipeline = program_pipeline{ []() {
        GLuint pp{};
        gl::CreateProgramPipelines(1, &pp);
        return pp;
    }() };

    _pipeline.use_vertex_program(_vs).use_geometry_program(_gs).use_fragment_program(_fs);

    _valid = true;
}

app::bufferless_draw_demo::~bufferless_draw_demo() {}

void
app::bufferless_draw_demo::draw(const xray::rendering::draw_context_t&)
{
    const float CLEAR_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, CLEAR_COLOR);
    gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

    gl::BindVertexArray(raw_handle(_vao));
    _pipeline.use();
    gl::DrawArrays(gl::POINTS, 0, 2);
}

void
app::bufferless_draw_demo::update(const float delta_ms)
{
}

void
app::bufferless_draw_demo::event_handler(const xray::ui::window_event& evt)
{
}

void
app::bufferless_draw_demo::compose_ui()
{
}
