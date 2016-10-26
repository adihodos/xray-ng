#include "xray/xray.hpp"
#include "cap2/colored_circle/colored_circle_demo.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pt.hpp"
#include "xray/xray_types.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <opengl/opengl.hpp>

using namespace xray::rendering;
using namespace xray::math;
using namespace std;

static const vertex_pt quad_vertices[] = {{{-0.8f, -0.8f, 0.0f}, {0.0f, 0.0f}},
                                          {{0.8f, -0.8f, 0.0f}, {1.0f, 0.0f}},
                                          {{0.8f, 0.8f, 0.0f}, {1.0f, 1.0f}},
                                          {{-0.8f, 0.8f, 0.0f}, {0.0f, 1.0f}}};

static constexpr uint16_t quad_indices[] = {0, 1, 2, 0, 2, 3};

app::colored_circle_demo::colored_circle_demo() { init(); }

void app::colored_circle_demo::init() noexcept {
  using namespace xray;
  using namespace xray::base;
  using namespace xray::rendering;

  vertex_buff_ = []() {
    GLuint vbh{};
    gl::CreateBuffers(1, &vbh);
    gl::NamedBufferStorage(
      vbh, (GLsizei) sizeof(quad_vertices), quad_vertices, 0);

    return vbh;
  }();

  index_buff_ = []() {
    GLuint ibh{};
    gl::CreateBuffers(1, &ibh);
    gl::NamedBufferStorage(
      ibh, (GLsizei) sizeof(quad_indices), quad_indices, 0);
    return ibh;
  }();

  layout_desc_ =
    [ vb = raw_handle(vertex_buff_), ib = raw_handle(index_buff_) ]() {

    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::VertexArrayVertexBuffer(vao, 0, vb, 0, (GLsizei) sizeof(vertex_pt));
    gl::VertexArrayElementBuffer(vao, ib);

    gl::VertexArrayAttribFormat(
      vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pt, position));
    gl::VertexArrayAttribFormat(
      vao, 1, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pt, texcoord));
    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);
    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);
    return vao;
  }
  ();

  {
    _vs = gpu_program_builder{}
            .add_file("shaders/cap2/colored_circle/vertex_shader.glsl")
            .build<graphics_pipeline_stage::vertex>();

    if (!_vs)
      return;

    _fs = gpu_program_builder{}
            .add_file("shaders/cap2/colored_circle/frag_shader.glsl")
            .build<graphics_pipeline_stage::fragment>();

    if (!_fs)
      return;

    _pipeline = program_pipeline{[]() {
      GLuint phandle{};
      gl::CreateProgramPipelines(1, &phandle);
      return phandle;
    }()};

    _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);
  }

  _valid = true;
}

void app::colored_circle_demo::draw(
  const xray::rendering::draw_context_t& ctx) {

  using namespace xray::base;

  constexpr auto clear_color = color_palette::web::black;

  gl::ViewportIndexedf(
    0, 0.0f, 0.0f, (float) ctx.window_width, (float) ctx.window_height);
  gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, clear_color.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0xffffffff);

  // // constexpr float outer_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
  // constexpr float inner_color[] = {1.0f, 1.0f, 0.75f, 1.0f};
  constexpr float inner_radius = 0.25f;
  constexpr float outer_radius = 0.45f;

  _fs.set_uniform("InnerColor", color_palette::web::spring_green.components)
    .set_uniform("OuterColor", clear_color)
    .set_uniform("RadiusInner", inner_radius)
    .set_uniform("RadiusOuter", outer_radius);
  _pipeline.use();

  gl::BindVertexArray(raw_handle(layout_desc_));
  gl::DrawElements(
    gl::TRIANGLES, XR_U32_COUNTOF__(quad_indices), gl::UNSIGNED_SHORT, nullptr);
}

void app::colored_circle_demo::update(const float /* delta_ms */) {}

void app::colored_circle_demo::event_handler(
  const xray::ui::window_event& /* evt */) {}

void app::colored_circle_demo::compose_ui() {}
