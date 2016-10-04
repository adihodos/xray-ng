#include "fractal.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pc.hpp"
#include "xray/rendering/vertex_format/vertex_pntt.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <opengl/opengl.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace std;

static constexpr float2 kNiceShapes[] = {
    float2{-0.7f, +0.27015f},     float2{+0.400f, +0.0f},
    float2{-0.8f, +0.156f},       float2{+0.285f, +0.0f},
    float2{-0.4f, +0.6f},         float2{+0.285f, +0.01f},
    float2{-0.70176f, -0.3842f},  float2{-0.835f, -0.2321f},
    float2{-0.74543f, +0.11301f}, float2{-0.75f, +0.11f},
    float2{-0.1f, +0.651f},       float2{-1.36f, +0.11f},
    float2{+0.32f, +0.043f},      float2{-0.391f, -0.587f},
    float2{-0.7f, -0.3f},         float2{-0.75f, -0.2f},
    float2{-0.75f, 0.15f},        float2{-0.7f, 0.35f}};

static constexpr uint32_t kIterations[] = {16,  32,   64,   128, 256,
                                           512, 1024, 2048, 4096};

app::fractal_painter::fractal_painter() { init(); }

app::fractal_painter::~fractal_painter() {}

void app::fractal_painter::init() {
  assert(!valid());

  {
    geometry_data_t quad_mesh;
    geometry_factory::fullscreen_quad(&quad_mesh);

    vector<vertex_pc> quad_vertices;
    transform(raw_ptr(quad_mesh.geometry),
              raw_ptr(quad_mesh.geometry) + quad_mesh.vertex_count,
              back_inserter(quad_vertices), [](const auto& vs_in) {
                vertex_pc vs_out;
                vs_out.position = vs_in.position;
                return vs_out;
              });

    constexpr float4 quad_colors[] = {{1.0f, 0.0f, 0.0f, 1.0f},
                                      {0.0f, 1.0f, 0.0f, 1.0f},
                                      {0.0f, 0.0f, 1.0f, 1.0f},
                                      {1.0f, 1.0f, 0.0f, 1.0f}};

    for (size_t idx = 0, vertex_count = quad_vertices.size();
         idx < vertex_count; ++idx) {
      //      quad_vertices[idx].color = quad_colors[idx];
    }

    quad_vb_ = make_buffer(gl::ARRAY_BUFFER, 0,
                           quad_vertices.size() * sizeof(quad_vertices[0]),
                           &quad_vertices[0]);

    if (!quad_vb_)
      return;

    quad_ib_ = make_buffer(gl::ELEMENT_ARRAY_BUFFER, 0,
                           quad_mesh.index_count * sizeof(uint32_t),
                           raw_ptr(quad_mesh.indices));

    if (!quad_ib_)
      return;
  }

  {
    quad_layout_ = make_vertex_array();
    if (!quad_layout_)
      return;

    gl::BindVertexArray(raw_handle(quad_layout_));
    gl::BindVertexBuffer(0, raw_handle(quad_vb_), 0, sizeof(vertex_pc));
    gl::EnableVertexAttribArray(0);
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribFormat(0, 3, gl::FLOAT, gl::FALSE_,
                           XR_U32_OFFSETOF(vertex_pc, position));
    gl::VertexAttribFormat(1, 4, gl::FLOAT, gl::FALSE_,
                           XR_U32_OFFSETOF(vertex_pc, color));
    gl::VertexAttribBinding(0, 0);
    gl::VertexAttribBinding(1, 0);
  }

  {
    scoped_shader_handle vertex_shader{
        make_shader(gl::VERTEX_SHADER, "shaders/fractal_passthrough.glsl")};

    if (!vertex_shader)
      return;

    scoped_shader_handle fragment_shader{
        make_shader(gl::FRAGMENT_SHADER, "shaders/fractal_frag.glsl")};

    if (!fragment_shader)
      return;

    const GLuint attached_shaders[] = {raw_handle(vertex_shader),
                                       raw_handle(fragment_shader)};

    quad_draw_prg_ = gpu_program{attached_shaders};
    if (!quad_draw_prg_)
      return;
  }

  initialized_ = true;
}

void app::fractal_painter::draw(const xray::ui::window_context& wnd_ctx) {
  assert(valid());

  fp_.width      = static_cast<float>(wnd_ctx.wnd_width);
  fp_.height     = static_cast<float>(wnd_ctx.wnd_height);
  fp_.iterations = kIterations[iter_sel_];
  fp_.shape_re   = kNiceShapes[shape_idx_ % XR_COUNTOF__(kNiceShapes)].x;
  fp_.shape_im   = kNiceShapes[shape_idx_ % XR_COUNTOF__(kNiceShapes)].y;

  quad_draw_prg_.set_uniform_block("fractal_params", fp_);

  gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(quad_ib_));
  gl::BindVertexArray(raw_handle(quad_layout_));
  quad_draw_prg_.bind_to_pipeline();

  gl::DrawElements(gl::TRIANGLES, 6, gl::UNSIGNED_INT, nullptr);
}

void app::fractal_painter::key_press(const int32_t key_code) {
  if (key_code == GLFW_KEY_UP) {
    ++shape_idx_;
    return;
  }

  if (key_code == GLFW_KEY_RIGHT) {
    if (iter_sel_ < (XR_COUNTOF__(kIterations) - 1)) {
      ++iter_sel_;
    }

    return;
  }

  if (key_code == GLFW_KEY_LEFT) {
    if (iter_sel_ >= 1) {
      --iter_sel_;
    }

    return;
  }
}
