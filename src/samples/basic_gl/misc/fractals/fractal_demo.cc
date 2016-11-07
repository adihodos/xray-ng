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

struct nice_shape_def {
  vec2f       pos;
  const char* text;
};

#define NICE_SHAPE_ENTRY(x, y)                                                 \
  { vec2f{x, y}, "[" #x ", " #y "]" }

static constexpr nice_shape_def NICE_SHAPES[] = {
  NICE_SHAPE_ENTRY(-0.7f, +0.27015f),
  NICE_SHAPE_ENTRY(+0.400f, +0.0f),
  NICE_SHAPE_ENTRY(-0.8f, +0.156f),
  NICE_SHAPE_ENTRY(+0.285f, +0.0f),
  NICE_SHAPE_ENTRY(-0.4f, +0.6f),
  NICE_SHAPE_ENTRY(+0.285f, +0.01f),
  NICE_SHAPE_ENTRY(-0.70176f, -0.3842f),
  NICE_SHAPE_ENTRY(-0.835f, -0.2321f),
  NICE_SHAPE_ENTRY(-0.74543f, +0.11301f),
  NICE_SHAPE_ENTRY(-0.75f, +0.11f),
  NICE_SHAPE_ENTRY(-0.1f, +0.651f),
  NICE_SHAPE_ENTRY(-1.36f, +0.11f),
  NICE_SHAPE_ENTRY(+0.32f, +0.043f),
  NICE_SHAPE_ENTRY(-0.391f, -0.587f),
  NICE_SHAPE_ENTRY(-0.7f, -0.3f),
  NICE_SHAPE_ENTRY(-0.75f, -0.2f),
  NICE_SHAPE_ENTRY(-0.75f, 0.15f),
  NICE_SHAPE_ENTRY(-0.7f, 0.35f)};

static constexpr uint32_t kIterations[] = {
  16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

app::fractal_demo::fractal_demo() { init(); }

app::fractal_demo::~fractal_demo() {}

void app::fractal_demo::init() {
  assert(!valid());

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

    _quad_vb = [&quad_vertices]() {
      GLuint vbh{};
      gl::CreateBuffers(1, &vbh);
      gl::NamedBufferStorage(
        vbh,
        (GLsizeiptr)(quad_vertices.size() * sizeof(quad_vertices[0])),
        &quad_vertices[0],
        0);

      return vbh;
    }();

    _quad_ib = [qm = &quad_mesh]() {
      GLuint ib{};
      gl::CreateBuffers(1, &ib);
      gl::NamedBufferStorage(ib,
                             (GLsizeiptr)(sizeof(uint32_t) * qm->index_count),
                             raw_ptr(qm->indices),
                             0);
      return ib;
    }
    ();

    if (!_quad_ib)
      return;
  }

  _quad_layout = [ vb = raw_handle(_quad_vb), ib = raw_handle(_quad_ib) ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::VertexArrayVertexBuffer(vao, 0, vb, 0, (GLsizei) sizeof(vertex_pc));
    gl::VertexArrayElementBuffer(vao, ib);

    gl::VertexArrayAttribFormat(
      vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pc, position));
    gl::EnableVertexArrayAttrib(vao, 0);
    gl::VertexArrayAttribBinding(vao, 0, 0);

    return vao;
  }
  ();

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/fractals/fractal_vert.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/misc/fractals/fractal_frag.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);
  _valid = true;
}

void app::fractal_demo::draw(const xray::rendering::draw_context_t& draw_ctx) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ViewportIndexedf(0,
                       0.0f,
                       0.0f,
                       (float) draw_ctx.window_width,
                       (float) draw_ctx.window_height);

  _fp.width      = static_cast<float>(draw_ctx.window_width);
  _fp.height     = static_cast<float>(draw_ctx.window_height);
  _fp.iterations = (uint32_t) _iterations;
  _fp.shape_re   = NICE_SHAPES[_shape_idx % XR_COUNTOF__(NICE_SHAPES)].pos.x;
  _fp.shape_im   = NICE_SHAPES[_shape_idx % XR_COUNTOF__(NICE_SHAPES)].pos.y;

  _fs.set_uniform_block("fractal_params", _fp);

  gl::BindVertexArray(raw_handle(_quad_layout));
  _pipeline.use();
  gl::DrawElements(gl::TRIANGLES, 6, gl::UNSIGNED_INT, nullptr);
}

void app::fractal_demo::update(const float /* delta_ms */) {}

void app::fractal_demo::event_handler(const xray::ui::window_event& evt) {
  if (evt.type == event_type::mouse_wheel) {
    auto mwe = &evt.event.wheel;
  }
}

bool get_next_shape_description(void*, int32_t idx, const char** shape_desc) {
  if (idx < (int32_t) XR_COUNTOF__(NICE_SHAPES)) {
    *shape_desc = NICE_SHAPES[idx].text;
    return true;
  }

  return false;
}

void app::fractal_demo::compose_ui() {
  ImGui::SetNextWindowPos({0, 0}, ImGuiSetCond_Always);
  ImGui::Begin("Options");
  ImGui::Combo("Shapes",
               &_shape_idx,
               &get_next_shape_description,
               nullptr,
               (int32_t) XR_COUNTOF__(NICE_SHAPES),
               (int32_t) XR_COUNTOF__(NICE_SHAPES) / 2);

  ImGui::SliderInt("Iterations", &_iterations, 32, 4096, nullptr);
  ImGui::End();
}
