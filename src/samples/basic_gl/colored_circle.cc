#include "colored_circle.hpp"
#include "xray/xray.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pt.hpp"
#include "xray/xray_types.hpp"
#include <cstdint>
#include <cstring>
#include <opengl/opengl.hpp>

using namespace xray::rendering;
using namespace xray::math;

static const vertex_pt quad_vertices[] = {{{-0.8f, -0.8f, 0.0f}, {0.0f, 0.0f}},
                                          {{0.8f, -0.8f, 0.0f}, {1.0f, 0.0f}},
                                          {{0.8f, 0.8f, 0.0f}, {1.0f, 1.0f}},
                                          {{-0.8f, 0.8f, 0.0f}, {0.0f, 1.0f}}};

static constexpr uint16_t quad_indices[] = {0, 1, 2, 0, 2, 3};

app::colored_circle::colored_circle()
    : layout_desc_{xray::rendering::make_vertex_array()} {
  init();
}

void app::colored_circle::init() noexcept {
  using namespace xray;
  using namespace xray::base;
  using namespace xray::rendering;

  {
    vertex_buff_ = scoped_buffer{
        make_buffer(gl::ARRAY_BUFFER, 0, sizeof(quad_vertices), quad_vertices)};

    if (!vertex_buff_)
      return;
  }

  {

    index_buff_ = scoped_buffer{make_buffer(
        gl::ELEMENT_ARRAY_BUFFER, 0, sizeof(quad_indices), quad_indices)};

    if (!index_buff_)
      return;
  }

  {
    if (!layout_desc_)
      return;

    gl::BindVertexArray(raw_handle(layout_desc_));
    gl::EnableVertexAttribArray(0);
    gl::EnableVertexAttribArray(1);

    gl::BindVertexBuffer(0, raw_handle(vertex_buff_), 0, sizeof(vertex_pt));
    gl::VertexAttribFormat(0, 3, gl::FLOAT, gl::FALSE_,
                           XR_U32_OFFSETOF(vertex_pt, position));
    gl::VertexAttribFormat(1, 2, gl::FLOAT, gl::FALSE_,
                           XR_U32_OFFSETOF(vertex_pt, texcoord));

    gl::VertexAttribBinding(0, 0);
    gl::VertexAttribBinding(1, 0);
  }

  {
    scoped_shader_handle vert{
        make_shader(gl::VERTEX_SHADER, "shaders/less2/vertex_shader.glsl")};

    if (!vert)
      return;

    scoped_shader_handle frag{
        make_shader(gl::FRAGMENT_SHADER, "shaders/less2/frag_shader.glsl")};

    const GLuint shader_handles[] = {raw_handle(vert), raw_handle(frag)};

    drawing_program_ = gpu_program{shader_handles};
    if (!drawing_program_)
      return;

    constexpr scalar_lowp outer_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    constexpr scalar_lowp inner_color[] = {1.0f, 1.0f, 0.75f, 1.0f};
    constexpr scalar_lowp inner_radius  = 0.25f;
    constexpr scalar_lowp outer_radius  = 0.45f;

    drawing_program_.set_uniform("InnerColor", inner_color);
    drawing_program_.set_uniform("OuterColor", outer_color);
    drawing_program_.set_uniform("RadiusInner", inner_radius);
    drawing_program_.set_uniform("RadiusOuter", outer_radius);
  }

  valid_ = true;
}

void app::colored_circle::update(const xray::scalar_lowp /* delta */) noexcept {
}

void app::colored_circle::draw(const xray::ui::window_context&) noexcept {
  using namespace xray::base;

  gl::BindVertexArray(raw_handle(layout_desc_));
  gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buff_));
  drawing_program_.bind_to_pipeline();
  gl::DrawElements(gl::TRIANGLES, XR_U32_COUNTOF__(quad_indices),
                   gl::UNSIGNED_SHORT, nullptr);
}
