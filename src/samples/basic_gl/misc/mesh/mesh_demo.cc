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
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/ui.hpp"
#include "xray/ui/user_interface.hpp"
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

struct mesh_load_info {
  const char* display_name;
  const char* file_name;
} const loadable_meshes[] = {{"A10 Thunderbolt II", "a10/a10.bin"},
                             {"A4", "a4/a4.bin"},
                             {"F15C Eagle", "f15/f15c.bin"},
                             {"F4 Phantom II", "f4/f4phantom.bin"},
                             {"Leopard I", "leo1/leo1.bin"},
                             {"Leopard 2A6", "leo2/leo2a6.bin"},
                             {"P38 Lighting", "p38/p38.bin"},
                             {"SA 23 Starfury Aurora", "sa23/sa23_aurora.bin"},
                             {"Eurofighter Typhoon", "typhoon/typhoon.bin"}

};

app::mesh_demo::mesh_demo(const init_context_t* init_ctx)
  : demo_base{init_ctx} {
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

  gl::CreateBuffers(1, raw_handle_ptr(_vb));
  gl::CreateBuffers(1, raw_handle_ptr(_ib));

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

void app::mesh_demo::draw(const float surface_width,
                          const float surface_height) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);
  gl::ViewportIndexedf(0, 0.0f, 0.0f, surface_width, surface_height);

  if (!_mesh_loaded) {
    return;
  }

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
    dc.window_width     = surface_width;
    dc.window_height    = surface_height;
    dc.view_matrix      = _camera.view();
    dc.proj_view_matrix = _camera.projection_view();
    dc.active_camera    = &_camera;
    _abbdraw.draw(dc, _bbox, color_palette::web::sea_green, 4.0f);
  }
}

nk_color
xray_color_to_nk_color(const xray::rendering::rgb_color& clr) noexcept {
  return {static_cast<uint8_t>(clr.r * 255.0f),
          static_cast<uint8_t>(clr.g * 255.0f),
          static_cast<uint8_t>(clr.b * 255.0f),
          static_cast<uint8_t>(clr.a * 255.0f)};
}

xray::rendering::rgb_color nk_color_to_xray_color(const nk_color clr) noexcept {
  return {static_cast<float>(clr.r) / 255.0f,
          static_cast<float>(clr.g) / 255.0f,
          static_cast<float>(clr.b) / 255.0f,
          static_cast<float>(clr.a) / 255.0f};
}

void app::mesh_demo::compose_ui() {
  auto ctx = _ui.ctx();
  if (nk_begin(ctx,
               "Options",
               nk_rect(0.0f, 0.0f, 300.0f, 400.0f),
               NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE |
                 NK_WINDOW_TITLE | NK_WINDOW_MOVABLE)) {

    nk_layout_row_dynamic(ctx, 25.0f, 1);
    nk_label(ctx, "Model", NK_TEXT_ALIGN_CENTERED);

    int32_t selected{_sel_id == -1 ? 0 : _sel_id};
    nk_combobox_callback(ctx,
                         [](void*, int id, const char** outstr) {
                           *outstr = loadable_meshes[id].display_name;
                         },
                         nullptr,
                         &selected,
                         XR_I32_COUNTOF(loadable_meshes),
                         25,
                         nk_vec2(200.0f, 200.0f));

    if (selected != _sel_id) {
      XR_DBG_MSG("Changing mesh from {} to {}", _sel_id, selected);
      _sel_id = selected;

      _mesh_info = load_mesh(
        xr_app_config->model_path(loadable_meshes[_sel_id].file_name).c_str());
    }

    if (_mesh_info) {
      nk_layout_row_static(ctx, 320, 200, 1);
      if (nk_group_begin(ctx,
                         "Mesh info",
                         NK_WINDOW_BORDER | NK_WINDOW_NO_SCROLLBAR |
                           NK_WINDOW_TITLE)) {

        const auto mi = _mesh_info.value();
        nk_layout_row_static(ctx, 18, 100, 1);
        nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Vertices %u", mi.vertices);
        nk_labelf(ctx, NK_TEXT_ALIGN_LEFT, "Indices %u", mi.indices);
        nk_group_end(ctx);
      }

      nk_layout_row_dynamic(ctx, 25.0f, 1);
      nk_checkbox_label(ctx, "Draw wireframe", &_drawparams.draw_wireframe);
      nk_checkbox_label(
        ctx, "Draw bounding box", &_drawparams.draw_boundingbox);
      nk_checkbox_label(ctx, "Draw normals", &_drawparams.drawnormals);

      if (_drawparams.drawnormals) {
        nk_label(ctx, "Normal color start", NK_TEXT_ALIGN_LEFT);
        const auto cs = nk_color_picker(
          ctx, xray_color_to_nk_color(_drawparams.start_color), NK_RGBA);

        _drawparams.start_color = nk_color_to_xray_color(cs);

        nk_label(ctx, "Normal color end", NK_TEXT_ALIGN_LEFT);
        const auto ce = nk_color_picker(
          ctx, xray_color_to_nk_color(_drawparams.end_color), NK_RGBA);

        _drawparams.end_color = nk_color_to_xray_color(ce);

        nk_label(
          ctx,
          fmt::format("Normal length {}", _drawparams.normal_len).c_str(),
          NK_TEXT_ALIGN_LEFT);

        _drawparams.normal_len = nk_propertyf(ctx,
                                              "Normal length",
                                              0.1f,
                                              _drawparams.normal_len,
                                              1.0f,
                                              0.05f,
                                              0.05f);
      }
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

  if (evt.type == event_type::key &&
      evt.event.key.keycode == key_sym::e::escape) {
    _quit_receiver();
    return;
  }

  _ui.ui_event(evt);
  if (!_ui.wants_input()) {
    _camcontrol.input_event(evt);
  }
}

void app::mesh_demo::poll_start(const xray::ui::poll_start_event&) {
  _ui.input_begin();
}

void app::mesh_demo::poll_end(const xray::ui::poll_end_event&) {
  _ui.input_end();
}

void app::mesh_demo::loop_event(const xray::ui::window_loop_event& wle) {
  compose_ui();
  draw(static_cast<float>(wle.wnd_width), static_cast<float>(wle.wnd_height));
  _ui.render(wle.wnd_width, wle.wnd_height);
}

xray::base::maybe<app::mesh_demo::mesh_info>
app::mesh_demo::load_mesh(const char* model_path) {
  mesh_loader mesh_loader{model_path};
  if (!mesh_loader) {
    return nothing{};
  }

  if (mesh_loader.header().vertex_count > _vertexcount) {
    XR_DBG_MSG("Reallocating vertex buffer!");
    gl::NamedBufferData(raw_handle(_vb),
                        mesh_loader.vertex_bytes(),
                        mesh_loader.vertex_data(),
                        gl::STREAM_DRAW);
  } else {
    scoped_resource_mapping srm{raw_handle(_vb),
                                gl::MAP_WRITE_BIT |
                                  gl::MAP_INVALIDATE_BUFFER_BIT,
                                mesh_loader.vertex_bytes()};

    if (!srm) {
      XR_DBG_MSG("Failed to map vertex buffer for mapping!");
      return nothing{};
    }

    memcpy(srm.memory(), mesh_loader.vertex_data(), mesh_loader.vertex_bytes());
  }

  _vertexcount = mesh_loader.header().vertex_count;

  if (mesh_loader.header().index_count > _indexcount) {
    XR_DBG_MSG("Reallocating index buffer!");
    gl::NamedBufferData(raw_handle(_ib),
                        mesh_loader.index_bytes(),
                        mesh_loader.index_data(),
                        gl::STREAM_DRAW);
  } else {
    scoped_resource_mapping srm{raw_handle(_ib),
                                gl::MAP_WRITE_BIT |
                                  gl::MAP_INVALIDATE_BUFFER_BIT,
                                mesh_loader.index_bytes()};

    if (!srm) {
      XR_DBG_MSG("Failed to map index buffer for mapping");
      return nothing{};
    }

    memcpy(srm.memory(), mesh_loader.index_data(), mesh_loader.index_bytes());
  }

  _indexcount  = mesh_loader.header().index_count;
  _bbox        = mesh_loader.bounding().axis_aligned_bbox;
  _mesh_loaded = true;

  return mesh_info{mesh_loader.header().vertex_count,
                   mesh_loader.header().index_count,
                   mesh_loader.vertex_bytes(),
                   mesh_loader.index_bytes(),
                   mesh_loader.bounding().axis_aligned_bbox,
                   mesh_loader.bounding().bounding_sphere};
}
