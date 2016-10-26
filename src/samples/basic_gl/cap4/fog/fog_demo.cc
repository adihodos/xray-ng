#include "cap4/fog/fog_demo.hpp"
#include "helpers.hpp"
#include "material.hpp"
#include "std_assets.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

static constexpr const char* const FOG_IMPLEMENTATIONS[] = {"fog_exponential",
                                                            "fog_linear"};

constexpr uint32_t app::fog_demo::NUM_MESHES;

app::fog_demo::fog_demo() { init(); }

app::fog_demo::~fog_demo() {}

void app::fog_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  gl::BindVertexArray(raw_handle(_vertex_array));

  const auto fog_color = color_palette::material::bluegrey500;
  gl::ClearColor(fog_color.r, fog_color.g, fog_color.b, fog_color.a);
  gl::Clear(gl::COLOR_BUFFER_BIT);

  const fog_params fp{fog_color, 1.0f, 20.0f, 0.2f};
  _draw_prog.set_uniform_block("scene_fog", fp);
  const light_source scene_light{mul_point(dc.view_matrix, _light.position),
                                 0.0f, color_palette::material::black,
                                 color_palette::material::white,
                                 color_palette::material::white};

  _draw_prog.set_uniform_block("scene_lights", scene_light);
  _draw_prog.set_subroutine_uniform(graphics_pipeline_stage::fragment,
                                    "fog_type",
                                    FOG_IMPLEMENTATIONS[_linear_fog]);

  struct matrix_pack_t {
    mat4f world_view;
    mat4f normal_view;
    mat4f world_view_proj;
  };

  for (const auto& mesh : _meshes) {
    matrix_pack_t obj_matrix_pack{
        dc.view_matrix * R4::translate(mesh.translation),
        dc.view_matrix * R4::translate(mesh.translation),
        dc.proj_view_matrix * R4::translate(mesh.translation),
    };

    _draw_prog.set_uniform_block("object_transforms", obj_matrix_pack);
    _draw_prog.set_uniform_block("object_material",
                                 material::stdc::red_plastic);
    _draw_prog.bind_to_pipeline();

    gl::DrawElements(gl::TRIANGLES, _mesh_index_count, gl::UNSIGNED_INT,
                     nullptr);
  }
}

void app::fog_demo::update(const float /*delta_ms*/) {}

void app::fog_demo::key_event(const int32_t key_code, const int32_t action,
                              const int32_t /*mods*/) {

  if ((key_code == GLFW_KEY_UP) && (action == GLFW_PRESS)) {
    _linear_fog = !_linear_fog;
  }
}

void app::fog_demo::init() {
  {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, "shaders/cap4/fog/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER, "shaders/cap4/fog/shader.frag"),
    };

    _draw_prog = gpu_program{compiled_shaders};
    if (!_draw_prog) {
      XR_LOG_ERR("Failed to create GL program !");
      return;
    }
  }

  geometry_data_t mesh;
  if (!geometry_factory::load_model(&mesh, MODEL_FILE_SA23,
                                    mesh_import_options::remove_points_lines)) {
    geometry_factory::torus(0.7f, 0.3f, 30, 30, &mesh);
  }

  _mesh_index_count = mesh.index_count;

  vector<vertex_pn> vertices;
  vertices.reserve(mesh.vertex_count);
  transform(raw_ptr(mesh.geometry), raw_ptr(mesh.geometry) + mesh.vertex_count,
            back_inserter(vertices), [](const auto& in_vertex) {
              return vertex_pn{in_vertex.position, in_vertex.normal};
            });

  _vertex_buffer = [&vertices]() {
    GLuint vbh{};
    gl::CreateBuffers(1, &vbh);
    gl::NamedBufferStorage(vbh, bytes_size(vertices), &vertices[0], 0);

    return vbh;
  }();

  _index_buffer = [&mesh]() {
    GLuint ibh;
    gl::CreateBuffers(1, &ibh);
    gl::NamedBufferStorage(ibh, mesh.index_count * sizeof(uint32_t),
                           raw_ptr(mesh.indices), 0);

    return ibh;
  }();

  _vertex_array =
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

  for (uint32_t i = 0; i < NUM_MESHES; ++i) {
    auto& msh = _meshes[i];

    msh.translation = {0.0f, 0.0f, 4.0f * static_cast<float>(i)};
    msh.mat         = material::stdc::copper;
  }

  _light.position = {0.0f, 10.0f, 0.0f};

  _valid = true;
}
