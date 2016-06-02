#include "cap5/normal_map/normal_map_demo.hpp"
#include "helpers.hpp"
#include "xray/base/app_config.hpp"
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
#include "xray/rendering/texture_loader.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <vector>

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

app::normal_map_demo::normal_map_demo() { init(); }

app::normal_map_demo::~normal_map_demo() {}

void app::normal_map_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  gl::BindVertexArray(raw_handle(_vertex_array));

  const GLuint bound_textures[] = {raw_handle(_diffuse_map),
                                   raw_handle(_normal_map)};
  gl::BindTextures(0, 2, bound_textures);

  const GLuint bound_samplers[] = {raw_handle(_sampler), raw_handle(_sampler)};
  gl::BindSamplers(0, 2, bound_samplers);

  struct matrix_pack {
    float4x4 world_view;
    float4x4 normal_view;
    float4x4 wvp;
  } const obj_transforms{dc.view_matrix, dc.view_matrix, dc.proj_view_matrix};

  gl::UseProgram(_draw_program.handle());
  _draw_program.set_uniform_block("obj_transforms", obj_transforms);

  const light_source sl{mul_point(dc.view_matrix, float3{0.0f, 15.0f, 15.0f}),
                        0.0f, color_palette::material::white,
                        color_palette::material::white,
                        color_palette::material::white};

  _draw_program.set_uniform_block("scene_lights", sl);
  _draw_program.set_uniform("diffuse_map", 0);
  _draw_program.set_uniform("normal_map", 1);
  _draw_program.bind_to_pipeline();

  gl::DrawElements(gl::TRIANGLES, _mesh_index_count, gl::UNSIGNED_INT, nullptr);
}

void app::normal_map_demo::update(const float /*delta_ms*/) {}

void app::normal_map_demo::key_event(const int32_t /*key_code*/,
                                     const int32_t /*action*/,
                                     const int32_t /*mods*/) {}

void app::normal_map_demo::init() {

  {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, "shaders/cap5/normal_map/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap5/normal_map/shader.frag")};

    _draw_program = gpu_program{compiled_shaders};
    if (!_draw_program) {
      XR_LOG_ERR("Failed to compile/link shaders/program.");
      return;
    }
  }

  {
    geometry_data_t mesh;
    if (!geometry_factory::load_model(
            &mesh, xr_app_config->model_path("ogre/ogre.obj"),
            mesh_import_options::remove_points_lines)) {
      XR_LOG_ERR("Failed to load mesh !");
      return;
    }

    _vertex_buffer = [&mesh]() {
      GLuint vbuff{};
      gl::CreateBuffers(1, &vbuff);
      gl::NamedBufferStorage(
          vbuff, bytes_size(raw_ptr(mesh.geometry), mesh.vertex_count),
          raw_ptr(mesh.geometry), 0);

      return vbuff;
    }();

    _index_buffer = [&mesh]() {
      GLuint ibuff{};
      gl::CreateBuffers(1, &ibuff);
      gl::NamedBufferStorage(
          ibuff, bytes_size(raw_ptr(mesh.indices), mesh.index_count),
          raw_ptr(mesh.indices), 0);

      return ibuff;
    }();

    _mesh_index_count = mesh.index_count;
  }

  _vertex_array =
      [ vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer) ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
    gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pntt));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);
    gl::EnableVertexArrayAttrib(vao, 2);
    gl::EnableVertexArrayAttrib(vao, 3);

    gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pntt, position));
    gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pntt, normal));
    gl::VertexArrayAttribFormat(vao, 2, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pntt, tangent));
    gl::VertexArrayAttribFormat(vao, 3, 2, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pntt, texcoords));

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);
    gl::VertexArrayAttribBinding(vao, 2, 0);
    gl::VertexArrayAttribBinding(vao, 3, 0);

    return vao;
  }
  ();

  _diffuse_map = []() {
    GLuint         texh{};
    texture_loader tex_ldr{xr_app_config->texture_path("ogre/ogre_diffuse.png"),
                           texture_load_options::flip_y};
    if (!tex_ldr)
      return texh;

    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGB8, tex_ldr.width(), tex_ldr.height());
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          gl::RGB, gl::UNSIGNED_BYTE, tex_ldr.data());
    return texh;
  }();

  _normal_map = []() {
    GLuint         texh{};
    texture_loader tex_ldr{
        xr_app_config->texture_path("ogre/ogre_normalmap.png")};
    if (!tex_ldr)
      return texh;

    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGB8, tex_ldr.width(), tex_ldr.height());
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          gl::RGB, gl::UNSIGNED_BYTE, tex_ldr.data());

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
