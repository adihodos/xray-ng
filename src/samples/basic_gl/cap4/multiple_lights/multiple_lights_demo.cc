#include "cap4/multiple_lights/multiple_lights_demo.hpp"
#include "helpers.hpp"
#include "std_assets.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include "xray/scene/camera.hpp"
#include <algorithm>
#include <cmath>
#include <opengl/opengl.hpp>
#include <span.h>
#include <stlsoft/memory/auto_buffer.hpp>
#include <string_span.h>
#include <type_traits>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

constexpr uint32_t app::multiple_lights_demo::NUM_LIGHTS;

app::multiple_lights_demo::multiple_lights_demo() { init(); }

app::multiple_lights_demo::~multiple_lights_demo() {}

void app::multiple_lights_demo::update(const float /*delta_ms*/) noexcept {
  _rot_angle_x += 0.03f;

  if (_rot_angle_x > two_pi<float>)
    _rot_angle_x -= two_pi<float>;

  _rot_angle_y += 0.015f;
  if (_rot_angle_y > two_pi<float>)
    _rot_angle_y -= two_pi<float>;
}

void app::multiple_lights_demo::key_event(const int32_t /*key_code*/,
                                          const int32_t /*action*/,
                                          const int32_t /*mods*/) noexcept {}


template <typename T>
using auto_buff = stlsoft::auto_buffer<T, 256>;

void app::multiple_lights_demo::init() {

  geometry_data_t obj_meshes[multiple_lights_demo::NUM_MESHES];

  if (!geometry_factory::load_model(&obj_meshes[0], MODEL_FILE_SA23,
                                    mesh_import_options::remove_points_lines)) {
    geometry_factory::torus(0.7f, 0.3f, 30, 30, &obj_meshes[0]);
  }

  geometry_factory::torus(0.7f, 0.3f, 30, 30, &obj_meshes[1]);

  draw_info_[0].base_vertex = 0;
  draw_info_[1].base_vertex = obj_meshes[0].vertex_count;
  draw_info_[0].index_end   = obj_meshes[0].index_count;
  draw_info_[1].index_start = obj_meshes[0].index_count;
  draw_info_[1].index_end =
      obj_meshes[0].index_count + obj_meshes[1].index_count;

  vector<vertex_pn> vertices;
  vertices.reserve(obj_meshes[0].vertex_count + obj_meshes[1].vertex_count);

  gsl::span<vertex_pntt> input_vert_spans[] = {
      {raw_ptr(obj_meshes[0].geometry),
       static_cast<ptrdiff_t>(obj_meshes[0].vertex_count)},
      {raw_ptr(obj_meshes[1].geometry),
       static_cast<ptrdiff_t>(obj_meshes[1].vertex_count)}};

  for (const auto& in_vtx_span : input_vert_spans) {
    transform(begin(in_vtx_span), end(in_vtx_span), back_inserter(vertices),
              [](const auto& in_vertex) {
                return vertex_pn{in_vertex.position, in_vertex.normal};
              });
  }

  {
    GLuint vbuff{0};
    gl::CreateBuffers(1, &vbuff);
    gl::NamedBufferStorage(vbuff, bytes_size(vertices), &vertices[0], 0);
    unique_handle_reset(vertex_buff_, vbuff);
  }

  {
    vector<uint32_t> indices;
    indices.reserve(obj_meshes[0].index_count + obj_meshes[1].index_count);

    {
      const auto in_span = gsl::span<uint32_t>{
          raw_ptr(obj_meshes[0].indices),
          static_cast<ptrdiff_t>(obj_meshes[0].index_count)};
      copy(begin(in_span), end(in_span), back_inserter(indices));
    }

    {
      const auto in_span = gsl::span<uint32_t>{
          raw_ptr(obj_meshes[1].indices),
          static_cast<ptrdiff_t>(obj_meshes[1].index_count)};
      copy(begin(in_span), end(in_span), back_inserter(indices));
    }

    GLuint ibuff{0};
    gl::CreateBuffers(1, &ibuff);
    gl::NamedBufferStorage(ibuff, bytes_size(indices), &indices[0], 0);
    unique_handle_reset(index_buff_, ibuff);
  }

  {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::BindVertexArray(vao);

    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buff_));
    gl::VertexArrayVertexBuffer(vao, 0, raw_handle(vertex_buff_), 0,
                                sizeof(vertex_pn));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);

    gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pn, position));
    gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pn, normal));

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);

    unique_handle_reset(layout_, vao);
  }

  {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER,
                    "shaders/cap4/multiple_lights/vert_shader.glsl"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap4/multiple_lights/frag_shader.glsl")};

    draw_prog_ = gpu_program{compiled_shaders};
    if (!draw_prog_) {
      XR_LOG_CRITICAL("Failed to link drawing program !!!");
      return;
    }
  }

  //
  // Setup lights.
  {
    for (uint32_t idx = 0; idx < multiple_lights_demo::NUM_LIGHTS; ++idx) {
      const auto x_pos =
          2.0f * std::cos((two_pi<float> / 5.0f) * static_cast<float>(idx));
      const auto z_pos =
          2.0f * std::sin((two_pi<float> / 5.0f) * static_cast<float>(idx));
      lights_[idx].position = {x_pos, 1.2f, z_pos + 1.0f};
    }

    lights_[0].intensity = {0.0f, 0.8f, 0.8f};
    lights_[1].intensity = {0.0f, 0.0f, 0.8f};
    lights_[2].intensity = {0.8f, 0.0f, 0.0f};
    lights_[3].intensity = {0.0f, 0.8f, 0.0f};
    lights_[4].intensity = {0.8f, 0.8f, 0.8f};

    //    lights_[0].intensity = {1.0f, 1.0f, 1.0f};
    //    lights_[1].intensity = {1.0f, 1.0f, 1.0f};
    //    lights_[2].intensity = {1.0f, 1.0f, 1.0f};
    //    lights_[3].intensity = {1.0f, 1.0f, 1.0f};
    //    lights_[4].intensity = {1.0f, 1.0f, 1.0f};

    lights_[0].ka = {0.75f, 0.75f, 0.75f, 1.0f};
    lights_[0].kd = {1.0f, 1.0f, 1.0f, 1.0f};
    lights_[0].ks = {1.0f, 1.0f, 1.0f, 1.0f};

    lights_[1].ka = {0.75f, 0.75f, 0.75f, 1.0f};
    lights_[1].kd = {1.0f, 1.0f, 1.0f, 1.0f};
    lights_[1].ks = {1.0f, 1.0f, 1.0f, 1.0f};

    lights_[2].ka = {0.1f, 0.1f, 0.1f, 1.0f};
    lights_[2].kd = {1.0f, 1.0f, 1.0f, 1.0f};
    lights_[2].ks = {1.0f, 1.0f, 1.0f, 1.0f};

    lights_[3].ka = {0.1f, 0.1f, 0.1f, 1.0f};
    lights_[3].kd = {1.0f, 1.0f, 1.0f, 1.0f};
    lights_[3].ks = {1.0f, 1.0f, 1.0f, 1.0f};

    lights_[4].ka = {0.1f, 0.1f, 0.1f, 1.0f};
    lights_[4].kd = {1.0f, 1.0f, 1.0f, 1.0f};
    lights_[4].ks = {1.0f, 1.0f, 1.0f, 1.0f};
  }

  valid_ = true;
}

struct on_off {
  static void on(const GLenum cap) noexcept { gl::Enable(cap); }

  static void off(const GLenum cap) noexcept { gl::Disable(cap); }
};

struct off_on {
  static void off(const GLenum cap) noexcept { gl::Enable(cap); }

  static void on(const GLenum cap) noexcept { gl::Disable(cap); }
};

template <typename mode = on_off>
struct scoped_capability_toggle {
public:
  explicit scoped_capability_toggle(const GLenum cap) noexcept : _cap{cap} {
    mode::on(_cap);
  }

  ~scoped_capability_toggle() noexcept { mode::off(_cap); }

private:
  GLenum _cap;
};

void app::multiple_lights_demo::draw(
    const xray::rendering::draw_context_t& dc) noexcept {

  assert(valid());

  gl::BindVertexArray(raw_handle(layout_));

  //
  // Set shared uniforms
  {
    light_source2 lights[NUM_LIGHTS];
    transform(begin(lights_), end(lights_), begin(lights),
              [vmtx = dc.view_matrix](const light_source2& in_light) {
                auto out_light     = in_light;
                out_light.position = mul_point(vmtx, out_light.position);

                return out_light;
              });

    draw_prog_.set_uniform_block("lighting_info", lights);
    draw_prog_.set_uniform_block("material_info", material::stdc::copper);
  }

  struct obj_transforms {
    float4x4 mv;
    float4x4 nv;
    float4x4 mvp;
  };

  {
    const auto to_world = float4x4::stdc::identity;
    const auto tf_uniform =
        obj_transforms{dc.view_matrix * to_world, dc.view_matrix,
                       dc.proj_view_matrix * to_world};

    draw_prog_.set_uniform_block("material_info", material::stdc::copper);
    draw_prog_.set_uniform_block("transforms", tf_uniform);
    draw_prog_.bind_to_pipeline();

    gl::DrawElementsBaseVertex(
        gl::TRIANGLES, static_cast<GLsizei>(draw_info_[1].index_end -
                                            draw_info_[1].index_start),
        gl::UNSIGNED_INT, offset_cast<uint32_t>(draw_info_[1].index_start),
        static_cast<GLint>(draw_info_[1].base_vertex));
  }

  {
    const auto to_world =
        float4x4{R3::rotate_x(_rot_angle_x) * R3::rotate_y(_rot_angle_y)};

    const auto tf_uniform =
        obj_transforms{dc.view_matrix * to_world, dc.view_matrix * to_world,
                       dc.proj_view_matrix * to_world};

    draw_prog_.set_uniform_block("material_info", material::stdc::gold);
    draw_prog_.set_uniform_block("transforms", tf_uniform);
    draw_prog_.bind_to_pipeline();

    gl::DrawElementsBaseVertex(
        gl::TRIANGLES, static_cast<GLsizei>(draw_info_[0].index_end -
                                            draw_info_[0].index_start),
        gl::UNSIGNED_INT, offset_cast<uint32_t>(draw_info_[0].index_start),
        static_cast<GLint>(draw_info_[0].base_vertex));
  }

  //  auto draw_fn_option_draw = [this]() {

  //    for (const auto &drw_info : draw_info_) {
  //      const GLsizei index_count =
  //          static_cast<GLsizei>(drw_info.index_end - drw_info.index_start);
  //      const auto ib_offset = static_cast<const char *>(nullptr) +
  //                             drw_info.index_start * sizeof(uint32_t);

  //      gl::DrawElementsBaseVertex(gl::TRIANGLES, index_count,
  //      gl::UNSIGNED_INT,
  //                                 ib_offset,
  //                                 static_cast<GLint>(drw_info.base_vertex));
  //    }
  //  };

  //  //  draw_fn_option_draw();

  //  auto draw_fn_option_multidraw = [this]() {
  //    const GLsizei index_count[] = {
  //        static_cast<GLsizei>(draw_info_[0].index_end -
  //                             draw_info_[0].index_start),
  //        static_cast<GLsizei>(draw_info_[1].index_end -
  //                             draw_info_[1].index_start)};

  //    const void *ib_offets[] = {
  //        offset_cast<uint32_t>(draw_info_[0].index_start),
  //        offset_cast<uint32_t>(draw_info_[1].index_start)};

  //    const GLint base_vertices[] = {
  //        static_cast<GLint>(draw_info_[0].base_vertex),
  //        static_cast<GLint>(draw_info_[1].base_vertex)};

  //    gl::MultiDrawElementsBaseVertex(gl::TRIANGLES, index_count,
  //                                    gl::UNSIGNED_INT, ib_offets, 2,
  //                                    base_vertices);
  //  };

  //  draw_fn_option_multidraw();
}
