#include "cap4/toon_shading/toon_shading_demo.hpp"
#include "helpers.hpp"
#include "material.hpp"
#include "std_assets.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <algorithm>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

const uint32_t app::toon_shading_demo::NUM_LIGHTS;

app::toon_shading_demo::toon_shading_demo() { init(); }

app::toon_shading_demo::~toon_shading_demo() {}

void app::toon_shading_demo::init() {
  //
  // Shaders
  {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER,
                    "shaders/cap4/toon_shading/vert_shader.glsl"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap4/toon_shading/frag_shader.glsl")};

    _draw_prog = gpu_program{compiled_shaders};
    if (!_draw_prog) {
      XR_LOG_ERR("Failed to create and link drawing program !");
      return;
    }
  }

  //
  // Vertex + index buffers
  {
    geometry_data_t mesh;
    geometry_factory::torus(0.7f, 0.3f, 30, 30, &mesh);

    vector<vertex_pn> vertices;
    vertices.reserve(mesh.vertex_count);
    transform(raw_ptr(mesh.geometry),
              raw_ptr(mesh.geometry) + mesh.vertex_count,
              back_inserter(vertices), [](const auto& in_vert) {
                return vertex_pn{in_vert.position, in_vert.normal};
              });

    _vertex_buffer = [&vertices]() {
      GLuint vbuff{};
      gl::CreateBuffers(1, &vbuff);
      gl::NamedBufferStorage(vbuff, bytes_size(vertices), &vertices[0], 0);

      return vbuff;
    }();

    _mesh_index_cnt = mesh.index_count;
    _index_buffer   = [&mesh]() {
      GLuint ibuff{};
      gl::CreateBuffers(1, &ibuff);
      gl::NamedBufferStorage(ibuff, mesh.index_count * sizeof(uint32_t),
                             raw_ptr(mesh.indices), 0);

      return ibuff;
    }();
  }

  //
  // Vertex array object.
  _vertex_array_obj =
      [ vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer) ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);

    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
    gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pn));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);

    gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pn, position));
    gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pn, normal));

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);

    return vao;
  }
  ();

  {
    for (uint32_t idx = 0; idx < toon_shading_demo::NUM_LIGHTS; ++idx) {
      const auto x_pos =
          2.0f * std::cos((two_pi<float> / 5.0f) * static_cast<float>(idx));
      const auto z_pos =
          2.0f * std::sin((two_pi<float> / 5.0f) * static_cast<float>(idx));
      _lights[idx].position = vec3f{x_pos, 1.2f, z_pos + 1.0f};
    }

    _lights[0].intensity = {0.9f, 0.9f, 0.9f};
    _lights[1].intensity = {0.9f, 0.9f, 0.9f};
    _lights[2].intensity = {0.9f, 0.9f, 0.9f};
    _lights[3].intensity = {0.9f, 0.9f, 0.9f};
  }

  _valid = true;
}

void app::toon_shading_demo::draw(
    const xray::rendering::draw_context_t& dc) noexcept {

  assert(valid());

  gl::BindVertexArray(raw_handle(_vertex_array_obj));

  {
    //
    // Set shader, bla bla

    light_source3 scene_light{_lights[0]};
    scene_light.position = {10.0f * std::cos(_light_angle), 10.0f,
                            10.0f * std::sin(_light_angle)};
    scene_light.position = mul_point(dc.view_matrix, scene_light.position);
    //    transform(begin(_lights), end(_lights), begin(scene_lights),
    //              [&dc](const auto &in_light) {
    //                auto out_light = in_light;
    //                out_light.position =
    //                    mul_point(dc.view_matrix, in_light.position);

    //                return out_light;
    //              });

    _draw_prog.set_uniform_block("light_source_pack", scene_light);
    _draw_prog.set_uniform_block("object_material", material_ad::stdc::copper);

    struct matrix_pack {
      mat4f world_to_view;
      mat4f normals_to_view;
      mat4f world_view_proj;
    } const obj_transforms_pack{dc.view_matrix, dc.view_matrix,
                                dc.proj_view_matrix};

    _draw_prog.set_uniform_block("transform_matrix_pack", obj_transforms_pack);
  }

  _draw_prog.bind_to_pipeline();
  gl::DrawElements(gl::TRIANGLES, _mesh_index_cnt, gl::UNSIGNED_INT, nullptr);
}

void app::toon_shading_demo::update(const float delta_ms) noexcept {
  _light_angle += 0.015f;

  if (_light_angle > two_pi<float>)
    _light_angle -= two_pi<float>;
}

void app::toon_shading_demo::key_event(const int32_t key_code,
                                       const int32_t action,
                                       const int32_t mods) noexcept {}
