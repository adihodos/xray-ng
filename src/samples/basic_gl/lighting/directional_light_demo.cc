#include "lighting/directional_light_demo.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar3_string_cast.hpp"
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
#include <opengl/opengl.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <pipes/pipes.hpp>

#include <filesystem>
#include <system_error>

#include <algorithm>
#include <cassert>
#include <numeric>
#include <queue>
#include <span>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::directional_light_demo::directional_light_demo(const init_context_t& ctx,
                                                    RenderState&&         rs,
                                                    const DemoOptions&    ds,
                                                    Scene&&               s)
  : demo_base{ctx}
  , mRenderState{std::move(rs)}
  , mDemoOptions{ds}
  , mScene{std::move(s)} {

  mRenderState.geometryObjects >>= pipes::transform([](basic_mesh& bm) {
    return graphics_object{
      &bm, xray::math::vec3f::stdc::zero, xray::math::vec3f::stdc::zero, 1.0f};
  }) >>= pipes::push_back(mRenderState.graphicsObjects);

  mScene.cam.set_projection(projections_rh::perspective_symmetric(
    static_cast<float>(ctx.surface_width) /
      static_cast<float>(ctx.surface_height),
    radians(70.0f),
    0.1f,
    100.0f));

  mScene.cam_control.set_camera(&mScene.cam);
  switch_mesh(SwitchMeshOpt::UseExisting);
  mScene.debugOutput.reserve(1024);

  _ui->set_global_font("Roboto-Regular");
  mScene.timer.start();
}

app::directional_light_demo::~directional_light_demo() {}

tl::optional<app::demo_bundle_t>
app::directional_light_demo::create(const app::init_context_t& initContext) {

  namespace fs = std::filesystem;
  queue<fs::path> pathQueue;
  pathQueue.push(xr_app_config->model_root().c_str());

  error_code errCode{};
  assert(fs::is_directory(pathQueue.front(), errCode));

  directional_light_demo::Scene s{};

  while (!pathQueue.empty()) {
    fs::path currentPath{pathQueue.front()};
    pathQueue.pop();

    auto meshFileFilterFn = [](const fs::path& path) {
      return path.extension().c_str() == std::string_view{".bin"};
    };

    if (fs::is_directory(currentPath, errCode)) {
      for (const fs::directory_entry& dirEntry :
           fs::recursive_directory_iterator{currentPath}) {
        if (dirEntry.is_directory()) {
          pathQueue.push(dirEntry.path());
        } else if (dirEntry.is_regular_file() && meshFileFilterFn(dirEntry)) {
          s.modelFiles.emplace_back(dirEntry.path(),
                                    dirEntry.path().stem().string());
        }
      }

      continue;
    }

    if (fs::is_regular_file(currentPath, errCode) &&
        meshFileFilterFn(currentPath)) {
      s.modelFiles.emplace_back(currentPath, currentPath.stem().string());
    }
  }

  s.modelFiles >>= pipes::for_each(
    [](const ModelInfo& mi) { XR_LOG_DEBUG("::: {} :::", mi.path.c_str()); });

  const DemoOptions demoOptions{};
  s.lights.emplace_back(rgb_color{demoOptions.ka},
                        rgb_color{demoOptions.kd},
                        rgb_color{demoOptions.ks},
                        normalize(demoOptions.lightdir),
                        0.0f);

  //
  // geometry
  geometry_data_t blob;
  geometry_factory::grid(16.0f, 16.0f, 128, 128, &blob);

  vector<vertex_pnt> verts;
  blob.vertex_span() >>= pipes::transform([](const vertex_pntt vsin) {
    return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
  }) >>= pipes::push_back(verts);

  const vertex_ripple_parameters rparams{
    1.0f, 3.0f * two_pi<float>, 16.0f, 16.0f};

  vertex_effect::ripple(
    std::span<vertex_pnt>{verts}, blob.index_span(), rparams);

  itlib::small_vector<basic_mesh, 4> geometryObjects{};

  geometryObjects.emplace_back(std::span{verts}, blob.index_span());

  auto viperMdlItr =
    find_if(cbegin(s.modelFiles), cend(s.modelFiles), [](const ModelInfo& mdl) {
      return string_view{mdl.path.c_str()}.find("viper") != string_view::npos;
    });

  mesh_loader::load(viperMdlItr != cend(s.modelFiles)
                      ? viperMdlItr->path.c_str()
                      : s.modelFiles.front().path.c_str())
    .map([&geometryObjects](mesh_loader loadedModel) {
      geometryObjects.emplace_back(loadedModel);
    });

  vertex_program vertShader =
    gpu_program_builder{}
      .add_file("shaders/lighting/vs.directional.glsl")
      .build<render_stage::e::vertex>();

  if (!vertShader)
    return tl::nullopt;

  fragment_program fragShader =
    gpu_program_builder{}
      .add_file("shaders/lighting/fs.directional.glsl")
      .build<render_stage::e::fragment>();

  if (!fragShader)
    return tl::nullopt;

  program_pipeline pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  pipeline.use_vertex_program(vertShader).use_fragment_program(fragShader);

  const tuple<uint32_t, rgb_color> texColorsData[] = {
    {256, color_palette::web::dark_orange}, {256, color_palette::web::navy}};

  itlib::small_vector<scoped_texture> colorTextures;
  texColorsData >>=
    pipes::transform([](const tuple<uint32_t, rgb_color>& texData) {
      GLuint newTexture{};

      gl::CreateTextures(gl::TEXTURE_2D, 1, &newTexture);

      const auto [texDimensions, texColor] = texData;

      gl::TextureStorage2D(
        newTexture, 1, gl::RGBA8, texDimensions, texDimensions);
      gl::ClearTexImage(
        newTexture, 0, gl::RGBA, gl::FLOAT, texColor.components);

      return scoped_texture{newTexture};
    }) >>= pipes::push_back(colorTextures);

  scoped_sampler sampler{[]() {
    GLuint samplerId{};
    gl::CreateSamplers(1, &samplerId);

    gl::SamplerParameteri(samplerId, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(samplerId, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return scoped_sampler{samplerId};
  }()};

  unique_pointer<directional_light_demo> object{
    new directional_light_demo{// base
                               initContext,
                               // renderstate
                               {surface_normal_visualizer{},
                                move(geometryObjects),
                                {},
                                move(vertShader),
                                move(fragShader),
                                move(pipeline),
                                move(colorTextures),
                                move(sampler)},
                               // demo options
                               demoOptions,
                               // scene
                               move(s)}};

  gl::Enable(gl::DEPTH_TEST);
  gl::Enable(gl::CULL_FACE);
  gl::CullFace(gl::BACK);

  auto winEventHandler =
    make_delegate(*object, &directional_light_demo::event_handler);
  auto loopEventHandler =
    make_delegate(*object, &directional_light_demo::loop_event);

  return tl::make_optional<demo_bundle_t>(
    std::move(object), winEventHandler, loopEventHandler);
}

void app::directional_light_demo::loop_event(
  const xray::ui::window_loop_event& wle) {
  mScene.timer.update_and_reset();
  update(mScene.timer.elapsed_millis());
  draw();
  draw_ui(wle.wnd_width, wle.wnd_height);
}

void app::directional_light_demo::draw() {
  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  gl::BindTextureUnit(
    0, raw_handle(mRenderState.objectTextures[obj_type::ripple]));
  gl::BindSampler(0, raw_handle(mRenderState.sampler));

  mRenderState.pipeline.use(false);

  auto ripple = &mRenderState.graphicsObjects[obj_type::ripple];

  assert(ripple->mesh != nullptr);
  gl::BindVertexArray(ripple->mesh->vertex_array());

  const mat4f rippleToWorld{R4::translate(ripple->pos)};
  struct VsUniformBuffer {
    mat4f world_view_proj;
    mat4f view;
    mat4f normals;
  } const rippleVertShaderTransforms = {mScene.cam.projection_view() *
                                          rippleToWorld,
                                        mScene.cam.view() * rippleToWorld,
                                        rippleToWorld};

  mRenderState.vs.set_uniform_block("TransformMatrices",
                                    rippleVertShaderTransforms);
  mRenderState.fs.set_uniform("DIFFUSE_MAP", 0);

  struct {
    directional_light lights[8];
    uint32_t          lightscount;
  } scene_lights;

  mScene.lights >>=
    pipes::transform([this](const directional_light& inputLight) {
      directional_light dl{inputLight};
      dl.direction =
        normalize(mul_vec(mScene.cam.view(), -inputLight.direction));
      dl.half_vector = normalize(dl.direction + mScene.cam.direction());

      return dl;
    }) >>= pipes::override(scene_lights.lights);

  scene_lights.lightscount = static_cast<uint32_t>(mScene.lights.size());

  mRenderState.fs.set_uniform_block("LightData", scene_lights);
  mRenderState.vs.flush_uniforms();
  mRenderState.fs.flush_uniforms();

  gl::Disable(gl::CULL_FACE);
  gl::DrawElements(
    gl::TRIANGLES, ripple->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

  auto teapot = &mRenderState.graphicsObjects[obj_type::teapot];

  if (teapot->mesh) {
    gl::BindVertexArray(teapot->mesh->vertex_array());
    gl::BindTextureUnit(
      0, raw_handle(mRenderState.objectTextures[obj_type::teapot]));

    const auto pitch = quaternionf{teapot->rotation.x, vec3f::stdc::unit_x};
    const auto roll  = quaternionf{teapot->rotation.z, vec3f::stdc::unit_z};
    const auto yaw   = quaternionf{teapot->rotation.y, vec3f::stdc::unit_y};
    const auto qrot  = yaw * pitch * roll;
    const auto teapot_rot = rotation_matrix(qrot);

    const auto teapot_world =
      R4::translate(teapot->pos) * teapot_rot * mat4f{R3::scale(teapot->scale)};

    const VsUniformBuffer teapotVertShaderTransforms = {
      .world_view_proj = mScene.cam.projection_view() * teapot_world,
      .view            = mScene.cam.view() * teapot_world,
      .normals         = teapot_rot};

    mRenderState.vs.set_uniform_block("TransformMatrices",
                                      teapotVertShaderTransforms);
    mRenderState.vs.flush_uniforms();

    gl::Enable(gl::CULL_FACE);
    gl::DrawElements(
      gl::TRIANGLES, teapot->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

    if (mDemoOptions.drawnormals) {
      const draw_context_t dc{0u,
                              0u,
                              mScene.cam.view(),
                              mScene.cam.projection(),
                              mScene.cam.projection_view(),
                              &mScene.cam,
                              nullptr};

      mRenderState.drawNormals.draw(dc,
                                    *teapot->mesh,
                                    R4::translate(teapot->pos) * teapot_rot,
                                    color_palette::web::green,
                                    color_palette::web::green,
                                    mDemoOptions.normal_len);
    }
  }

  if (mDemoOptions.drawnormals) {
    const draw_context_t dc{0u,
                            0u,
                            mScene.cam.view(),
                            mScene.cam.projection(),
                            mScene.cam.projection_view(),
                            &mScene.cam,
                            nullptr};

    mRenderState.drawNormals.draw(dc,
                                  *ripple->mesh,
                                  R4::translate(ripple->pos),
                                  color_palette::web::cadet_blue,
                                  color_palette::web::cadet_blue,
                                  mDemoOptions.normal_len);
  }
}

void app::directional_light_demo::update(const float delta_ms) {
  auto teapot = &mRenderState.graphicsObjects[obj_type::teapot];

  if (mDemoOptions.rotate_x) {
    teapot->rotation.x += mDemoOptions.rotate_speed;
    if (teapot->rotation.x > two_pi<float>) {
      teapot->rotation.x -= two_pi<float>;
    }
  }

  if (mDemoOptions.rotate_y) {
    teapot->rotation.y += mDemoOptions.rotate_speed;

    if (teapot->rotation.y > two_pi<float>) {
      teapot->rotation.y -= two_pi<float>;
    }
  }

  if (mDemoOptions.rotate_z) {
    teapot->rotation.z += mDemoOptions.rotate_speed;

    if (teapot->rotation.z > two_pi<float>) {
      teapot->rotation.z -= two_pi<float>;
    }
  }

  mScene.cam_control.update();
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
        mDemoOptions = {};
        return;
      }

      if (evt.event.key.keycode == key_sym::e::insert) {
        // reset mesh scale, pos, rot, etc
        adjust_model_transforms();
      }

      if (evt.event.key.keycode == key_sym::e::escape) {
        _quit_receiver();
        return;
      }
    }

    mScene.cam_control.input_event(evt);
    return;
  }

  if (evt.type == event_type::configure) {
    mScene.cam.set_projection(projections_rh::perspective_symmetric(
      static_cast<float>(evt.event.configure.width) /
        static_cast<float>(evt.event.configure.height),
      radians(70.0f),
      0.1f,
      100.0f));

    return;
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
            &mScene.current_mesh_idx,
            [](void* p, int32_t idx, const char** out) {
              auto obj = static_cast<directional_light_demo*>(p);
              *out     = obj->mScene.modelFiles[idx].description.c_str();
              return true;
            },
            this,
            static_cast<int32_t>(mScene.modelFiles.size()),
            5)) {
        switch_mesh(SwitchMeshOpt::LoadFromFile);
      }

      ImGui::Separator();
      ImGui::SliderFloat("Scale",
                         &mRenderState.graphicsObjects[obj_type::teapot].scale,
                         0.1f,
                         10.0f);

      ImGui::Separator();
      if (ImGui::ColorPicker3("Object diffuse color",
                              mDemoOptions.kd_main.components,
                              ImGuiColorEditFlags_NoAlpha)) {
        gl::ClearTexImage(
          raw_handle(mRenderState.objectTextures[obj_type::teapot]),
          0,
          gl::RGBA,
          gl::FLOAT,
          mDemoOptions.kd_main.components);
      }
    }

    if (ImGui::CollapsingHeader("Rotation",
                                ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_Framed)) {
      ImGui::Checkbox("X", &mDemoOptions.rotate_x);
      ImGui::Checkbox("Y", &mDemoOptions.rotate_y);
      ImGui::Checkbox("Z", &mDemoOptions.rotate_z);
      ImGui::SliderFloat("Speed", &mDemoOptions.rotate_speed, 0.001f, 0.5f);
    }

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_Framed)) {
      directional_light* dl = &mScene.lights[0];

      ImGui::SliderFloat3("Direction", dl->direction.components, -1.0f, +1.0f);

      ImGui::Separator();

      ImGui::ColorPicker3(
        "Ambient (ka)", dl->ka.components, ImGuiColorEditFlags_NoAlpha);
      ImGui::Separator();

      ImGui::ColorPicker3(
        "Diffuse (kd)", dl->kd.components, ImGuiColorEditFlags_NoAlpha);
      ImGui::Separator();

      ImGui::ColorPicker3(
        "Specular (ks)", dl->ks.components, ImGuiColorEditFlags_NoAlpha);
      ImGui::Separator();

      ImGui::SliderFloat("Specular intensity", &dl->shininess, 1.0f, 256.0f);
      ImGui::Separator();

      ImGui::SliderFloat("Light intensity", &dl->strength, 1.0f, 256.0f);
    }

    if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_Framed)) {
      ImGui::Checkbox("Draw surface normals", &mDemoOptions.drawnormals);
      ImGui::Separator();
      ImGui::SliderFloat("Normal length", &mDemoOptions.normal_len, 0.1f, 5.0f);
      ImGui::Separator();

      mScene.debugOutput.clear();
      mRenderState.graphicsObjects >>=
        pipes::for_each([this](const graphics_object& graphicsObj) mutable {
          fmt::format_to(
            back_inserter(mScene.debugOutput),
            "position: {:3.3f}, rotation: {:3.3f}, scale: {:3.3f}\n",
            graphicsObj.pos,
            graphicsObj.rotation,
            graphicsObj.scale);
        });
      ImGui::TextUnformatted(mScene.debugOutput.c_str());
    }
  }

  ImGui::End();

  _ui->draw();
}

void app::directional_light_demo::poll_start(
  const xray::ui::poll_start_event&) {}

void app::directional_light_demo::poll_end(const xray::ui::poll_end_event&) {}

void app::directional_light_demo::switch_mesh(
  const SwitchMeshOpt switchMeshOpt) {
  if (switchMeshOpt == SwitchMeshOpt::LoadFromFile) {
    basic_mesh loaded_mesh{
      mScene.modelFiles[mScene.current_mesh_idx].path.c_str()};
    if (!loaded_mesh) {
      XR_DBG_MSG("Failed to create mesh!");
      return;
    }
    mRenderState.geometryObjects[obj_type::teapot] = std::move(loaded_mesh);
  }

  adjust_model_transforms();
}

void app::directional_light_demo::adjust_model_transforms() {
  graphics_object* teapotObj = &mRenderState.graphicsObjects[obj_type::teapot];
  const float      teapotBoundingSphRadius =
    teapotObj->mesh->bounding_sphere().radius;

  const graphics_object* rippleObj =
    &mRenderState.graphicsObjects[obj_type::ripple];
  const float rippleBoundingSphRadius =
    rippleObj->mesh->bounding_sphere().radius;

  const float k = rippleBoundingSphRadius / teapotBoundingSphRadius;

  teapotObj->scale = k;
  teapotObj->pos =
    vec3f{0.0f,
          rippleObj->mesh->aabb().height() * 0.5f + k * teapotBoundingSphRadius,
          0.0f};
  teapotObj->rotation = vec3f::stdc::zero;
}
