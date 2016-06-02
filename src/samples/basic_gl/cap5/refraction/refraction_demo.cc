#include "cap5/refraction/refraction_demo.hpp"
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
#include "xray/rendering/opengl/scoped_state.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include "xray/scene/camera.hpp"
#include <algorithm>
#include <imgui/imgui.h>
#include <vector>

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

app::refraction_demo::refraction_demo() { init(); }

app::refraction_demo::~refraction_demo() {}

void app::refraction_demo::compose_ui() {
  ImGui::Begin("Surface parameters", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize);
  //  ImGui::SetNextWindowPos(ImVec2{0.0f, 400.0f}, ImGuiSetCond_FirstUseEver);
  //  ImGui::SetNextWindowSize(ImVec2{500.0f, 250.0f},
  //  ImGuiSetCond_FirstUseEver);

  {
    const bool val_changed =
        ImGui::SliderFloat("Refractivity", &_surfaceparams.refract_surface,
                           0.0f, 1.0f, "%3.3f", 1.0f);

    if (val_changed)
      _surfaceparams.reflect_surface = 1.0f - _surfaceparams.refract_surface;
  }

  {
    const auto val_changed =
        ImGui::SliderFloat("Reflectivity", &_surfaceparams.reflect_surface,
                           0.0f, 1.0f, "%3.3f", 1.0f);

    if (val_changed)
      _surfaceparams.refract_surface = 1.0f - _surfaceparams.reflect_surface;
  }

  ImGui::Checkbox("Draw skybox as wired mesh", &_skybox_wiremesh);
  ImGui::End();
}

void app::refraction_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  {
    gl::BindTextureUnit(0, raw_handle(_skybox_texture));
    gl::BindSampler(0, raw_handle(_skybox_sampler));
  }

  struct matrix_pack {
    float4x4 model_world;
    float4x4 normal_world;
    float4x4 world_view_proj;
  };

  //
  // draw skybox
  {
    scoped_triangle_winding set_cw_winding{gl::CW};

    if (_skybox_wiremesh) {
      gl::Disable(gl::CULL_FACE);
      gl::PolygonMode(gl::FRONT_AND_BACK, gl::LINE);
    }

    scoped_vertex_array_binding skybox_vao_binding{
        raw_handle(_vertexarray_skybox)};

    const auto skybox_world_tf = R4::translate(dc.active_camera->origin());
    const matrix_pack skybox_matrices{skybox_world_tf, skybox_world_tf,
                                      dc.proj_view_matrix * skybox_world_tf};

    _drawprogram.set_uniform_block("transforms_pack", skybox_matrices);
    _drawprogram.set_subroutine_uniform(
        pipeline_stage::vertex, "surface_process_func", "process_skybox");
    _drawprogram.set_subroutine_uniform(pipeline_stage::fragment,
                                        "color_surface_func", "color_skybox");
    _drawprogram.set_uniform("environment_map", 0);
    _drawprogram.bind_to_pipeline();
    gl::DrawElements(gl::TRIANGLES, _skybox_indexcount, gl::UNSIGNED_INT,
                     nullptr);

    if (_skybox_wiremesh) {
      gl::Enable(gl::CULL_FACE);
      gl::PolygonMode(gl::FRONT_AND_BACK, gl::FILL);
    }
  }

  {
    const auto        spacecraft_world_tf = float4x4::stdc::identity;
    const matrix_pack spacecraft_matrices{
        spacecraft_world_tf, spacecraft_world_tf,
        dc.proj_view_matrix * spacecraft_world_tf};

    _drawprogram.set_uniform_block("transforms_pack", spacecraft_matrices);
    _drawprogram.set_uniform("material_eta", _surfaceparams.refract_surface);
    _drawprogram.set_uniform("eye_pos", dc.active_camera->origin());
    _drawprogram.set_subroutine_uniform(
        pipeline_stage::vertex, "surface_process_func", "process_surface");
    _drawprogram.set_uniform("surface_reflection",
                             _surfaceparams.reflect_surface);
    _drawprogram.set_uniform("environment_map", 0);
    _drawprogram.set_subroutine_uniform(pipeline_stage::fragment,
                                        "color_surface_func", "color_surface");
    _drawprogram.bind_to_pipeline();

    _spacecraft.draw();
  }
}

void app::refraction_demo::update(const float /*delta_ms*/) {}

void app::refraction_demo::key_event(const int32_t /*key_code*/,
                                     const int32_t /*action*/,
                                     const int32_t /*mods*/) {}

void app::refraction_demo::init() {

  _drawprogram = []() {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, "shaders/cap5/refraction/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap5/refraction/shader.frag")};

    return gpu_program{compiled_shaders};
  }();

  if (!_drawprogram)
    return;

  {
    geometry_data_t skybox_mesh;
    geometry_factory::geosphere(512.0f, 5, &skybox_mesh);
    _skybox_indexcount = skybox_mesh.index_count;

    vector<vertex_pn> vertices;
    vertices.reserve(skybox_mesh.vertex_count);

    transform(raw_ptr(skybox_mesh.geometry),
              raw_ptr(skybox_mesh.geometry) + skybox_mesh.vertex_count,
              back_inserter(vertices), [](const auto& vs_in) {
                return vertex_pn{vs_in.position, vs_in.normal};
              });

    _vertexbuffer_skybox = [&vertices, buff_size = bytes_size(vertices) ]() {
      GLuint vbuff{};
      gl::CreateBuffers(1, &vbuff);
      gl::NamedBufferStorage(vbuff, buff_size, &vertices[0], 0);

      return vbuff;
    }
    ();

    _indexbuffer_skybox = [
      data = raw_ptr(skybox_mesh.indices),
      buff_size =
          bytes_size(raw_ptr(skybox_mesh.indices), skybox_mesh.index_count)
    ]() {
      GLuint ibuff{};
      gl::CreateBuffers(1, &ibuff);
      gl::NamedBufferStorage(ibuff, buff_size, data, 0);

      return ibuff;
    }
    ();
  }

  _vertexarray_skybox = [
    vb = raw_handle(_vertexbuffer_skybox), ib = raw_handle(_indexbuffer_skybox)
  ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);

    scoped_vertex_array_binding vao_binding{vao};
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ib);
    gl::VertexArrayVertexBuffer(vao, 0, vb, 0, sizeof(vertex_pn));

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
    constexpr const char* const MODEL_FILE = "starfury/sa23_aurora.obj";
    //    geometry_data_t             mesh;
    //    geometry_factory::geosphere(12.0f, 3, &mesh);

    _spacecraft = simple_mesh{vertex_format::pn,
                              //            mesh
                              xr_app_config->model_path(MODEL_FILE)};
    if (!_spacecraft) {
      XR_LOG_ERR("Failed to import model!");
      return;
    }
  }

  _skybox_texture = []() {

    struct tex_name_and_target {
      const char* file_name;
      GLenum      target;
      GLint       layer;
    } const cube_faces[] = {
        {"spacesky/sky_right1.png", gl::TEXTURE_CUBE_MAP_POSITIVE_X, 0},
        {"spacesky/sky_left2.png", gl::TEXTURE_CUBE_MAP_NEGATIVE_X, 1},
        {"spacesky/sky_top3.png", gl::TEXTURE_CUBE_MAP_POSITIVE_Y, 2},
        {"spacesky/sky_bottom4.png", gl::TEXTURE_CUBE_MAP_NEGATIVE_Y, 3},
        {"spacesky/sky_front5.png", gl::TEXTURE_CUBE_MAP_POSITIVE_Z, 4},
        {"spacesky/sky_back6.png", gl::TEXTURE_CUBE_MAP_NEGATIVE_Z, 5}};

    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_CUBE_MAP, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGB8, 1024, 1024);

    for (const auto& face : cube_faces) {
      texture_loader tex_ldr{xr_app_config->texture_path(face.file_name),
                             texture_load_options::none};
      if (!tex_ldr) {
        XR_LOG_ERR("Failed to load texture {}", face.file_name);
        return GLuint{};
      }

      gl::TextureSubImage3D(texh, 0, 0, 0, face.layer, tex_ldr.width(),
                            tex_ldr.height(), 1, gl::RGB, gl::UNSIGNED_BYTE,
                            tex_ldr.data());
    }

    return texh;
  }();

  if (!_skybox_texture) {
    XR_LOG_ERR("Failed to load cubemap !");
    return;
  }

  _skybox_sampler = []() {
    GLuint smph{};
    gl::CreateSamplers(1, &smph);
    gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_R, gl::CLAMP_TO_EDGE);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);

    return smph;
  }();

  _valid = true;
}
