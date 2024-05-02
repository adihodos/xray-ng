#include "misc/fractals/fractal_demo.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/vertex_format/vertex_pc.hpp"
#include "xray/rendering/vertex_format/vertex_pntt.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/user_interface.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <imgui/imgui.h>
#include <iterator>
#include <opengl/opengl.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

struct nice_shape_def
{
    vec2f pos;
    const char* text;
};

#define NICE_SHAPE_ENTRY(x, y)                                                                                         \
    {                                                                                                                  \
        vec2f{ x, y }, "[" #x ", " #y "]"                                                                              \
    }

static constexpr nice_shape_def NICE_SHAPES[] = {
    NICE_SHAPE_ENTRY(-0.7f, +0.27015f),    NICE_SHAPE_ENTRY(+0.400f, +0.0f),    NICE_SHAPE_ENTRY(-0.8f, +0.156f),
    NICE_SHAPE_ENTRY(+0.285f, +0.0f),      NICE_SHAPE_ENTRY(-0.4f, +0.6f),      NICE_SHAPE_ENTRY(+0.285f, +0.01f),
    NICE_SHAPE_ENTRY(-0.70176f, -0.3842f), NICE_SHAPE_ENTRY(-0.835f, -0.2321f), NICE_SHAPE_ENTRY(-0.74543f, +0.11301f),
    NICE_SHAPE_ENTRY(-0.75f, +0.11f),      NICE_SHAPE_ENTRY(-0.1f, +0.651f),    NICE_SHAPE_ENTRY(-1.36f, +0.11f),
    NICE_SHAPE_ENTRY(+0.32f, +0.043f),     NICE_SHAPE_ENTRY(-0.391f, -0.587f),  NICE_SHAPE_ENTRY(-0.7f, -0.3f),
    NICE_SHAPE_ENTRY(-0.75f, -0.2f),       NICE_SHAPE_ENTRY(-0.75f, 0.15f),     NICE_SHAPE_ENTRY(-0.7f, 0.35f)
};

static constexpr uint32_t kIterations[] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

tl::optional<app::demo_bundle_t>
app::fractal_demo::create(const init_context_t& initContext)
{
    geometry_data_t quad_mesh;
    geometry_factory::fullscreen_quad(&quad_mesh);

    vector<vertex_pc> quad_vertices;
    transform(raw_ptr(quad_mesh.geometry),
              raw_ptr(quad_mesh.geometry) + quad_mesh.vertex_count,
              back_inserter(quad_vertices),
              [](const auto& vs_in) {
                  vertex_pc vs_out;
                  vs_out.position = vs_in.position;
                  return vs_out;
              });

    scoped_buffer quad_vb{ [&quad_vertices]() {
        GLuint vbh{};
        gl::CreateBuffers(1, &vbh);
        gl::NamedBufferStorage(
            vbh, (GLsizeiptr)(quad_vertices.size() * sizeof(quad_vertices[0])), &quad_vertices[0], 0);

        return vbh;
    }() };

    if (!quad_vb)
        return tl::nullopt;

    scoped_buffer quad_ib{ [qm = &quad_mesh]() {
        GLuint ib{};
        gl::CreateBuffers(1, &ib);
        gl::NamedBufferStorage(ib, (GLsizeiptr)(sizeof(uint32_t) * qm->index_count), raw_ptr(qm->indices), 0);
        return ib;
    }() };

    if (!quad_ib)
        return tl::nullopt;

    scoped_vertex_array quad_layout{ [vb = raw_handle(quad_vb), ib = raw_handle(quad_ib)]() {
        GLuint vao{};
        gl::CreateVertexArrays(1, &vao);
        gl::VertexArrayVertexBuffer(vao, 0, vb, 0, (GLsizei)sizeof(vertex_pc));
        gl::VertexArrayElementBuffer(vao, ib);

        gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pc, position));
        gl::EnableVertexArrayAttrib(vao, 0);
        gl::VertexArrayAttribBinding(vao, 0, 0);

        return vao;
    }() };

    if (!quad_layout)
        return tl::nullopt;

    vertex_program vs{
        gpu_program_builder{}.add_file("shaders/misc/fractals/fractal_vert.glsl").build<render_stage::e::vertex>()
    };

    if (!vs)
        return tl::nullopt;

    fragment_program fs{
        gpu_program_builder{}.add_file("shaders/misc/fractals/fractal_frag.glsl").build<render_stage::e::fragment>()
    };

    if (!fs)
        return tl::nullopt;

    program_pipeline pipeline{ []() {
        GLuint ppl{};
        gl::CreateProgramPipelines(1, &ppl);
        return ppl;
    }() };

    pipeline.use_vertex_program(vs).use_fragment_program(fs);

    auto demoObj = xray::base::make_unique<fractal_demo>(ConstructToken{},
                                                         initContext,
                                                         move(quad_vb),
                                                         move(quad_ib),
                                                         move(quad_layout),
                                                         move(vs),
                                                         move(fs),
                                                         move(pipeline));
    auto winEventHandler = make_delegate(*demoObj, &fractal_demo::event_handler);
    auto loopEventHandler = make_delegate(*demoObj, &fractal_demo::loop_event);

    return tl::make_optional<demo_bundle_t>(move(demoObj), winEventHandler, loopEventHandler);
}

app::fractal_demo::~fractal_demo() {}

void
app::fractal_demo::loop_event(const xray::ui::window_loop_event& wle)
{
    _ui->tick(1.0f / 60.0f);
    draw(wle.wnd_width, wle.wnd_height);
    draw_ui(wle.wnd_width, wle.wnd_height);
}

void
app::fractal_demo::event_handler(const xray::ui::window_event& evt)
{
    if (is_input_event(evt)) {
        _ui->input_event(evt);
        if (_ui->wants_input()) {
            return;
        }

        if (evt.type == event_type::key && evt.event.key.keycode == key_sym::e::escape) {
            _quit_receiver();
            return;
        }
    }

    //  if (evt.type == event_type::mouse_wheel) {
    //    auto mwe = &evt.event.wheel;
    //  }
}

bool
get_next_shape_description(void*, int32_t idx, const char** shape_desc)
{
    if (idx < XR_I32_COUNTOF(NICE_SHAPES)) {
        *shape_desc = NICE_SHAPES[idx].text;
        return true;
    }

    return false;
}

void
app::fractal_demo::draw(const int32_t surface_w, const int32_t surface_h)
{
    gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, color_palette::web::black.components);
    gl::ViewportIndexedf(0, 0.0f, 0.0f, static_cast<float>(surface_w), static_cast<float>(surface_h));

    _fp.width = static_cast<float>(surface_w);
    _fp.height = static_cast<float>(surface_h);
    _fp.iterations = (uint32_t)_iterations;
    _fp.shape_re = NICE_SHAPES[_shape_idx % XR_COUNTOF(NICE_SHAPES)].pos.x;
    _fp.shape_im = NICE_SHAPES[_shape_idx % XR_COUNTOF(NICE_SHAPES)].pos.y;

    _fs.set_uniform_block("fractal_params", _fp);

    gl::BindVertexArray(raw_handle(_quad_layout));
    _pipeline.use(false);
    _vs.flush_uniforms();
    _fs.flush_uniforms();
    gl::DrawElements(gl::TRIANGLES, 6, gl::UNSIGNED_INT, nullptr);
}

void
app::fractal_demo::draw_ui(const int32_t surface_w, const int32_t surface_h)
{

    _ui->new_frame(surface_w, surface_h);

    ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Appearing);
    if (ImGui::Begin("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders)) {
        ImGui::Combo("Shapes",
                     &_shape_idx,
                     &get_next_shape_description,
                     nullptr,
                     XR_I32_COUNTOF(NICE_SHAPES),
                     XR_I32_COUNTOF(NICE_SHAPES) / 2);

        ImGui::SliderInt("Iterations", &_iterations, 16, 4096, nullptr);
    }

    ImGui::End();
    _ui->draw();
}
