#include "lighting/directional_light_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <imgui/imgui.h>
#include <iterator>
#include <opengl/opengl.hpp>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::directional_light_demo::directional_light_demo() { init(); }

app::directional_light_demo::~directional_light_demo() {}

void app::directional_light_demo::init() {
  assert(!valid());

  static constexpr auto MODEL_FILE =
    //"SuperCobra.3ds";
    //"f4/f4phantom.obj";
    //"sa23/sa23_aurora.obj";
    //"cube_rounded.obj";
    //"stanford/dragon/dragon.obj";
    "stanford/teapot/teapot.obj";
  //"stanford/sportscar/sportsCar.obj";
  //"stanford/sibenik/sibenik.obj";
  //"stanford/rungholt/rungholt.obj";
  //"stanford/lost-empire/lost_empire.obj";
  //"stanford/cube/cube.obj";
  //"stanford/head/head.OBJ";
  _meshes[obj_type::teapot] =
    basic_mesh{xr_app_config->model_path(MODEL_FILE).c_str()};

  if (!_meshes[obj_type::teapot]) {
    return;
  }

  _objects[obj_type::teapot].scale = 0.085f;

  geometry_data_t blob;
  geometry_factory::grid(16.0f, 16.0f, 128, 128, &blob);

  vector<vertex_pnt> verts;
  transform(raw_ptr(blob.geometry),
            raw_ptr(blob.geometry) + blob.vertex_count,
            back_inserter(verts),
            [](const vertex_pntt& vsin) {
              return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
            });

  const vertex_ripple_parameters rparams{
    1.0f, 3.0f * two_pi<float>, 16.0f, 16.0f};

  vertex_effect::ripple(
    gsl::span<vertex_pnt>{verts},
    gsl::span<uint32_t>{raw_ptr(blob.indices),
                        raw_ptr(blob.indices) + blob.index_count},
    rparams);

  _meshes[obj_type::ripple] = basic_mesh{
    verts.data(), verts.size(), raw_ptr(blob.indices), blob.index_count};

  _objects[obj_type::ripple].mesh = &_meshes[obj_type::ripple];
  _objects[obj_type::teapot].mesh = &_meshes[obj_type::teapot];
  _objects[obj_type::teapot].pos.y +=
    _meshes[obj_type::ripple].aabb().height() * 1.5f;

  _vs = gpu_program_builder{}
          .add_file("shaders/lighting/vs.directional.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/lighting/fs.directional.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

  {
    texture_loader tldr{
      xr_app_config->texture_path("stanford/teapot/default.png").c_str()};

    if (!tldr) {
      return;
    }

    gl::CreateTextures(
      gl::TEXTURE_2D, 1, raw_handle_ptr(_objtex[obj_type::teapot]));

    gl::TextureStorage2D(raw_handle(_objtex[obj_type::teapot]),
                         1,
                         tldr.internal_format(),
                         tldr.width(),
                         tldr.height());
    gl::TextureSubImage2D(raw_handle(_objtex[obj_type::teapot]),
                          0,
                          0,
                          0,
                          tldr.width(),
                          tldr.height(),
                          tldr.format(),
                          gl::UNSIGNED_BYTE,
                          tldr.data());
  }

  {
    gl::CreateTextures(
      gl::TEXTURE_2D, 1, raw_handle_ptr(_objtex[obj_type::ripple]));
    gl::TextureStorage2D(
      raw_handle(_objtex[obj_type::ripple]), 1, gl::RGBA8, 1, 1);
    gl::ClearTexImage(raw_handle(_objtex[obj_type::ripple]),
                      0,
                      gl::RGBA,
                      gl::FLOAT,
                      color_palette::web::red.components);
  }

  gl::CreateSamplers(1, raw_handle_ptr(_sampler));
  const auto smp = raw_handle(_sampler);
  gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
  gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

  gl::Enable(gl::DEPTH_TEST);

  _valid = true;
}

void app::directional_light_demo::draw(
  const xray::rendering::draw_context_t& draw_ctx) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  gl::BindTextureUnit(0, raw_handle(_objtex[obj_type::ripple]));
  gl::BindSampler(0, raw_handle(_sampler));

  struct {
    mat4f world_view_proj;
    mat4f view;
    mat4f normals;
  } vs_ubo;

  auto ripple = &_objects[obj_type::ripple];

  gl::BindVertexArray(ripple->mesh->vertex_array());

  vs_ubo.world_view_proj =
    draw_ctx.proj_view_matrix * R4::translate(ripple->pos);
  vs_ubo.view    = draw_ctx.view_matrix;
  vs_ubo.normals = draw_ctx.view_matrix;

  _vs.set_uniform_block("TransformMatrices", vs_ubo);
  _fs.set_uniform("DIFFUSE_MAP", 0);
  _pipeline.use();

  gl::Disable(gl::CULL_FACE);
  gl::DrawElements(
    gl::TRIANGLES, ripple->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

  auto teapot = &_objects[obj_type::teapot];
  gl::BindVertexArray(teapot->mesh->vertex_array());
  gl::BindTextureUnit(0, raw_handle(_objtex[obj_type::teapot]));

  const auto teapot_world = R4::translate(teapot->pos) *
                            mat4f{R3::rotate_y(teapot->rotation.y)} *
                            mat4f{R3::scale(teapot->scale)};

  vs_ubo.world_view_proj = draw_ctx.proj_view_matrix * teapot_world;
  vs_ubo.normals         = teapot_world;

  _vs.set_uniform_block("TransformMatrices", vs_ubo);
  _vs.flush_uniforms();

  gl::Enable(gl::CULL_FACE);
  gl::DrawElements(
    gl::TRIANGLES, teapot->mesh->index_count(), gl::UNSIGNED_INT, nullptr);
}

void app::directional_light_demo::update(const float /* delta_ms */) {
  auto teapot = &_objects[obj_type::teapot];
  teapot->rotation.y += 0.01f;

  if (teapot->rotation.y > two_pi<float>) {
    teapot->rotation.y -= two_pi<float>;
  }
}

void app::directional_light_demo::event_handler(
  const xray::ui::window_event& /*evt*/) {
  // if (evt.type == event_type::mouse_wheel) {
  //   auto mwe = &evt.event.wheel;
  // }
}

void app::directional_light_demo::compose_ui() {
  ImGui::SetNextWindowPos({0, 0}, ImGuiSetCond_Always);
  ImGui::Begin("Options");
  ImGui::End();
}

struct directional_light {
  vec3f     direction;
  rgb_color ka;
  rgb_color kd;
  rgb_color ks;
};