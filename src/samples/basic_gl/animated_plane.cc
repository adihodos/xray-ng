#include "animated_plane.hpp"
#include "animated_plane_model.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/opengl/shader_base.hpp"

void app::animated_paper_plane::init() noexcept {
  using namespace xray::base;
  using namespace xray::rendering;

  {
    vertex_buff_ = make_buffer();
    if (!vertex_buff_)
      return;

    gl::BindBuffer(gl::ARRAY_BUFFER, raw_handle(vertex_buff_));
    gl::BufferData(gl::ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices,
                   gl::STATIC_DRAW);
  }

  {
    index_buff_ = make_buffer();
    if (!index_buff_)
      return;

    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buff_));
    gl::BufferData(gl::ELEMENT_ARRAY_BUFFER, sizeof(plane_indices),
                   plane_indices, gl::STATIC_DRAW);
  }

  {
    layout_desc_ = make_vertex_array();
    if (!layout_desc_)
      return;

    gl::BindVertexArray(raw_handle(layout_desc_));
    gl::EnableVertexAttribArray(0);

    gl::BindVertexBuffer(0, raw_handle(vertex_buff_), 0,
                         sizeof(xray::math::float2));

    gl::VertexAttribFormat(0, 2, gl::FLOAT, gl::FALSE_, 0);
    gl::VertexAttribBinding(0, 0);
  }

  {
    scoped_shader_handle vertex_shader{
        make_shader(gl::VERTEX_SHADER, "shaders/vertex_coloring.glsl")};

    scoped_shader_handle fragment_shader{
        make_shader(gl::FRAGMENT_SHADER, "shaders/basic_coloring.glsl")};

    const GLuint shaders[] = {raw_handle(vertex_shader),
                              raw_handle(fragment_shader)};

    drawing_program_ = gpu_program{shaders};
    if (!drawing_program_)
      return;
  }

  valid_ = true;
}

void app::animated_paper_plane::update(const xray::scalar_lowp) noexcept {
  using namespace xray::math;

  rotation_angle_ += 0.01f;
  if (rotation_angle_ > two_pi<float>)
    rotation_angle_ = 0.0f;
}

void app::animated_paper_plane::draw(
    const xray::rendering::draw_context_t&) noexcept {
  using namespace xray::math;

  assert(valid());

  gl::Clear(gl::COLOR_BUFFER_BIT);
  gl::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buff_));
  gl::BindVertexArray(raw_handle(layout_desc_));
  gl::UseProgram(drawing_program_.handle());

  drawing_program_.set_uniform("vertex_color", float3{1.0f, 0.4f, 0.8f});

  const auto  rotate_mtx = R3::rotate_z(rotation_angle_);
  const float values[] = {rotate_mtx.a00, rotate_mtx.a01, rotate_mtx.a02, 0.0f,
                          rotate_mtx.a10, rotate_mtx.a11, rotate_mtx.a12, 0.0f,
                          rotate_mtx.a20, rotate_mtx.a21, rotate_mtx.a22, 0.0f};

  drawing_program_.set_uniform_block("transforms", values, sizeof(values));
  drawing_program_.bind_to_pipeline();

  gl::DrawElements(gl::LINES, XR_U32_COUNTOF__(plane_indices),
                   gl::UNSIGNED_SHORT, nullptr);
}
