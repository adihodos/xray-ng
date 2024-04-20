#include "lighting/directional_light_demo.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
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
#include "xray/ui/user_interface.hpp"
#include <cassert>
#include <opengl/opengl.hpp>
#include <span>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::directional_light_demo::directional_light_demo(
  const app::init_context_t& init_ctx)
  : app::demo_base{init_ctx} {

  init();

  _scene.cam.set_projection(projections_rh::perspective_symmetric(
    static_cast<float>(init_ctx.surface_width) /
      static_cast<float>(init_ctx.surface_height),
    radians(70.0f),
    0.1f,
    100.0f));

  _ui->set_global_font("Roboto-Regular");
}

app::directional_light_demo::~directional_light_demo() {}

void app::directional_light_demo::init() {
  assert(!valid());

  _scene.available_meshes.build_list(xr_app_config->model_root().c_str());
  if (_scene.available_meshes.mesh_list().empty()) {
    XR_DBG_MSG("No available meshes!!");
    return;
  }

  if (!_drawnormals) {
    return;
  }

  _lights[0].direction = normalize(_demo_opts.lightdir);
  _lights[0].ka        = color_palette::web::black;
  _lights[0].kd        = color_palette::web::white;
  _lights[0].ks        = color_palette::web::orange_red;

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
    std::span<vertex_pnt>{verts},
    std::span<uint32_t>{raw_ptr(blob.indices),
                        raw_ptr(blob.indices) + blob.index_count},
    rparams);

  _meshes[obj_type::ripple] = basic_mesh{
    verts.data(), verts.size(), raw_ptr(blob.indices), blob.index_count};

  _objects[obj_type::ripple].mesh = &_meshes[obj_type::ripple];
  _objects[obj_type::teapot].mesh = &_meshes[obj_type::teapot];

  switch_mesh(
    _scene.available_meshes.mesh_list()[_scene.current_mesh_idx].path.c_str());

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
    gl::CreateTextures(
      gl::TEXTURE_2D, 1, raw_handle_ptr(_objtex[obj_type::teapot]));

    gl::TextureStorage2D(
      raw_handle(_objtex[obj_type::teapot]), 1, gl::RGBA8, 256, 256);
    gl::ClearTexImage(raw_handle(_objtex[obj_type::teapot]),
                      0,
                      gl::RGBA,
                      gl::FLOAT,
                      _demo_opts.kd_main.components);
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

  _timer.start();
  _valid = true;
}

void app::directional_light_demo::loop_event(
  const xray::ui::window_loop_event& wle) {
  _timer.update_and_reset();
  update(_timer.elapsed_millis());
  draw();
  draw_ui(wle.wnd_width, wle.wnd_height);
}

void app::directional_light_demo::draw() {
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
    _scene.cam.projection_view() * R4::translate(ripple->pos);
  vs_ubo.view    = _scene.cam.projection_view();
  vs_ubo.normals = _scene.cam.projection_view();

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
    normalize(mul_vec(_scene.cam.view(), _demo_opts.lightdir));
  scene_lights.specular_intensity = _demo_opts.specular_intensity;

  _fs.set_uniform_block("LightData", scene_lights);

  _pipeline.use();

  gl::Disable(gl::CULL_FACE);
  gl::DrawElements(
    gl::TRIANGLES, ripple->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

  auto teapot = &_objects[obj_type::teapot];
  if (teapot->mesh) {
    gl::BindVertexArray(teapot->mesh->vertex_array());
    gl::BindTextureUnit(0, raw_handle(_objtex[obj_type::teapot]));

    const auto pitch = quaternionf{teapot->rotation.x, vec3f::stdc::unit_x};
    const auto roll  = quaternionf{teapot->rotation.z, vec3f::stdc::unit_z};
    const auto yaw   = quaternionf{teapot->rotation.y, vec3f::stdc::unit_y};
    const auto qrot  = yaw * pitch * roll;
    const auto teapot_rot = rotation_matrix(qrot);

    const auto teapot_world =
      R4::translate(teapot->pos) * teapot_rot * mat4f{R3::scale(teapot->scale)};

    vs_ubo.world_view_proj = _scene.cam.projection_view() * teapot_world;
    vs_ubo.normals         = _scene.cam.view() * teapot_rot;

    _vs.set_uniform_block("TransformMatrices", vs_ubo);
    _vs.flush_uniforms();

    gl::Enable(gl::CULL_FACE);
    gl::DrawElements(
      gl::TRIANGLES, teapot->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

    if (_demo_opts.drawnormals) {
      const draw_context_t dc{0u,
                              0u,
                              _scene.cam.view(),
                              _scene.cam.projection(),
                              _scene.cam.projection_view(),
                              &_scene.cam,
                              nullptr};

      _drawnormals.draw(dc,
                        *teapot->mesh,
                        R4::translate(teapot->pos) * teapot_rot,
                        color_palette::web::green,
                        color_palette::web::green,
                        _demo_opts.normal_len);
    }
  }

  if (_demo_opts.drawnormals) {
    const draw_context_t dc{0u,
                            0u,
                            _scene.cam.view(),
                            _scene.cam.projection(),
                            _scene.cam.projection_view(),
                            &_scene.cam,
                            nullptr};

    _drawnormals.draw(dc,
                      *ripple->mesh,
                      R4::translate(ripple->pos),
                      color_palette::web::cadet_blue,
                      color_palette::web::cadet_blue,
                      _demo_opts.normal_len);
  }
}

void app::directional_light_demo::update(const float delta_ms) {
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

  _scene.cam_control.update();
  _ui->tick(delta_ms);
}

void app::directional_light_demo::event_handler(
  const xray::ui::window_event& evt) {

  if (is_input_event(evt)) {

    _ui->input_event(evt);
    if (_ui->wants_input()) {
      return;
    }

    if (evt.type == event_type::key) {
      if (evt.event.key.keycode == key_sym::e::backspace) {
        _demo_opts = decltype(_demo_opts){};
        return;
      }

      if (evt.event.key.keycode == key_sym::e::escape) {
        _quit_receiver();
        return;
      }
    }

    _scene.cam_control.input_event(evt);
  }

  if (evt.type == event_type::configure) {
    _scene.cam.set_projection(projections_rh::perspective_symmetric(
      static_cast<float>(evt.event.configure.width) /
        static_cast<float>(evt.event.configure.height),
      radians(70.0f),
      0.1f,
      100.0f));
  }
}

void app::directional_light_demo::draw_ui(const int32_t surface_w,
                                          const int32_t surface_h) {
  _ui->new_frame(surface_w, surface_h);

  ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Appearing);

  if (ImGui::Begin("Options",
                   nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoMove)) {

    if (ImGui::CollapsingHeader("Model",
                                ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_Framed)) {
      if (ImGui::Combo(
            "Select a model",
            &_scene.current_mesh_idx,
            [](void* p, int32_t idx, const char** out) {
              auto obj = static_cast<directional_light_demo*>(p);
              *out = obj->_scene.available_meshes.mesh_list()[idx].name.c_str();
              return true;
            },
            this,
            static_cast<int32_t>(_scene.available_meshes.mesh_list().size()),
            5)) {
        switch_mesh(_scene.available_meshes.mesh_list()[_scene.current_mesh_idx]
                      .path.c_str());
      }

      ImGui::Separator();
      ImGui::SliderFloat(
        "Scale", &_objects[obj_type::teapot].scale, 0.1f, 10.0f);

      ImGui::Separator();
      if (ImGui::ColorPicker3("Diffuse",
                              _demo_opts.kd_main.components,
                              ImGuiColorEditFlags_NoAlpha)) {
        gl::ClearTexImage(raw_handle(_objtex[obj_type::teapot]),
                          0,
                          gl::RGBA,
                          gl::FLOAT,
                          _demo_opts.kd_main.components);
      }
    }

    if (ImGui::CollapsingHeader("Rotation",
                                ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_Framed)) {
      ImGui::Checkbox("X", &_demo_opts.rotate_x);
      ImGui::Checkbox("Y", &_demo_opts.rotate_y);
      ImGui::Checkbox("Z", &_demo_opts.rotate_z);
      ImGui::SliderFloat("Speed", &_demo_opts.rotate_speed, 0.001f, 0.5f);
    }

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_Framed)) {

      ImGui::SliderFloat3(
        "Direction", _demo_opts.lightdir.components, -1.0f, +1.0f);
      ImGui::Separator();

      ImGui::ColorPicker3(
        "Diffuse", _lights[0].kd.components, ImGuiColorEditFlags_NoAlpha);
      ImGui::Separator();

      ImGui::ColorPicker3(
        "Specular", _lights[0].ks.components, ImGuiColorEditFlags_NoAlpha);
      ImGui::Separator();

      ImGui::SliderFloat(
        "Specular intensity", &_demo_opts.specular_intensity, 1.0f, 256.0f);
    }

    if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_Framed)) {
      ImGui::Checkbox("Draw surface normals", &_demo_opts.drawnormals);
      ImGui::Separator();
      ImGui::SliderFloat("Normal length", &_demo_opts.normal_len, 0.1f, 5.0f);
    }
  }

  ImGui::End();

  _ui->draw();
}

void app::directional_light_demo::poll_start(
  const xray::ui::poll_start_event&) {}

void app::directional_light_demo::poll_end(const xray::ui::poll_end_event&) {}

void app::directional_light_demo::switch_mesh(const char* mesh_path) {
  auto loaded_mesh = basic_mesh{mesh_path};
  if (!loaded_mesh) {
    XR_DBG_MSG("Failed to create mesh!");
    return;
  }

  _meshes[obj_type::teapot] = std::move(loaded_mesh);
  _objects[obj_type::teapot].pos.y =
    _objects[obj_type::ripple].mesh->aabb().height() /*+
    _objects[obj_type::teapot].mesh->aabb().max_dimension() * 0.5f*/;
}
