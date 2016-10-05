#include "cap5/render_texture/render_texture_demo.hpp"
#include "helpers.hpp"
#include "std_assets.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
#include "xray/math/constants.hpp"
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
#include <algorithm>
#include <imgui/imgui.h>
#include <span.h>
#include <string>
#include <vector>

struct torus_config {
  float    outer_radius{0.7f};
  float    inner_radius{0.3f};
  uint32_t sides{16};
  uint32_t rings{16};
} torus;

struct box_config {
  float width{1.0f};
  float height{1.0f};
  float depth{1.0f};
  bool  invert_uvs{false};
  bool  wind_cw{false};
} box;

struct sphere_config {
  float    radius{3.0f};
  uint32_t max_subdivisions{3};
} sph;

bool import_light(xray::base::config_file* cfile, const char* cfg_path,
                  app::light_source4* ls) {
  char tmp_name[512];
  snprintf(tmp_name, XR_U32_COUNTOF__(tmp_name), "%s.pos", cfg_path);
  if (!cfile->lookup_vector_or_matrix(tmp_name, ls->position))
    return false;

  snprintf(tmp_name, XR_U32_COUNTOF__(tmp_name), "%s.color", cfg_path);
  return cfile->lookup_vector_or_matrix(tmp_name, ls->color);
}

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

app::render_texture_demo::render_texture_demo() { init(); }

app::render_texture_demo::~render_texture_demo() {}

void app::render_texture_demo::compose_ui() {
  {
    ImGui::Begin("Debug options", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Checkbox("Rotate mesh", &_dbg_opts.object_rotates);
    ImGui::Checkbox("Draw object", &_dbg_opts.draw_main_obj);
    ImGui::Checkbox("Draw wireframe", &_dbg_opts.draw_wireframe);
    ImGui::Checkbox("Draw normals to surface", &_dbg_opts.debug_normals);
    ImGui::ColorEdit3("Normal start color",
                      _dbg_opts.normal_color_beg.components);
    ImGui::ColorEdit3("Normal end color",
                      _dbg_opts.normal_color_end.components);
    ImGui::SliderFloat("Normal line length", &_dbg_opts.normal_line_len, 0.1f,
                       5.0f, "%3.3f");
    ImGui::SliderFloat3("Light position", _dbg_opts.light_pos.components,
                        -100.0f, +100.0f, "%3.3f");
    ImGui::End();
  }
}

void app::render_texture_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  gl::BindSampler(0, raw_handle(_fbo_texture_sampler));

  struct matrix_transform_pack {
    float4x4 model_view;
    float4x4 normal_view;
    float4x4 model_view_proj;
  };

  {
    const auto view_mtx = view_frame::look_at(_rto.cam_pos, float3::stdc::zero,
                                              float3::stdc::unit_y);
    const auto proj_mtx = projection::perspective_symmetric(
        static_cast<float>(_rto.rendertarget_size.x),
        static_cast<float>(_rto.rendertarget_size.y), radians(_rto.fov), 0.5f,
        100.0f);

    const auto proj_view_mtx = proj_mtx * view_mtx;

    const matrix_transform_pack tf_pack_spacecraft{view_mtx, view_mtx,
                                                   proj_view_mtx};

    const light_source4 scene_lights[] = {
        {_rto.lights[0].color, mul_point(view_mtx, _rto.lights[0].position)},
        {_rto.lights[1].color, mul_point(view_mtx, _rto.lights[1].position)},
    };

    gl::BindFramebuffer(gl::FRAMEBUFFER, raw_handle(_fbo));
    gl::BindTextureUnit(0, raw_handle(_spacecraft_material));

    gl::Viewport(0, 0, static_cast<GLsizei>(_rto.rendertarget_size.x),
                 static_cast<GLsizei>(_rto.rendertarget_size.y));

    gl::ClearColor(_rto.tex_clear_color.r, _rto.tex_clear_color.g,
                   _rto.tex_clear_color.b, _rto.tex_clear_color.a);

    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

    _drawprogram.set_uniform_block("transform_pack", tf_pack_spacecraft);
    _drawprogram.set_uniform_block("scene_light_sources", scene_lights);
    _drawprogram.set_uniform("surface_mtl", 0);
    _drawprogram.bind_to_pipeline();
    _spacecraft_mesh.draw();

    gl::BindFramebuffer(gl::FRAMEBUFFER, 0);
  }

  {
    const auto cube_world = _dbg_opts.object_rotates
                                ? float4x4{R3::rotate_y(_cube_r_angle)}
                                : float4x4::stdc::identity;
    const auto model_view      = dc.view_matrix * cube_world;
    const auto model_view_proj = dc.proj_view_matrix * cube_world;
    const matrix_transform_pack tf_pack_cube{model_view, model_view,
                                             model_view_proj};

    const light_source4 scene_lights[] = {
        {_rto.lights[0].color,
         mul_point(dc.view_matrix, _rto.lights[0].position)},
        {_rto.lights[1].color,
         mul_point(dc.view_matrix, _rto.lights[1].position)},
    };

    gl::BindTextureUnit(0, raw_handle(_fbo_texture));
    gl::Viewport(0, 0, static_cast<GLsizei>(dc.window_width),
                 static_cast<GLsizei>(dc.window_height));
    gl::ClearColor(_rto.scene_clear_color.r, _rto.scene_clear_color.g,
                   _rto.scene_clear_color.b, _rto.scene_clear_color.a);
    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

    _drawprogram.set_uniform_block("transform_pack", tf_pack_cube);
    _drawprogram.set_uniform_block("scene_light_sources", scene_lights);
    _drawprogram.set_uniform("surface_mtl", 0);
    _drawprogram.bind_to_pipeline();

    if (_dbg_opts.draw_main_obj) {

      if (_dbg_opts.draw_wireframe) {
        gl::Disable(gl::CULL_FACE);
        gl::PolygonMode(gl::FRONT_AND_BACK, gl::LINE);
      }

      _cube_mesh.draw();

      if (_dbg_opts.draw_wireframe) {
        gl::Enable(gl::CULL_FACE);
        gl::PolygonMode(gl::FRONT_AND_BACK, gl::FILL);
      }
    }

    if (_dbg_opts.debug_normals) {
      _drawprogram_debug.set_uniform_block("transform_pack", model_view_proj);
      _drawprogram_debug.set_uniform("normal_length",
                                     _dbg_opts.normal_line_len);
      _drawprogram_debug.set_uniform("start_point_color",
                                     _dbg_opts.normal_color_beg);
      _drawprogram_debug.set_uniform("end_point_color",
                                     _dbg_opts.normal_color_end);
      _drawprogram_debug.bind_to_pipeline();
      _cube_mesh.draw();
    }
  }
}

void app::render_texture_demo::update(const float /*delta_ms*/) {
  if (_dbg_opts.object_rotates) {
    _cube_r_angle += 0.01f;
    if (_cube_r_angle > two_pi<float>)
      _cube_r_angle -= two_pi<float>;
  }
}

void app::render_texture_demo::key_event(const int32_t /*key_code*/,
                                         const int32_t /*action*/,
                                         const int32_t /*mods*/) {}

void app::render_texture_demo::init() {

  _drawprogram = []() {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER,
                    "shaders/cap5/render_texture/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER,
                    "shaders/cap5/render_texture/shader.frag")};

    return gpu_program{compiled_shaders};
  }();

  if (!_drawprogram) {
    XR_LOG_ERR("Failed to compile/link drawing program!");
    return;
  }

  _drawprogram_debug = []() {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, "shaders/dbg/debug.vs"),
        make_shader(gl::GEOMETRY_SHADER, "shaders/dbg/debug.gs"),
        make_shader(gl::FRAGMENT_SHADER, "shaders/dbg/debug.fs")};

    return gpu_program{compiled_shaders};
  }();

  if (!_drawprogram_debug) {
    XR_LOG_ERR("Failed to compile/link debug drawing program!");
    return;
  }

  _fbo_texture = []() {
    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGBA8, 512, 512);

    return texh;
  }();

  _fbo_texture_sampler = []() {
    GLuint smph{};
    gl::CreateSamplers(1, &smph);
    gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_R, gl::REPEAT);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_S, gl::REPEAT);
    gl::SamplerParameteri(smph, gl::TEXTURE_WRAP_T, gl::REPEAT);

    return smph;
  }();

  _fbo_depthbuffer = []() {
    GLuint rbo{};
    gl::CreateRenderbuffers(1, &rbo);
    gl::NamedRenderbufferStorage(rbo, gl::DEPTH_COMPONENT, 512, 512);

    return rbo;
  }();

  _fbo = [
    tex = raw_handle(_fbo_texture), depth_buffer = raw_handle(_fbo_depthbuffer)
  ]() {
    GLuint fbo{};
    gl::CreateFramebuffers(1, &fbo);
    gl::NamedFramebufferTexture(fbo, gl::COLOR_ATTACHMENT0, tex, 0);
    gl::NamedFramebufferRenderbuffer(fbo, gl::DEPTH_ATTACHMENT,
                                     gl::RENDERBUFFER, depth_buffer);
    gl::NamedFramebufferDrawBuffer(fbo, gl::COLOR_ATTACHMENT0);

    const auto status = gl::CheckNamedFramebufferStatus(fbo, gl::FRAMEBUFFER);
    if (status != gl::FRAMEBUFFER_COMPLETE) {
      XR_LOG_ERR("Failed to create framebuffer!");
      return GLuint{};
    }

    return fbo;
  }
  ();

  config_file demo_cfg{"config/cap5/render_texture/demo.conf"};
  if (!demo_cfg) {
    XR_LOG_ERR("Failed to read configuration file !");
    return;
  }

  {
    demo_cfg.lookup_vector_or_matrix("app.params.cam_pos", _rto.cam_pos);
    demo_cfg.lookup_value("app.params.render_tex_size.x",
                          _rto.rendertarget_size.x);
    demo_cfg.lookup_value("app.params.render_tex_size.y",
                          _rto.rendertarget_size.y);
    demo_cfg.lookup_value("app.params.fov", _rto.fov);
    import_light(&demo_cfg, "app.params.light", &_rto.lights[0]);
    import_light(&demo_cfg, "app.params.light1", &_rto.lights[1]);
    demo_cfg.lookup_vector_or_matrix("app.params.clear_color.texture",
                                     _rto.tex_clear_color);
    demo_cfg.lookup_vector_or_matrix("app.params.clear_color.scene",
                                     _rto.scene_clear_color);
  }

  {
    const char* mdl_file{nullptr};
    demo_cfg.lookup_value("app.params.model", mdl_file);
    _spacecraft_mesh = simple_mesh{
        vertex_format::pnt, c_str_ptr(xr_app_config->model_path(mdl_file))};

    if (!_spacecraft_mesh) {
      XR_LOG_ERR("Failed to import model {}", mdl_file);
      return;
    }
  }

  _cube_mesh = [&demo_cfg, this]() {

    const char* shape_type{nullptr};
    demo_cfg.lookup_value("app.params.active_shape", shape_type);

    geometry_data_t obj_mesh;

    if (!strcmp("torus", shape_type)) {
      demo_cfg.lookup_value("app.shape_config.torus.outer_radius",
                            torus.outer_radius);
      demo_cfg.lookup_value("app.shape_config.torus.inner_radius",
                            torus.inner_radius);
      demo_cfg.lookup_value("app.shape_config.torus.sides", torus.sides);
      demo_cfg.lookup_value("app.shape_config.torus.rings", torus.rings);

      geometry_factory::torus(torus.outer_radius, torus.inner_radius,
                              torus.sides, torus.rings, &obj_mesh);

    } else if (!strcmp("box", shape_type)) {
      demo_cfg.lookup_value("app.shape_config.box.width", box.width);
      demo_cfg.lookup_value("app.shape_config.box.height", box.height);
      demo_cfg.lookup_value("app.shape_config.box.depth", box.depth);
      demo_cfg.lookup_value("app.shape_config.box.invert_uvs", box.invert_uvs);
      demo_cfg.lookup_value("app.shape_config.box.wind_cw", box.wind_cw);

      geometry_factory::box(box.width, box.height, box.depth, &obj_mesh);
      _dbg_opts.frontface_cw = box.wind_cw;

    } else if (!strcmp("sphere", shape_type)) {
      demo_cfg.lookup_value("app.shape_config.sphere.radius", sph.radius);
      demo_cfg.lookup_value("app.shape_config.sphere.max_subdiv",
                            sph.max_subdivisions);

      geometry_factory::geosphere(sph.radius, sph.max_subdivisions, &obj_mesh);
    } else {
      XR_LOG_ERR("Unsupported shape!");
      return simple_mesh{};
    }

    return simple_mesh{vertex_format::pnt, obj_mesh};
  }();

  if (!_cube_mesh) {
    XR_LOG_ERR("Failed to create object mesh!");
    return;
  }

  _spacecraft_material = [&demo_cfg]() {

    const char* material{nullptr};
    demo_cfg.lookup_value("app.params.texture", material);
    if (!material) {
      XR_LOG_ERR("Failed to load material !");
      return GLuint{};
    }

    texture_loader tex_ldr{c_str_ptr(xr_app_config->texture_path(material)),
                           texture_load_options::flip_y};

    if (!tex_ldr) {
      XR_LOG_ERR("Failed to load material !");
      return GLuint{};
    }

    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, tex_ldr.depth() == 4 ? gl::RGBA8 : gl::RGB8,
                         tex_ldr.width(), tex_ldr.height());
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          tex_ldr.depth() == 4 ? gl::RGBA : gl::RGB,
                          gl::UNSIGNED_BYTE, tex_ldr.data());
    return texh;
  }();

  if (!_spacecraft_material) {
    XR_LOG_ERR("Failed to load material !!");
    return;
  }

  _null_texture = []() {
    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGB8, 1, 1);
    const uint8_t white_pixel[] = {255, 255, 255};
    gl::TextureSubImage2D(texh, 0, 0, 0, 1, 1, gl::RGB, gl::UNSIGNED_BYTE,
                          white_pixel);
    return texh;
  }();

  _valid = true;
}
