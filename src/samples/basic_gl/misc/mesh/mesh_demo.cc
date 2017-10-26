#include "misc/mesh/mesh_demo.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/mesh_loader.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/ui.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
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

app::mesh_demo::mesh_demo(const init_context_t* init_ctx) {
  _camera.set_projection(projections_rh::perspective_symmetric(
    static_cast<float>(init_ctx->surface_width) /
      static_cast<float>(init_ctx->surface_height),
    radians(70.0f),
    0.1f,
    100.0f));

  _camcontrol.update();
  init();
}

app::mesh_demo::~mesh_demo() {}

void app::mesh_demo::init() {
  assert(!valid());

  //
  // turn off these so we don't get spammed
  gl::DebugMessageControl(gl::DONT_CARE,
                          gl::DONT_CARE,
                          gl::DEBUG_SEVERITY_NOTIFICATION,
                          0,
                          nullptr,
                          gl::FALSE_);

  if (!_abbdraw)
    return;

  static constexpr auto MODEL_FILE =
    //"SuperCobra.3ds";
    "f4/f4phantom.obj";
  //"starfury/viper.obj"
  //"sa23/sa23_aurora.obj";
  //"cube_rounded.obj";
  //"stanford/dragon/dragon.obj";
  //"stanford/teapot/teapot.obj";
  //"stanford/sportscar/sportsCar.obj";
  //"stanford/sibenik/sibenik.obj";
  //"stanford/rungholt/rungholt.obj";
  //"stanford/lost-empire/lost_empire.obj";
  //"stanford/cube/cube.obj";
  //"stanford/head/head.OBJ";
  ;

  // geometry_data_t blob;
  // geometry_factory::grid(16.0f, 16.0f, 128, 128, &blob);

  // vector<vertex_pnt> verts;
  // transform(raw_ptr(blob.geometry),
  //          raw_ptr(blob.geometry) + blob.vertex_count,
  //          back_inserter(verts),
  //          [](const vertex_pntt& vsin) {
  //            return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
  //          });

  // const vertex_ripple_parameters rparams{_rippledata.amplitude,
  //                                       _rippledata.period * two_pi<float>,
  //                                       _rippledata.width,
  //                                       _rippledata.height};

  // vertex_effect::ripple(
  //  gsl::span<vertex_pnt>{verts},
  //  gsl::span<uint32_t>{raw_ptr(blob.indices),
  //                      raw_ptr(blob.indices) + blob.index_count},
  //  rparams);

  //_mesh = basic_mesh{verts.data(),
  //                   verts.size(),
  //                   raw_ptr(blob.indices),
  //                   blob.index_count,
  //                   mesh_type::writable};

  const char* const BIN_MDL = "f4/f4phantom.bin";
  //"starfury/viper.bin";

  const auto mesh_hdr =
    mesh_loader::read_header(xr_app_config->model_path(BIN_MDL).c_str());

  if (mesh_hdr) {
    auto h = &mesh_hdr.value();
    XR_LOG_INFO(
      "Vertex count {}, index count {}, vertex offset {}, index offset {}",
      h->vertex_count,
      h->index_count,
      h->vertex_offset,
      h->index_offset);
  }

  timer_highp tmr;

  {
    scoped_timing_object<timer_highp> stmr{&tmr};

    mesh_loader ldr{xr_app_config->model_path(BIN_MDL).c_str()};
    if (!ldr) {
      XR_LOG_ERR("Failed to load model!");
      return;
    }

    _bbox = ldr.bounding().axis_aligned_bbox;
    XR_DBG_MSG(
      "Bounding box {} x {}", string_cast(_bbox.min), string_cast(_bbox.max));

    _indexcount = ldr.header().index_count;

    gl::CreateBuffers(1, raw_handle_ptr(_vb));
    gl::NamedBufferStorage(
      raw_handle(_vb), ldr.vertex_bytes(), ldr.vertex_data(), 0);

    gl::CreateBuffers(1, raw_handle_ptr(_ib));
    gl::NamedBufferStorage(
      raw_handle(_ib), ldr.index_bytes(), ldr.index_data(), 0);
  }

  XR_LOG_INFO("Mesh creation millis {}", tmr.elapsed_millis());

  //  {
  //    scoped_timing_object<timer_highp> stmr{&tmr};
  //    _mesh = basic_mesh{xr_app_config->model_path(MODEL_FILE).c_str()};
  //  }

  //  XR_LOG_INFO("Mesh creation (obj) millis {}", tmr.elapsed_millis());

  {
    gl::CreateVertexArrays(1, raw_handle_ptr(_vao));
    gl::VertexArrayVertexBuffer(
      raw_handle(_vao), 0, raw_handle(_vb), 0, sizeof(vertex_pnt));
    gl::VertexArrayElementBuffer(raw_handle(_vao), raw_handle(_ib));

    gl::VertexArrayAttribFormat(raw_handle(_vao),
                                0,
                                3,
                                gl::FLOAT,
                                gl::FALSE_,
                                offsetof(vertex_pnt, position));
    gl::VertexArrayAttribFormat(raw_handle(_vao),
                                1,
                                3,
                                gl::FLOAT,
                                gl::FALSE_,
                                offsetof(vertex_pnt, normal));
    gl::VertexArrayAttribFormat(raw_handle(_vao),
                                2,
                                2,
                                gl::FLOAT,
                                gl::FALSE_,
                                offsetof(vertex_pnt, texcoord));

    gl::VertexArrayAttribBinding(raw_handle(_vao), 0, 0);
    gl::VertexArrayAttribBinding(raw_handle(_vao), 1, 0);
    gl::VertexArrayAttribBinding(raw_handle(_vao), 2, 0);

    gl::EnableVertexArrayAttrib(raw_handle(_vao), 0);
    gl::EnableVertexArrayAttrib(raw_handle(_vao), 1);
    gl::EnableVertexArrayAttrib(raw_handle(_vao), 2);
  }

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/mesh/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/misc/mesh/fs.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _vsnormals = gpu_program_builder{}
                 .add_file("shaders/misc/mesh/vs.pass.glsl")
                 .build<render_stage::e::vertex>();

  if (!_vsnormals)
    return;

  _gsnormals = gpu_program_builder{}
                 .add_file("shaders/misc/mesh/gs.glsl")
                 .build<render_stage::e::geometry>();

  if (!_gsnormals)
    return;

  _fsnormals = gpu_program_builder{}
                 .add_file("shaders/misc/mesh/fs.color.glsl")
                 .build<render_stage::e::fragment>();

  if (!_fsnormals)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  texture_loader tldr{
    xr_app_config->texture_path("uv_grids/ash_uvgrid01.jpg").c_str()};

  if (!tldr) {
    return;
  }

  gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_objtex));
  gl::TextureStorage2D(raw_handle(_objtex),
                       1,
                       tldr.internal_format(),
                       tldr.width(),
                       tldr.height());
  gl::TextureSubImage2D(raw_handle(_objtex),
                        0,
                        0,
                        0,
                        tldr.width(),
                        tldr.height(),
                        tldr.format(),
                        gl::UNSIGNED_BYTE,
                        tldr.data());

  {
    gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_greentexture));
    gl::TextureStorage2D(raw_handle(_greentexture), 1, gl::RGBA8, 1, 1);

    const uint8_t clr[] = {0, 255, 0, 0};
    gl::ClearTexImage(
      raw_handle(_greentexture), 0, gl::RGBA, gl::UNSIGNED_BYTE, clr);
  }

  gl::CreateSamplers(1, raw_handle_ptr(_sampler));
  const auto smp = raw_handle(_sampler);
  gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
  gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

  gl::Enable(gl::DEPTH_TEST);
  gl::Enable(gl::CULL_FACE);

  _valid = true;
}

void app::mesh_demo::draw(const xray::rendering::draw_context_t& draw_ctx) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  gl::ViewportIndexedf(0,
                       0.0f,
                       0.0f,
                       (float) draw_ctx.window_width,
                       (float) draw_ctx.window_height);

  gl::BindVertexArray(raw_handle(_vao));

  struct transform_matrices {
    mat4f world_view_proj;
    mat4f normals;
    mat4f view;
  } tfmat;

  tfmat.view            = _camera.view();
  tfmat.world_view_proj = _camera.projection_view();
  tfmat.normals         = tfmat.view;

  _vs.set_uniform_block("TransformMatrices", tfmat);
  _fs.set_uniform("IMAGE_TEX", 0);

  _pipeline.use_vertex_program(_vs)
    .use_fragment_program(_fs)
    .disable_stage(render_stage::e::geometry)
    .use();

  const GLuint bound_textures[] = {raw_handle(_objtex)};
  gl::BindTextures(0, 1, bound_textures);

  const GLuint bound_samplers[] = {raw_handle(_sampler)};
  gl::BindSamplers(0, 1, bound_samplers);

  {
    scoped_polygon_mode_setting set_wireframe{
      _drawparams.draw_wireframe ? gl::LINE : gl::FILL};

    // scoped_winding_order_setting set_cw{gl::CW};
    gl::DrawElements(gl::TRIANGLES, _indexcount, gl::UNSIGNED_INT, nullptr);
  }

  if (_drawparams.drawnormals) {
    struct {
      mat4f     WORLD_VIEW_PROJECTION;
      rgb_color COLOR_START;
      rgb_color COLOR_END;
      float     NORMAL_LENGTH;
    } gs_uniforms;

    gs_uniforms.WORLD_VIEW_PROJECTION = tfmat.world_view_proj;
    gs_uniforms.COLOR_START           = _drawparams.start_color;
    gs_uniforms.COLOR_END             = _drawparams.end_color;
    gs_uniforms.NORMAL_LENGTH         = _drawparams.normal_len;

    _gsnormals.set_uniform_block("TransformMatrices", gs_uniforms);

    _pipeline.use_vertex_program(_vsnormals)
      .use_geometry_program(_gsnormals)
      .use_fragment_program(_fsnormals)
      .use();

    gl::DrawElements(gl::TRIANGLES, _indexcount, gl::UNSIGNED_INT, nullptr);
  }

  if (_drawparams.draw_boundingbox) {
    draw_context_t dc;
    dc.window_width     = draw_ctx.window_width;
    dc.window_height    = draw_ctx.window_height;
    dc.view_matrix      = _camera.view();
    dc.proj_view_matrix = _camera.projection_view();
    dc.active_camera    = &_camera;
    _abbdraw.draw(dc, _bbox, color_palette::web::sea_green, 4.0f);
  }

  _ui.render(static_cast<int32_t>(draw_ctx.window_width),
             static_cast<int32_t>(draw_ctx.window_height));
}

nk_color
xray_color_to_nk_color(const xray::rendering::rgb_color& clr) noexcept {
  return {static_cast<uint8_t>(clr.r * 255.0f),
          static_cast<uint8_t>(clr.g * 255.0f),
          static_cast<uint8_t>(clr.b * 255.0f),
          static_cast<uint8_t>(clr.a * 255.0f)};
}

void app::mesh_demo::update(const float /* delta_ms */) {
  auto ctx = _ui.ctx();
  if (nk_begin(ctx,
               "Demo options",
               nk_rect(0.0f, 0.0f, 300.0f, 400.0f),
               NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE |
                 NK_WINDOW_TITLE | NK_WINDOW_MOVABLE)) {

    nk_layout_row_static(ctx, 32, 200, 1);
    nk_checkbox_label(ctx, "Draw wireframe", &_drawparams.draw_wireframe);
    nk_checkbox_label(ctx, "Draw bounding box", &_drawparams.draw_boundingbox);
    nk_checkbox_label(ctx, "Draw normals", &_drawparams.drawnormals);
    if (_drawparams.drawnormals) {
      const auto cs = nk_color_picker(
        ctx, xray_color_to_nk_color(_drawparams.start_color), NK_RGBA);
      const auto ce = nk_color_picker(
        ctx, xray_color_to_nk_color(_drawparams.end_color), NK_RGBA);

      nk_label(ctx,
               fmt::format("Normal length {}", _drawparams.normal_len).c_str(),
               NK_TEXT_ALIGN_LEFT);
      _drawparams.normal_len = nk_propertyf(
        ctx, "Normal length", 0.1f, _drawparams.normal_len, 1.0f, 0.05f, 0.05f);
    }
  }

  nk_end(ctx);
  _camcontrol.update();
}

void app::mesh_demo::event_handler(const xray::ui::window_event& evt) {
  if (evt.type == event_type::configure) {
    const auto cfg = &evt.event.configure;
    _camera.set_projection(projections_rh::perspective_symmetric(
      static_cast<float>(cfg->width) / static_cast<float>(cfg->height),
      radians(70.0f),
      0.1f,
      100.0f));

    return;
  }

  _ui.ui_event(evt);
  if (!_ui.wants_input()) {
    _camcontrol.input_event(evt);
  }
}

void app::mesh_demo::compose_ui() {

  // ImGui::SetNextWindowPos({0, 0}, ImGuiSetCond_Always);
  // ImGui::Begin("Options");

  // ImGui::Checkbox("Draw wireframe", &_drawparams.draw_wireframe);
  // ImGui::Checkbox("Draw normals", &_drawparams.drawnormals);

  // if (_drawparams.drawnormals) {
  //  ImGui::SliderFloat("Normal length", &_drawparams.normal_len, 0.1f, 3.0f);
  //  ImGui::SliderFloat3(
  //    "Start color", _drawparams.start_color.components, 0.0f, 1.0f);
  //  ImGui::SliderFloat3(
  //    "End color", _drawparams.end_color.components, 0.0f, 1.0f);
  //}

  // ImGui::Checkbox("Draw bounding box", &_drawparams.draw_boundingbox);

  // ImGui::End();

  // ImGui::Begin("Mesh parameters");
  // bool update_mesh =
  //  ImGui::SliderFloat("Amplitude", &_rippledata.amplitude, 0.1f, 10.0f);

  // update_mesh |= ImGui::SliderFloat("Period", &_rippledata.period, 0.0f,
  // 10.0f);  update_mesh |= ImGui::SliderFloat("Width", &_rippledata.width,
  // 0.1f, 32.0f);  update_mesh |= ImGui::SliderFloat("Height",
  // &_rippledata.height, 0.1f, 32.0f);

  // ImGui::End();

  // if (!update_mesh)
  //  return;

  // const vertex_ripple_parameters rparams{_rippledata.amplitude,
  //                                       _rippledata.period * two_pi<float>,
  //                                       _rippledata.width,
  //                                       _rippledata.height};

  // vertex_effect::ripple(_mesh.vertices(), _mesh.indices(), rparams);
  //_mesh.update_vertices();
}

void app::mesh_demo::poll_start(const xray::ui::poll_start_event&) {
  _ui.input_begin();
}

void app::mesh_demo::poll_end(const xray::ui::poll_end_event&) {
  _ui.input_end();
}
