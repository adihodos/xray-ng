#include "lit_torus.hpp"
#include "std_assets.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/colors/color_cast_rgb_variants.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/colors/rgb_variants.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/vertex_format/vertex_pntt.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <opengl/opengl.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace std;

app::lit_object::lit_object() { init(); }

app::lit_object::~lit_object() {}

void app::lit_object::init() {
  assert(!valid());

  {
    geometry_data_t torus_mesh;
    timer_stdp load_tmr{};

    {
      scoped_timing_object<timer_stdp> sto{&load_tmr};

      if (!geometry_factory::load_model(
              &torus_mesh, MODEL_FILE_VIPER,
              mesh_import_options::remove_points_lines)) {
        geometry_factory::torus(0.7f, 0.3f, 30, 30, &torus_mesh);
      }
    }

    OUTPUT_DBG_MSG("Model loaded in %f milliseconds",
                   load_tmr.elapsed_millis());

    // geometry_factory::box(1.0f, 1.0f, 1.0f, &torus_mesh);
    // geometry_factory::tetrahedron(&torus_mesh);
    // geometry_factory::hexahedron(&torus_mesh);
    // geometry_factory::octahedron(&torus_mesh);
    // geometry_factory::dodecahedron(&torus_mesh);
    // geometry_factory::icosahedron(&torus_mesh);

    vector<vertex_pn> torus_vertices{torus_mesh.vertex_count};
    transform(raw_ptr(torus_mesh.geometry),
              raw_ptr(torus_mesh.geometry) + torus_mesh.vertex_count,
              &torus_vertices[0], [](const auto &vx_in) {
                return vertex_pn{vx_in.position, vx_in.normal};
              });

    vbuff_ = make_buffer(gl::ARRAY_BUFFER, gl::MAP_READ_BIT,
                         torus_vertices.size() * sizeof(torus_vertices[0]),
                         &torus_vertices[0]);

    if (!vbuff_)
      return;

    ibuff_ = make_buffer(gl::ELEMENT_ARRAY_BUFFER, gl::MAP_READ_BIT,
                         torus_mesh.index_count * sizeof(uint32_t),
                         raw_ptr(torus_mesh.indices));
    if (!ibuff_)
      return;

    index_count_ = static_cast<uint32_t>(torus_mesh.index_count);
  }

  {
    layout_ = make_vertex_array();
    if (!layout_)
      return;

    gl::BindVertexArray(raw_handle(layout_));
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(ibuff_));
    gl::VertexArrayVertexBuffer(raw_handle(layout_), 0, raw_handle(vbuff_), 0,
                                sizeof(vertex_pn));

    gl::EnableVertexArrayAttrib(raw_handle(layout_), 0);
    gl::EnableVertexArrayAttrib(raw_handle(layout_), 1);
    gl::VertexArrayAttribFormat(raw_handle(layout_), 0, 3, gl::FLOAT,
                                gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pn, position));
    gl::VertexArrayAttribFormat(raw_handle(layout_), 1, 3, gl::FLOAT,
                                gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, normal));

    gl::VertexArrayAttribBinding(raw_handle(layout_), 0, 0);
    gl::VertexArrayAttribBinding(raw_handle(layout_), 1, 0);
  }

  {
    scoped_shader_handle vertex_shader{
        make_shader(gl::VERTEX_SHADER, "shaders/less4/phong_lighting.glsl")};

    if (!vertex_shader)
      return;

    scoped_shader_handle fragment_shader{make_shader(
        gl::FRAGMENT_SHADER, "shaders/less4/frag_passthrough.glsl")};

    if (!fragment_shader)
      return;

    const GLuint shaders[] = {raw_handle(vertex_shader),
                              raw_handle(fragment_shader)};

    draw_prog_ = gpu_program{shaders};
    if (!draw_prog_)
      return;
  }

  // gl::FrontFace(gl::CW);
  valid_ = true;
}

void app::lit_object::draw(const xray::rendering::draw_context_t &draw_ctx) {

  {
    // layout(binding = 1) uniform light_settings {
    //     vec3 eye_pos;
    //     vec3 light_eye_pos;
    //     vec4 light_ka;
    //     vec4 light_kd;
    //     vec4 light_ks;
    // };

    struct light_uniform_t {
      float4 la;
      float4 ld;
      float4 ls;
      float3 lpos;
    } lu;

    lu.lpos = mul_point(draw_ctx.view_matrix, float3{5.0f, 5.0f, 2.0f});
    lu.la = {0.0f, 0.1f, 0.1f, 1.0f};
    lu.ld = {1.0f, 1.0f, 1.0f, 1.0f};
    lu.ls = {1.0f, 1.0f, 1.0f, 1.0f};

    draw_prog_.set_uniform_block("light_settings", lu);
  }

  {
    // layout(binding = 2) uniform material_settings {
    //     vec4 mat_ka;
    //     vec4 mat_kd;
    //     vec4 mat_ks;
    //     float shine_factor;
    // };
    struct material_data_t {
      rgb_color ka;
      rgb_color kd;
      rgb_color ks;
      float shininess;
    } mtl;

    mtl.kd = color_palette::material::amber;
    mtl.ka = {0.9f, 0.5f, 0.3f, 1.0f};
    mtl.ks = {0.8f, 0.8f, 0.8f, 1.0f};
    mtl.shininess = 100.0f;

    draw_prog_.set_uniform_block("material_settings", mtl);
  }

  {
    struct transforms_uniform_t {
      float4x4 model_to_view;
      float4x4 normal_to_view;
      float4x4 model_view_proj;
    } transforms;

    const auto model2view =
        draw_ctx.view_matrix * float4x4{R3::rotate_y(ry_) * R3::rotate_x(rx_)};

    //
    // model to view is just some rotations so use it as it is for
    // normal transform matrix
    transforms.normal_to_view = model2view;
    transforms.model_to_view = model2view;
    transforms.model_view_proj = draw_ctx.projection_matrix * model2view;

    draw_prog_.set_uniform_block("transforms", transforms);
  }

  {
    gl::BindVertexArray(raw_handle(layout_));
    draw_prog_.bind_to_pipeline();
    gl::DrawElements(gl::TRIANGLES, index_count_, gl::UNSIGNED_INT, nullptr);
  }
}

void app::lit_object::update(const float /* delta */) {
  rx_ += 0.02f;
  if (rx_ > two_pi<float>)
    rx_ = 0.0f;

  ry_ += 0.02f;
  if (ry_ > two_pi<float>)
    ry_ = 0.0;
}

void app::lit_object::key_event(const int32_t /*key_code*/,
                                const int32_t /*action*/,
                                const int32_t /*mods*/) noexcept {}
