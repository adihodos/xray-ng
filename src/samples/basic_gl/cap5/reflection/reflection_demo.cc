#include "cap5/reflection/reflection_demo.hpp"
#include "helpers.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/basic_timer.hpp"
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
#include "xray/rendering/opengl/scoped_state.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include "xray/scene/camera.hpp"
#include <algorithm>
#include <imgui/imgui.h>
#include <vector>

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

app::reflection_demo::reflection_demo() { init(); }

app::reflection_demo::~reflection_demo() {}

void app::reflection_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  {
    const GLuint bound_textures[] = {raw_handle(_skybox)};
    gl::BindTextures(0, 1, bound_textures);

    const GLuint bound_samplers[] = {raw_handle(_sampler_skybox)};
    gl::BindSamplers(0, 1, bound_samplers);
  }

  struct matrix_pack_t {
    float4x4 world;
    float4x4 world_normal;
    float4x4 world_view_projection;
    float3   eye_pos;
  };

  {
    scoped_triangle_winding set_front_cw{gl::CW};

    gl::BindVertexArray(raw_handle(_vertex_arr));
    const auto skybox_world = R4::translate(dc.active_camera->origin());
    const matrix_pack_t skybox_tf_pack{skybox_world, float4x4::stdc::identity,
                                       dc.proj_view_matrix * skybox_world,
                                       dc.active_camera->origin()};

    _draw_prog.set_uniform_block("transform_pack", skybox_tf_pack);
    _draw_prog.set_subroutine_uniform(graphics_pipeline_stage::vertex,
                                      "reflection_func", "skybox_func");
    _draw_prog.set_subroutine_uniform(graphics_pipeline_stage::fragment,
                                      "reflect_surface_fn", "reflect_skybox");

    _draw_prog.bind_to_pipeline();
    gl::DrawElements(gl::TRIANGLES, _skybox_mesh_index_count, gl::UNSIGNED_INT,
                     nullptr);
  }

  {
    const matrix_pack_t object_tf_pack{
        float4x4::stdc::identity, float4x4::stdc::identity, dc.proj_view_matrix,
        dc.active_camera->origin()};

    _draw_prog.set_uniform_block("transform_pack", object_tf_pack);
    _draw_prog.set_subroutine_uniform(graphics_pipeline_stage::vertex,
                                      "reflection_func",
                                      "reflect_surface_func");
    _draw_prog.set_subroutine_uniform(graphics_pipeline_stage::fragment,
                                      "reflect_surface_fn", "reflect_object");
    _draw_prog.set_uniform_block("reflection_params", _surface_params);

    _draw_prog.bind_to_pipeline();
    _spacecraft.draw();
  }
}

void app::reflection_demo::update(const float delta_ms) {
  XR_UNUSED_ARG(delta_ms);
}

void app::reflection_demo::key_event(const int32_t key_code,
                                     const int32_t action, const int32_t mods) {

  XR_UNUSED_ARG(key_code);
  XR_UNUSED_ARG(action);
  XR_UNUSED_ARG(mods);
}

void app::reflection_demo::init() {
  {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, "shaders/cap5/reflection/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap5/reflection/shader.frag")};

    _draw_prog = gpu_program{compiled_shaders};
    if (!_draw_prog) {
      XR_LOG_CRITICAL("Failed to compile/link shaders/program!");
      return;
    }
  }

  {
    geometry_data_t mesh;
    geometry_factory::geosphere(512.0f, 3, &mesh);
    _skybox_mesh_index_count = mesh.index_count;

    vector<vertex_pn> vertices;
    vertices.reserve(mesh.vertex_count);

    transform(raw_ptr(mesh.geometry),
              raw_ptr(mesh.geometry) + mesh.vertex_count,
              back_inserter(vertices), [](const auto& vs_in) {
                return vertex_pn{vs_in.position, vs_in.position};
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
  }

  _vertex_arr =
      [ vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer) ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
    gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pn));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);

    gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_, 0);
    gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pn, normal));

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);

    gl::BindVertexArray(0);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, 0);

    return vao;
  }
  ();

  _skybox = []() {
    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_CUBE_MAP, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGB8, 1024, 1024);

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
        {"spacesky/sky_back6.png", gl::TEXTURE_CUBE_MAP_NEGATIVE_Z, 5}

    };

    for (const auto& cube_face : cube_faces) {
      texture_loader tex_ldr{
          c_str_ptr(xr_app_config->texture_path(cube_face.file_name))
          /*texture_load_options::flip_y*/};

      if (!tex_ldr)
        return GLuint{};

      gl::TextureSubImage3D(texh, 0, 0, 0, cube_face.layer, tex_ldr.width(),
                            tex_ldr.height(), 1, gl::RGB, gl::UNSIGNED_BYTE,
                            tex_ldr.data());
    }

    return texh;
  }();

  _sampler_skybox = []() {
    GLuint smph{};
    gl::CreateSamplers(1, &smph);
    gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_EDGE);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_R, gl::CLAMP_TO_EDGE);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_EDGE);

    return smph;
  }();

  constexpr const char* const MODEL_FILE = "starfury/sa23_aurora.obj";

  xray::base::timer_highp ld_timer;

  XR_LOG_INFO("Mesh load time (old mode) (milliseconds) {}",
              ld_timer.elapsed_millis());

  ld_timer.start();
  _spacecraft = simple_mesh{vertex_format::pn,
                            xr_app_config->model_path(MODEL_FILE).c_str()};
  ld_timer.end();

  if (!_spacecraft) {
    XR_LOG_ERR("Failed to import spacecraft model !");
    return;
  }

  XR_LOG_INFO("Mesh load time (new mode) (milliseconds) {}",
              ld_timer.elapsed_millis());

  _valid = true;
}

void app::reflection_demo::compose_ui() {
  ImGui::Begin("Parameters");
  ImGui::SetNextWindowPos(ImVec2{0.0f, 400.0f}, ImGuiSetCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2{200.0f, 200.0f}, ImGuiSetCond_FirstUseEver);
  ImGui::SliderFloat("Reflectivity", &_surface_params._reflectivity, 0.0f, 1.0f,
                     "%3.3f", 1.0f);
  ImGui::Separator();
  ImGui::SliderFloat("Red", &_surface_params._color.r, 0.0f, 1.0f, "%3.3f",
                     1.0f);
  ImGui::SliderFloat("Green", &_surface_params._color.g, 0.0f, 1.0f, "%3.3f",
                     1.0f);
  ImGui::SliderFloat("Blue", &_surface_params._color.b, 0.0f, 1.0f, "%3.3f",
                     1.0f);
  ImGui::End();
}
