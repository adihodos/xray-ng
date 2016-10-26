#include "cap6/edge_detect/edge_detect_demo.hpp"
#include "debug_record.hpp"
#include "helpers.hpp"
#include "init_context.hpp"
#include "resize_context.hpp"
#include "scene_loader.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/texture_loader.hpp"
#include <algorithm>
#include <array>
#include <imgui/imgui.h>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <platformstl/filesystem/path.hpp>
#include <platformstl/filesystem/path_functions.hpp>
#include <stlsoft/memory/auto_buffer.hpp>
#include <tbb/tbb.h>
#include <unordered_map>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace xray::scene;
using namespace std;
using namespace app;

app::edge_detect_demo::edge_detect_demo(const init_context_t& ctx) {
  init(ctx);
}

app::edge_detect_demo::~edge_detect_demo() {}

void app::edge_detect_demo::compose_ui() {
  if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoMove)) {

    if (ImGui::RadioButton("No edge detection",
                           (_detect_method == edge_detect_none))) {
      _detect_method = edge_detect_none;
    }

    if (ImGui::RadioButton("Sobel edge detection",
                           (_detect_method == edge_detect_sobel))) {
      _detect_method = edge_detect_sobel;
    }

    if (ImGui::RadioButton("Frei-Chen edge detection",
                           (_detect_method == edge_detect_frei_chen))) {
      _detect_method = edge_detect_frei_chen;
    }

    if (_detect_method == edge_detect_sobel) {
      ImGui::Separator();
      ImGui::SliderFloat("Edge treshold", &_edge_treshold, 0.01f, 0.25f,
                         "%3.3f", 0.1f);
    }

    ImGui::Checkbox("Rotate model", &_rotate_object);

    ImGui::Separator();
    ImGui::BeginGroup();

    for (auto first = detail::DEBUGRecord_records_begin(),
              last  = detail::DEBUGRecord_records_end();
         first != last; ++first) {
      if (first->hit_count == 0)
        continue;

      ImGui::Text("BLOCK: %s %d (%3.3f)", first->func, first->line,
                  first->millis);
      ImGui::Separator();
    }
    ImGui::EndGroup();
  }
  ImGui::End();
}

void app::edge_detect_demo::draw(
    const xray::rendering::draw_context_t& draw_ctx) {
  assert(valid());

  {
    // SCOPED_NAMED_TIME_BLOCK("Phong lighting pass");

    //
    // first pass - normal phong lighting
    gl::BindFramebuffer(gl::FRAMEBUFFER, raw_handle(_fbo.fbo_object));

    {
      const float viewport_dim[] = {0.0f, 0.0f,
                                    static_cast<float>(draw_ctx.window_width),
                                    static_cast<float>(draw_ctx.window_height)};

      gl::ViewportIndexedfv(0, viewport_dim);
      gl::ClearNamedFramebufferfv(raw_handle(_fbo.fbo_object), gl::COLOR, 0,
                                  color_palette::web::black.components);
      const float def_clear_depthval{1.0f};
      gl::ClearNamedFramebufferfv(raw_handle(_fbo.fbo_object), gl::DEPTH, 0,
                                  &def_clear_depthval);
    }

    {
      const GLuint bound_textures[] = {raw_handle(_fbo.fbo_texture),
                                       raw_handle(_obj_material),
                                       raw_handle(_obj_material)};

      gl::BindTextures(0, XR_U32_COUNTOF__(bound_textures), bound_textures);
    }

    {
      const GLuint bound_samplers[] = {raw_handle(_fbo.fbo_sampler),
                                       raw_handle(_sampler_obj),
                                       raw_handle(_sampler_obj)};

      gl::BindSamplers(0, XR_U32_COUNTOF__(bound_samplers), bound_samplers);
    }

    const auto obj_to_world = mat4f{R3::rotate_xyz(_rx, _ry, _rz)};
    const auto obj_to_view  = draw_ctx.view_matrix * obj_to_world;

    struct matrix_pack_t {
      mat4f world_view;
      mat4f normal_view;
      mat4f world_view_proj;
    } const obj_transforms{obj_to_view, obj_to_view,
                           draw_ctx.projection_matrix * obj_to_view};

    _vertex_prg.set_uniform_block("transform_pack", obj_transforms);

    point_light lights[edge_detect_demo::max_lights];
    transform(begin(_lights), end(_lights), begin(lights),
              [&draw_ctx](const auto& in_light) -> point_light {
                return {in_light.ka, in_light.kd, in_light.ks,
                        mul_point(draw_ctx.view_matrix, in_light.position)};
              });

    _frag_prg.set_uniform_block("scene_lighting", lights)
        .set_uniform("light_count", _lightcount)
        .set_uniform("mat_diffuse", 1)
        .set_uniform("mat_specular", 2)
        .set_uniform("mat_shininess", _mat_spec_pwr);

    gpu_program_pipeline_setup_builder{raw_handle(_prog_pipeline)}
        .add_vertex_program(_vertex_prg)
        .add_fragment_program(_frag_prg)
        .install();

    _obj_graphics.draw();
  }

  //
  // second pass, edge detection
  {
    // SCOPED_NAMED_TIME_BLOCK("edge detection pass");
    //
    // set default framebuffer
    gl::BindFramebuffer(gl::FRAMEBUFFER, 0);

    gl::BindTextureUnit(0, raw_handle(_fbo.fbo_texture));
    gl::BindSampler(0, raw_handle(_sampler_obj));

    const auto surface_size = vec2f{static_cast<float>(draw_ctx.window_width),
                                    static_cast<float>(draw_ctx.window_height)};

    constexpr const char* const drawing_subroutines[] = {
        "color_default", "color_edges_sobel", "color_edges_frei_chen"};

    _fs_edge_detect.set_uniform("kSourceTexture", 0)
        .set_uniform("kEdgeTresholdSquared", _edge_treshold)
        .set_uniform("kSurfaceSize", surface_size)
        .set_subroutine_uniform("kDrawingStyle",
                                drawing_subroutines[_detect_method]);

    gpu_program_pipeline_setup_builder{raw_handle(_prog_pipeline)}
        .add_vertex_program(_vs_edge_detect)
        .add_fragment_program(_fs_edge_detect)
        .install();

    _fsquad_graphics.draw();
  }
}

void app::edge_detect_demo::update(const float /*delta_ms*/) {
  if (!_rotate_object)
    return;

  static constexpr auto ROTATION_SPEED_RADS = 0.01f;
  _rx += ROTATION_SPEED_RADS;
  if (_rx > two_pi<float>) {
    _rx -= two_pi<float>;
  }

  _ry += ROTATION_SPEED_RADS;
  if (_ry > two_pi<float>) {
    _ry -= two_pi<float>;
  }

  _rz += ROTATION_SPEED_RADS;
  if (_rz > two_pi<float>) {
    _rz -= two_pi<float>;
  }
}

void app::edge_detect_demo::key_event(const int32_t /*key_code*/,
                                      const int32_t /*action*/,
                                      const int32_t /*mods*/) {}

void app::edge_detect_demo::resize_event(const resize_context_t& rctx) {
  assert(valid());

  const auto r_width  = static_cast<GLsizei>(rctx.surface_width);
  const auto r_height = static_cast<GLsizei>(rctx.surface_height);

  create_framebuffer(r_width, r_height);
}

void app::edge_detect_demo::create_framebuffer(const GLsizei r_width,
                                               const GLsizei r_height) {
  _fbo.fbo_depth   = scoped_renderbuffer{};
  _fbo.fbo_object  = scoped_framebuffer{};
  _fbo.fbo_texture = scoped_texture{};

  _fbo.fbo_texture = [r_width, r_height]() {
    GLuint tex{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &tex);
    gl::TextureStorage2D(tex, 1, gl::RGBA8, r_width, r_height);

    return tex;
  }();

  _fbo.fbo_depth = [r_width, r_height]() {
    GLuint depth_fb{};
    gl::CreateRenderbuffers(1, &depth_fb);
    gl::NamedRenderbufferStorage(depth_fb, gl::DEPTH_COMPONENT, r_width,
                                 r_height);
    return depth_fb;
  }();

  if (!_fbo.fbo_sampler) {
    _fbo.fbo_sampler = []() {
      GLuint smpl{};
      gl::CreateSamplers(1, &smpl);
      gl::SamplerParameteri(smpl, gl::TEXTURE_MIN_FILTER, gl::NEAREST);
      gl::SamplerParameteri(smpl, gl::TEXTURE_MAG_FILTER, gl::NEAREST);

      return smpl;
    }();
  }

  _fbo.fbo_object = [
    f_tex = raw_handle(_fbo.fbo_texture), f_depth = raw_handle(_fbo.fbo_depth)
  ]() {
    GLuint fbo{};
    gl::CreateFramebuffers(1, &fbo);
    gl::NamedFramebufferTexture(fbo, gl::COLOR_ATTACHMENT0, f_tex, 0);
    gl::NamedFramebufferRenderbuffer(fbo, gl::DEPTH_ATTACHMENT,
                                     gl::RENDERBUFFER, f_depth);
    gl::NamedFramebufferDrawBuffer(fbo, gl::COLOR_ATTACHMENT0);

    if (gl::CheckNamedFramebufferStatus(fbo, gl::DRAW_FRAMEBUFFER) !=
        gl::FRAMEBUFFER_COMPLETE) {
      XR_LOG_CRITICAL("Failed to create framebuffer!");
      XR_NOT_REACHED();
    }

    return fbo;
  }
  ();
}

void app::edge_detect_demo::init(const init_context_t& ini_ctx) {
  const auto render_wnd_width  = static_cast<GLsizei>(ini_ctx.surface_width);
  const auto render_wnd_height = static_cast<GLsizei>(ini_ctx.surface_height);

  OUTPUT_DBG_MSG("Init window size %f x %f", render_wnd_width,
                 render_wnd_height);

  _sampler_obj = []() {
    GLuint smp_obj{};
    gl::CreateSamplers(1, &smp_obj);
    gl::SamplerParameteri(smp_obj, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smp_obj, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return smp_obj;
  }();

  create_framebuffer(render_wnd_width, render_wnd_height);

  {
    _vertex_prg = gpu_program_builder{}
                      .add_file("shaders/cap6/edge_detect/shader.vert")
                      .build<graphics_pipeline_stage::vertex>();

    _frag_prg = gpu_program_builder{}
                    .add_file("shaders/cap6/edge_detect/shader.frag")
                    .build<graphics_pipeline_stage::fragment>();
  }

  {
    _vs_edge_detect =
        gpu_program_builder{}
            .add_file("shaders/cap6/edge_detect/passthrough.vert.glsl")
            .build<graphics_pipeline_stage::vertex>();

    _fs_edge_detect =
        gpu_program_builder{}
            .add_file("shaders/cap6/edge_detect/edge_detect.frag.glsl")
            .build<graphics_pipeline_stage::fragment>();
  }

  if (!_vertex_prg || !_frag_prg || !_vs_edge_detect || !_fs_edge_detect) {
    XR_NOT_REACHED();
  }

  _prog_pipeline = scoped_program_pipeline_handle{[]() {
    GLuint id{};
    gl::CreateProgramPipelines(1, &id);
    return id;
  }()};

  config_file app_cfg{"config/cap6/edge_detect/app.conf"};
  if (!app_cfg) {
    XR_LOG_CRITICAL("Config file error {}", app_cfg.error());
    XR_NOT_REACHED();
  }

  {
    const char* model_file{nullptr};
    if (!app_cfg.lookup_value("app.scene.model", model_file)) {
      XR_LOG_CRITICAL("Failed to read model file name from config!");
      XR_NOT_REACHED();
    }

    _object = simple_mesh{vertex_format::pnt,
                          c_str_ptr(ini_ctx.cfg->model_path(model_file))};

    if (!_object) {
      XR_LOG_CRITICAL("Failed to load model from file!");
      XR_NOT_REACHED();
    }

    _obj_graphics = mesh_graphics_rep{_object};
    if (!_obj_graphics) {
      XR_LOG_CRITICAL("Failed to create mesh graphical representation!");
    }
  }

  {
    _lightcount =
        xray::math::min<uint32_t>(scene_loader::parse_point_lights_section(
                                      app_cfg.lookup_entry("app.scene.lights"),
                                      _lights, XR_U32_COUNTOF__(_lights)),
                                  edge_detect_demo::max_lights);

    if (!_lightcount) {
      XR_LOG_CRITICAL("Lights section not found in config file!");
      XR_NOT_REACHED();
    }
  }

  _obj_material = [&app_cfg, &ini_ctx]() {
    const char* material_file{};
    app_cfg.lookup_value("app.scene.material.file", material_file);
    if (!material_file)
      return GLuint{};

    bool flip_yaxis{false};
    app_cfg.lookup_value("app.scene.material.flip_y", flip_yaxis);

    texture_loader tex_ldr{c_str_ptr(ini_ctx.cfg->texture_path(material_file)),
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

  {
    geometry_data_t quad_geom{};
    geometry_factory::fullscreen_quad(&quad_geom);

    _fsquad          = simple_mesh{vertex_format::p, quad_geom};
    _fsquad_graphics = mesh_graphics_rep{_fsquad};
    //    assert(_fsquad_graphics && "Failed to create mesh GPU data!");
  }

  _valid = true;
}
