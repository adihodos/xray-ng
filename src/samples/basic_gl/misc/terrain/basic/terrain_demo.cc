#include "terrain_demo.hpp"
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
#include "xray/ui/user_interface.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iterator>
#include <opengl/opengl.hpp>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <span.h>
#include <stb/stb_image.h>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

class heightmap_loader {
public:
  heightmap_loader(const char* path);
  heightmap_loader(const std::string& path) : heightmap_loader{path.c_str()} {}

  gsl::span<const uint8_t> data() const noexcept {
    return gsl::as_span(_data, _width * _height);
  }

  int32_t width() const noexcept { return _width; }

  int32_t height() const noexcept { return _height; }

  explicit operator bool() const noexcept {
    return _data && (_width != 0) && (_height != 0);
  }

private:
  platformstl::memory_mapped_file _file;
  const uint8_t*                  _data{};
  int32_t                         _width{};
  int32_t                         _height{};

  XRAY_NO_COPY(heightmap_loader);
};

heightmap_loader::heightmap_loader(const char* path) {
  try {
    _file   = platformstl::memory_mapped_file{path};
    _data   = static_cast<const uint8_t*>(_file.memory()) + 100;
    _width  = *reinterpret_cast<const int32_t*>(_file.memory());
    _height = *(reinterpret_cast<const int32_t*>(_file.memory()) + 1);
  } catch (...) {
    return;
  }
}

app::terrain_demo::terrain_demo(const init_context_t& init_ctx)
  : demo_base{init_ctx} {
  _camera.set_projection(projections_rh::perspective_symmetric(
    static_cast<float>(init_ctx.surface_width) /
      static_cast<float>(init_ctx.surface_height),
    radians(70.0f),
    0.1f,
    1000.0f));

  _camcontrol.update();
  init();
  _ui->set_global_font("UbuntuMono-Regular");
}

app::terrain_demo::~terrain_demo() {}

void app::terrain_demo::init() {
  assert(!valid());

  //
  // turn off these so we don't get spammed
  gl::DebugMessageControl(gl::DONT_CARE,
                          gl::DONT_CARE,
                          gl::DEBUG_SEVERITY_NOTIFICATION,
                          0,
                          nullptr,
                          gl::FALSE_);

  heightmap_loader hmldr{
    xr_app_config->texture_path("heightmaps/terrain_basic/terrain.hmp")
      .c_str()};

  if (!hmldr) {
    XR_DBG_MSG("Failed to load heightmap!");
    return;
  }

  geometry_data_t terrain;
  geometry_factory::grid(static_cast<float>(_terrain_opts.width / 2.0f),
                         static_cast<float>(_terrain_opts.height / 2.0f),
                         static_cast<size_t>(_terrain_opts.width - 1),
                         static_cast<size_t>(_terrain_opts.height - 1),
                         &terrain);

  vector<vertex_pnt> vertices;
  vertices.reserve(terrain.vertex_count);
  const auto vspan = terrain.vertex_span();
  const auto hspan = hmldr.data();

  for (int32_t z = 0; z < _terrain_opts.height; ++z) {
    for (int32_t x = 0; x < _terrain_opts.width; ++x) {
      const vec3f pos{vspan[z * _terrain_opts.width + x].position.x,
                      static_cast<float>(hspan[z * _terrain_opts.width + x]) /
                        255.0f,
                      vspan[z * _terrain_opts.width + x].position.z};

      vertices.push_back({pos,
                          vspan[z * _terrain_opts.width + x].normal,
                          vspan[z * _terrain_opts.width + x].texcoords});
    }
  }

  //  transform(begin(vspan),
  //            end(vspan),
  //            back_inserter(vertices),
  //            [hmap = hmldr.data()](const vertex_pntt& vsin) {
  //              return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
  //            });

  _mesh = basic_mesh{vertices.data(),
                     vertices.size(),
                     terrain.index_data(),
                     terrain.index_count,
                     mesh_type::writable};
  if (!_mesh) {
    XR_DBG_MSG("Failed to create grid!");
    return;
  }

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/terrain/basic/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/misc/terrain/basic/fs.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

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

  gl::CreateSamplers(1, raw_handle_ptr(_sampler));
  const auto smp = raw_handle(_sampler);
  gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
  gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

  gl::Enable(gl::DEPTH_TEST);
  gl::Enable(gl::CULL_FACE);

  _valid = true;
}

void app::terrain_demo::draw(const float surface_width,
                             const float surface_height) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);
  gl::ViewportIndexedf(0, 0.0f, 0.0f, surface_width, surface_height);

  //  const GLuint bound_textures[] = {raw_handle(_objtex)};
  //  gl::BindTextures(0, 1, bound_textures);

  //  const GLuint bound_samplers[] = {raw_handle(_sampler)};
  //  gl::BindSamplers(0, 1, bound_samplers);

  {
    scoped_polygon_mode_setting  set_wireframe{gl::LINE};
    scoped_winding_order_setting set_cw{gl::CW};

    gl::BindVertexArray(_mesh.vertex_array());

    _vs.set_uniform("world_view_proj_mtx",
                    _camera.projection_view() *
                      mat4f{R3::scale_y(_terrain_opts.scaling)});
    _pipeline.use();
    gl::DrawElements(
      gl::TRIANGLES, _mesh.index_count(), gl::UNSIGNED_INT, nullptr);
  }

  //  if (_drawparams.drawnormals) {
  //    struct {
  //      mat4f     WORLD_VIEW_PROJECTION;
  //      rgb_color COLOR_START;
  //      rgb_color COLOR_END;
  //      float     NORMAL_LENGTH;
  //    } gs_uniforms;

  //    gs_uniforms.WORLD_VIEW_PROJECTION = tfmat.world_view_proj;
  //    gs_uniforms.COLOR_START           = _drawparams.start_color;
  //    gs_uniforms.COLOR_END             = _drawparams.end_color;
  //    gs_uniforms.NORMAL_LENGTH         = _drawparams.normal_len;

  //    _gsnormals.set_uniform_block("TransformMatrices", gs_uniforms);

  //    _pipeline.use_vertex_program(_vsnormals)
  //      .use_geometry_program(_gsnormals)
  //      .use_fragment_program(_fsnormals)
  //      .use();

  //    gl::DrawElements(gl::TRIANGLES, _indexcount, gl::UNSIGNED_INT, nullptr);
  //  }

  //  if (_drawparams.draw_boundingbox) {
  //    draw_context_t dc;
  //    dc.window_width     = surface_width;
  //    dc.window_height    = surface_height;
  //    dc.view_matrix      = _camera.view();
  //    dc.proj_view_matrix = _camera.projection_view();
  //    dc.active_camera    = &_camera;
  //    _abbdraw.draw(dc, _bbox, color_palette::web::sea_green, 4.0f);
  //  }
}

void app::terrain_demo::draw_ui(const int32_t surface_width,
                                const int32_t surface_height) {
  _ui->new_frame(surface_width, surface_height);

  ImGui::SetNextWindowPos({0.0f, 0.0f}, ImGuiCond_Appearing);

  if (ImGui::Begin("Options",
                   nullptr,
                   ImGuiWindowFlags_ShowBorders |
                     ImGuiWindowFlags_AlwaysAutoResize)) {

    ImGui::SliderFloat("Scaling", &_terrain_opts.scaling, 1.0f, 255.0f);
  }

  ImGui::End();
  _ui->draw();
}

void app::terrain_demo::event_handler(const xray::ui::window_event& evt) {
  if (evt.type == event_type::configure) {
    const auto cfg = &evt.event.configure;
    _camera.set_projection(projections_rh::perspective_symmetric(
      static_cast<float>(cfg->width) / static_cast<float>(cfg->height),
      radians(70.0f),
      0.1f,
      1000.0f));

    return;
  }

  if (is_input_event(evt)) {
    if (evt.type == event_type::key &&
        evt.event.key.keycode == key_sym::e::escape) {
      _quit_receiver();
      return;
    }

    _ui->input_event(evt);
    if (!_ui->wants_input()) {
      _camcontrol.input_event(evt);
    }
  }
}

void app::terrain_demo::loop_event(const xray::ui::window_loop_event& wle) {
  _camcontrol.update();
  _ui->tick(1.0f / 60.0f);

  draw(static_cast<float>(wle.wnd_width), static_cast<float>(wle.wnd_height));
  draw_ui(wle.wnd_width, wle.wnd_height);
}
