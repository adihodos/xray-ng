#include "cap4/directional_lights/directional_lights_demo.hpp"
#include "helpers.hpp"
#include "material.hpp"
#include "std_assets.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <algorithm>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace std;

const uint32_t app::directional_light_demo::NUM_LIGHTS;

app::directional_light_demo::directional_light_demo() { init(); }

app::directional_light_demo::~directional_light_demo() {}

void app::directional_light_demo::init() {
  {
    geometry_data_t mesh;
    if (!geometry_factory::load_model(
            &mesh, MODEL_FILE_SA23, mesh_import_options::remove_points_lines)) {
      geometry_factory::torus(0.7f, 0.3f, 30, 30, &mesh);
    }

    vector<vertex_pn> vertices;
    vertices.reserve(mesh.vertex_count);

    transform(raw_ptr(mesh.geometry),
              raw_ptr(mesh.geometry) + mesh.vertex_count,
              back_inserter(vertices), [](const auto& vs_in) {
                return vertex_pn{vs_in.position, vs_in.normal};
              });

    _vertex_buffer = [&vertices]() {
      GLuint handle{};
      gl::CreateBuffers(1, &handle);
      gl::NamedBufferStorage(handle, bytes_size(vertices), &vertices[0], 0);

      return handle;
    }();

    if (!_vertex_buffer) {
      XR_LOG_CRITICAL("Failed to create vertex buffer!");
      return;
    }

    _index_buffer = [&mesh]() {
      GLuint handle{};
      gl::CreateBuffers(1, &handle);
      gl::NamedBufferStorage(handle, mesh.index_count * sizeof(uint32_t),
                             raw_ptr(mesh.indices), 0);

      return handle;
    }();

    _index_count = mesh.index_count;
  }

  {
    _vertex_array = [
      vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer)
    ]() {
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
  }

  {
    const GLuint shaders[] = {
        make_shader(gl::VERTEX_SHADER,
                    "shaders/cap4/directional_lights/vert_shader.glsl"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap4/directional_lights/frag_shader.glsl")};

    _draw_program = gpu_program{shaders};
    if (!_draw_program) {
      XR_LOG_CRITICAL("Failed to create drawing program!");
      return;
    }
  }

  {
    //
    // Setup lights.
    {
      for (uint32_t idx = 0; idx < directional_light_demo::NUM_LIGHTS; ++idx) {
        const auto x_pos =
            2.0f * std::cos((two_pi<float> / 5.0f) * static_cast<float>(idx));
        const auto z_pos =
            2.0f * std::sin((two_pi<float> / 5.0f) * static_cast<float>(idx));
        _lights[idx].direction = -normalize(vec3f{x_pos, 1.2f, z_pos + 1.0f});
      }

      _lights[0].ka = {0.75f, 0.75f, 0.75f, 1.0f};
      _lights[0].kd = {1.0f, 1.0f, 1.0f, 1.0f};
      _lights[0].ks = {1.0f, 1.0f, 1.0f, 1.0f};

      _lights[1].ka = {0.75f, 0.75f, 0.75f, 1.0f};
      _lights[1].kd = {1.0f, 1.0f, 1.0f, 1.0f};
      _lights[1].ks = {1.0f, 1.0f, 1.0f, 1.0f};

      _lights[2].ka = {0.1f, 0.1f, 0.1f, 1.0f};
      _lights[2].kd = {1.0f, 1.0f, 1.0f, 1.0f};
      _lights[2].ks = {1.0f, 1.0f, 1.0f, 1.0f};

      _lights[3].ka = {0.1f, 0.1f, 0.1f, 1.0f};
      _lights[3].kd = {1.0f, 1.0f, 1.0f, 1.0f};
      _lights[3].ks = {1.0f, 1.0f, 1.0f, 1.0f};
    }
  }

  _valid = true;
}

void app::directional_light_demo::draw(
    const xray::rendering::draw_context_t& dc) noexcept {

  assert(valid());
  gl::BindVertexArray(raw_handle(_vertex_array));

  {
    directional_light scene_lights[NUM_LIGHTS];
    transform(begin(_lights), end(_lights), begin(scene_lights),
              [&dc](const auto& in_light) {
                auto out_light = in_light;
                out_light.direction =
                    normalize(mul_vec(dc.view_matrix, in_light.direction));
                return out_light;
              });

    _draw_program.set_uniform_block("scene_lights", scene_lights);
    _draw_program.set_uniform_block("obj_mat", material::stdc::copper);

    struct transform_matrix_pack {
      mat4f world_to_view;
      mat4f normals_to_view;
      mat4f world_view_proj;
    } const mtx_uniform{dc.view_matrix, dc.view_matrix, dc.proj_view_matrix};

    _draw_program.set_uniform_block("transforms", mtx_uniform);
  }

  _draw_program.bind_to_pipeline();
  gl::DrawElements(gl::TRIANGLES, _index_count, gl::UNSIGNED_INT, nullptr);
}

void app::directional_light_demo::update(const float /*delta_ms*/) noexcept {}

void app::directional_light_demo::key_event(const int32_t /*key_code*/,
                                            const int32_t /*action*/,
                                            const int32_t /*mods*/) noexcept {}
