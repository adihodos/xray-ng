#include "cap6/edge_detect/edge_detect_demo.hpp"
#include "helpers.hpp"
#include "std_assets.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/math/projection.hpp"
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
#include "xray/rendering/vertex_format/vertex_pc.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/config_reader_scene.hpp"
#include "xray/scene/point_light.hpp"
#include <algorithm>
#include <imgui/imgui.h>
#include <span.h>
#include <string>
#include <vector>

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace xray::scene;
using namespace std;

app::edge_detect_demo::edge_detect_demo() { init(); }

app::edge_detect_demo::~edge_detect_demo() {}

void app::edge_detect_demo::compose_ui() {}

void app::edge_detect_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  {
    //
    // first pass - normal phong lighting
    const auto obj_to_world = float4x4::stdc::identity;
    const auto obj_to_view  = dc.view_matrix * obj_to_world;

    struct matrix_pack_t {
      float4x4 world_view;
      float4x4 normal_view;
      float4x4 world_view_proj;
    } const obj_transforms{obj_to_view, obj_to_view,
                           dc.projection_matrix * obj_to_view};

    _drawprog_first_pass.set_uniform_block("transform_pack", obj_transforms);

    point_light lights[edge_detect_demo::max_lights];
    transform(begin(_lights), end(_lights), begin(lights),
              [&dc](const auto& in_light) -> point_light {
                return {in_light.ka, in_light.kd, in_light.ks,
                        mul_point(dc.view_matrix, in_light.position)};
              });

    _drawprog_first_pass.set_uniform_block("scene_lighting", lights);
    _drawprog_first_pass.set_uniform("light_count", _lightcount);
    _drawprog_first_pass.set_uniform("mat_diffuse", 0);
    _drawprog_first_pass.set_uniform("mat_specular", 1);
    _drawprog_first_pass.set_uniform("mat_shininess", _mat_spec_pwr);
    _drawprog_first_pass.bind_to_pipeline();

    {
      const GLuint samplers[] = {raw_handle(_fbo.fbo_sampler),
                                 raw_handle(_fbo.fbo_sampler)};

      gl::BindSamplers(0, XR_U32_COUNTOF__(samplers), samplers);
    }

    {
      const GLuint materials[] = {raw_handle(_obj_material),
                                  raw_handle(_obj_material)};
      gl::BindTextures(0, XR_I32_COUNTOF__(materials), materials);
    }

    _object.draw();
  }
}

void app::edge_detect_demo::update(const float /*delta_ms*/) {}

void app::edge_detect_demo::key_event(const int32_t /*key_code*/,
                                      const int32_t /*action*/,
                                      const int32_t /*mods*/) {}

void app::edge_detect_demo::init() {
  uint32_t render_wnd_width{1024};
  uint32_t render_wnd_height{1024};

  _fbo.fbo_texture = [ w = render_wnd_width, h = render_wnd_height ]() {
    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGBA8, w, h);

    return texh;
  }
  ();

  _fbo.fbo_sampler = []() {
    GLuint smpl{};
    gl::CreateSamplers(1, &smpl);
    gl::SamplerParameteri(smpl, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smpl, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return smpl;
  }();

  _fbo.fbo_depth = [ w = render_wnd_width, h = render_wnd_height ]() {
    GLuint depth_rbuff{};
    gl::CreateRenderbuffers(1, &depth_rbuff);
    gl::NamedRenderbufferStorage(depth_rbuff, gl::DEPTH_COMPONENT, w, h);

    return depth_rbuff;
  }
  ();

  _fbo.fbo_object = [
    tex = raw_handle(_fbo.fbo_texture), depth = raw_handle(_fbo.fbo_depth)
  ]() {
    GLuint fbo{};
    gl::CreateFramebuffers(1, &fbo);
    gl::NamedFramebufferTexture(fbo, gl::COLOR_ATTACHMENT0, tex, 0);
    gl::NamedFramebufferRenderbuffer(fbo, gl::DEPTH_ATTACHMENT,
                                     gl::RENDERBUFFER, depth);
    gl::NamedFramebufferDrawBuffer(fbo, gl::COLOR_ATTACHMENT0);

    const auto fbo_status =
        gl::CheckNamedFramebufferStatus(fbo, gl::DRAW_FRAMEBUFFER);
    if (fbo_status != gl::FRAMEBUFFER_COMPLETE) {
      XR_LOG_ERR("Framebuffer is not complete!");
      return GLuint{};
    }

    return fbo;
  }
  ();

  if (!_fbo.fbo_object) {
    XR_LOG_ERR("Failed to cretae/init framebuffer!!");
    return;
  }

  _drawprog_first_pass = []() {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, "shaders/cap6/edge_detect/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap6/edge_detect/shader.frag")};

    return gpu_program{compiled_shaders};
  }();

  config_file app_cfg{"config/cap6/edge_detect/app.conf"};
  if (!app_cfg) {
    XR_LOG_ERR("Fatal error : config file not found !");
    return;
  }

  {
    const char* model_file{nullptr};
    if (!app_cfg.lookup_value("app.scene.model", model_file)) {
      XR_LOG_ERR("Failed to read model file name from config!");
      return;
    }

    _object =
        simple_mesh{vertex_format::pnt, xr_app_config->model_path(model_file)};
    if (!_object) {
      XR_LOG_ERR("Failed to load model from file!");
      return;
    }
  }

  {
    auto lights_entry = app_cfg.lookup_entry("app.scene.lights");
    if (!lights_entry) {
      XR_LOG_ERR("Lights section not found in config file!");
      return;
    }

    _lightcount = xray::math::min<uint32_t>(lights_entry.length(),
                                            edge_detect_demo::max_lights);

    for (uint32_t idx = 0; idx < _lightcount; ++idx) {
      const auto le = lights_entry[idx];
      config_scene_reader::read_point_light(le, &_lights[idx]);
    }
  }

  _obj_material = [&app_cfg]() {
    const char* material_file{};
    app_cfg.lookup_value("app.scene.material.file", material_file);
    if (!material_file)
      return GLuint{};

    bool flip_yaxis{false};
    app_cfg.lookup_value("app.scene.material.flip_y", flip_yaxis);

    texture_loader tex_ldr{xr_app_config->texture_path(material_file),
                           flip_yaxis ? texture_load_options::flip_y
                                      : texture_load_options::none};
    if (!tex_ldr)
      return GLuint{};

    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    constexpr uint32_t internal_format[] = {gl::RGB8, gl::RGBA8};
    gl::TextureStorage2D(texh, 1, internal_format[tex_ldr.depth() == 4],
                         tex_ldr.width(), tex_ldr.height());
    constexpr uint32_t img_format[] = {gl::RGB, gl::RGBA};
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          img_format[tex_ldr.depth() == 4], gl::UNSIGNED_BYTE,
                          tex_ldr.data());

    return texh;
  }();

  app_cfg.lookup_value("app.scene.shininess", _mat_spec_pwr);
  _valid = true;
}
