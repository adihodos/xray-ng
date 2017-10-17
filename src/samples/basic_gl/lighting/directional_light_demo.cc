#include "lighting/directional_light_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/objects/ray3.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
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

  if (!_drawnormals) {
    return;
  }

  const auto r0 = ray3f32{{0.0f, 0.0f, 0.0f}, vec3f::stdc::unit_y};
  const auto r1 = ray3f32::construct::from_points({10.0f, 0.0f, 5.0f},
                                                  {100.0f, -10.0f, 2.0f});

  const auto p = r0.eval(20.0f);

  _lights[0].direction = normalize(_demo_opts.lightdir);
  _lights[0].ka        = color_palette::web::black;
  _lights[0].kd        = color_palette::web::white;
  _lights[0].ks        = color_palette::web::orange_red;

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

  //_meshes[obj_type::teapot] =
  //  basic_mesh{xr_app_config->model_path(MODEL_FILE).c_str()};

  {
    geometry_data_t blob;
    geometry_factory::torus(16.0f, 8.0f, 32u, 64u, &blob);
    vector<vertex_pnt> vertices;
    transform(raw_ptr(blob.geometry),
              raw_ptr(blob.geometry) + blob.vertex_count,
              back_inserter(vertices),
              [](const vertex_pntt& vsin) {
                return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
              });

    _meshes[obj_type::teapot] = basic_mesh{vertices.data(),
                                           vertices.size(),
                                           raw_ptr(blob.indices),
                                           blob.index_count};
  }

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
  _objects[obj_type::teapot].pos.y += _meshes[obj_type::ripple].aabb().height();

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

  struct {
    directional_light lights[8];
    uint32_t          lightscount;
    float             specular_intensity;
  } scene_lights;

  scene_lights.lightscount = 1;
  scene_lights.lights[0]   = _lights[0];
  scene_lights.lights[0].direction =
    normalize(mul_vec(draw_ctx.view_matrix, _demo_opts.lightdir));
  scene_lights.specular_intensity = _demo_opts.specular_intensity;

  _fs.set_uniform_block("LightData", scene_lights);

  _pipeline.use();

  gl::Disable(gl::CULL_FACE);
  gl::DrawElements(
    gl::TRIANGLES, ripple->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

  auto teapot = &_objects[obj_type::teapot];
  gl::BindVertexArray(teapot->mesh->vertex_array());
  gl::BindTextureUnit(0, raw_handle(_objtex[obj_type::teapot]));

  const auto pitch      = quaternionf{teapot->rotation.x, vec3f::stdc::unit_x};
  const auto roll       = quaternionf{teapot->rotation.z, vec3f::stdc::unit_z};
  const auto yaw        = quaternionf{teapot->rotation.y, vec3f::stdc::unit_y};
  const auto qrot       = yaw * pitch * roll;
  const auto teapot_rot = rotation_matrix(qrot);

  const auto teapot_world =
    R4::translate(teapot->pos) * teapot_rot * mat4f{R3::scale(teapot->scale)};

  vs_ubo.world_view_proj = draw_ctx.proj_view_matrix * teapot_world;
  vs_ubo.normals         = draw_ctx.view_matrix * teapot_rot;

  _vs.set_uniform_block("TransformMatrices", vs_ubo);
  _vs.flush_uniforms();

  gl::Enable(gl::CULL_FACE);
  scoped_winding_order_setting faces_cw{gl::CW};

  gl::DrawElements(
    gl::TRIANGLES, teapot->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

  if (_demo_opts.drawnormals) {
    _drawnormals.draw(draw_ctx,
                      *teapot->mesh,
                      teapot_world,
                      color_palette::web::green,
                      color_palette::web::green);
  }
}

void app::directional_light_demo::update(const float /* delta_ms */) {
  auto teapot = &_objects[obj_type::teapot];

  if (_demo_opts.rotate_x) {
    teapot->rotation.x += _demo_opts.rotate_speed;
    if (teapot->rotation.x > two_pi<float>) {
      teapot->rotation.x -= two_pi<float>;
    }
  }

  if (_demo_opts.rotate_y) {
    teapot->rotation.y += _demo_opts.rotate_speed;

    if (teapot->rotation.y > two_pi<float>) {
      teapot->rotation.y -= two_pi<float>;
    }
  }

  if (_demo_opts.rotate_z) {
    teapot->rotation.z += _demo_opts.rotate_speed;

    if (teapot->rotation.z > two_pi<float>) {
      teapot->rotation.z -= two_pi<float>;
    }
  }
}

void app::directional_light_demo::event_handler(
  const xray::ui::window_event& evt) {
  if (evt.type == event_type::key) {
    if (evt.event.key.keycode == key_sym::e::backspace) {
      _demo_opts = decltype(_demo_opts){};
      return;
    }
  }
}

void app::directional_light_demo::compose_ui() {
  ImGui::SetNextWindowPos({0, 0}, ImGuiSetCond_Always);
  ImGui::Begin("Options");

  ImGui::Checkbox("Rotate X", &_demo_opts.rotate_x);
  ImGui::Checkbox("Rotate Y", &_demo_opts.rotate_y);
  ImGui::Checkbox("Rotate Z", &_demo_opts.rotate_z);

  ImGui::SliderFloat("Rotation speed", &_demo_opts.rotate_speed, 0.001f, 0.5f);
  ImGui::SliderFloat3(
    "Light direction", _demo_opts.lightdir.components, -1.0f, +1.0f);
  ImGui::SliderFloat(
    "Specular intensity", &_demo_opts.specular_intensity, 1.0f, 256.0f);

  ImGui::Checkbox("Draw surface normals", &_demo_opts.drawnormals);

  ImGui::End();
}
