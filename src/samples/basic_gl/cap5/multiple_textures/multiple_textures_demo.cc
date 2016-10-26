#include "cap5/multiple_textures/multiple_textures_demo.hpp"
#include "helpers.hpp"
#include "std_assets.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
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
#include "xray/rendering/texture_loader.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::multiple_textures_demo::multiple_textures_demo() { init(); }

app::multiple_textures_demo::~multiple_textures_demo() {}

void app::multiple_textures_demo::draw(
    const xray::rendering::draw_context_t& dc) {
  assert(valid());

  gl::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

  gl::BindVertexArray(raw_handle(_vertex_array));

  const GLuint bound_textures[] = {raw_handle(_base_tex),
                                   raw_handle(_overlay_tex)};
  gl::BindTextures(0, 2, bound_textures);

  const GLuint bound_samplers[] = {raw_handle(_sampler), raw_handle(_sampler)};
  gl::BindSamplers(0, 2, bound_samplers);

  struct matrix_pack {
    mat4f world_view;
    mat4f normals_view;
    mat4f world_view_proj;
  } const object_transforms{dc.view_matrix, dc.view_matrix,
                            dc.proj_view_matrix};

  gl::FrontFace(gl::CW);
  _draw_program.set_uniform_block("obj_transforms", object_transforms);
  _draw_program.set_uniform("base_mtl", 0);
  _draw_program.set_uniform("overlay_mtl", 1);
  _draw_program.bind_to_pipeline();

  gl::DrawElements(gl::TRIANGLES, _mesh_index_count, gl::UNSIGNED_INT, nullptr);
}

void app::multiple_textures_demo::update(const float /*delta_ms*/) {}

void app::multiple_textures_demo::key_event(const int32_t /*key_code*/,
                                            const int32_t /*action*/,
                                            const int32_t /*mods*/) {}

void app::multiple_textures_demo::init() {
  {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER,
                    "shaders/cap5/multiple_textures/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap5/multiple_textures/shader.frag")};

    _draw_program = gpu_program{compiled_shaders};
    if (!_draw_program) {
      XR_LOG_ERR("Failed to compile/link program!");
      return;
    }
  }

  {
    geometry_data_t mesh;
    geometry_factory::box(2.0f, 2.0f, 2.0f, &mesh);

    vector<vertex_pnt> vertices;
    vertices.reserve(mesh.vertex_count);
    transform(raw_ptr(mesh.geometry),
              raw_ptr(mesh.geometry) + mesh.vertex_count,
              back_inserter(vertices), [](const auto& in_vx) {

                const auto texcoord_gl =
                    vec2f{in_vx.texcoords.u, 1.0f - in_vx.texcoords.v};

                return vertex_pnt{in_vx.position, in_vx.normal * (-1.0f),
                                  texcoord_gl};
              });

    _vertex_buffer = [&vertices]() {
      GLuint vbh{};
      gl::CreateBuffers(1, &vbh);
      gl::NamedBufferStorage(vbh, bytes_size(vertices), &vertices[0], 0);

      return vbh;
    }();

    _index_buffer = [&mesh]() {
      GLuint ibh{};
      gl::CreateBuffers(1, &ibh);
      gl::NamedBufferStorage(ibh, mesh.index_count * sizeof(uint32_t),
                             raw_ptr(mesh.indices), 0);

      return ibh;
    }();

    _mesh_index_count = mesh.index_count;
  }

  _vertex_array =
      [ vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer) ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
    gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pnt));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);
    gl::EnableVertexArrayAttrib(vao, 2);

    gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pnt, position));
    gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pnt, normal));
    gl::VertexArrayAttribFormat(vao, 2, 2, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pnt, texcoord));

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);
    gl::VertexArrayAttribBinding(vao, 2, 0);

    return vao;
  }
  ();

  _base_tex = []() {
    texture_loader tex_ldr{
        c_str_ptr(xr_app_config->texture_path("brick1.jpg"))};

    if (!tex_ldr)
      return GLuint{};

    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGB8, tex_ldr.width(), tex_ldr.height());
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          gl::RGB, gl::UNSIGNED_BYTE, tex_ldr.data());
    return texh;
  }();

  _overlay_tex = []() {
    texture_loader tex_ldr{c_str_ptr(xr_app_config->texture_path("moss.png"))};

    if (!tex_ldr)
      return GLuint{};

    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGBA8, tex_ldr.width(), tex_ldr.height());
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          gl::RGBA, gl::UNSIGNED_BYTE, tex_ldr.data());

    return texh;
  }();

  _sampler = []() {
    GLuint smph{};
    gl::CreateSamplers(1, &smph);
    gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return smph;
  }();

  _valid = true;
}
