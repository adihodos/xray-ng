#include "terrain_demo.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/pod_zero.hpp"
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
#include <ktx.h>
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

xray::rendering::scoped_texture
load_texture_from_file(const char*           file_path,
                       xray::math::vec3ui32* texture_dimensions) {
  xray::rendering::scoped_texture texture{};

  try {
    platformstl::memory_mapped_file      texture_file{file_path};
    xray::base::pod_zero<KTX_dimensions> tex_dimensions{};
    GLenum                               texture_target{};

    const auto load_result =
      ktxLoadTextureM(texture_file.memory(),
                      static_cast<GLsizei>(texture_file.size()),
                      raw_handle_ptr(texture),
                      &texture_target,
                      &tex_dimensions,
                      nullptr,
                      nullptr,
                      nullptr,
                      nullptr);

    if (load_result != KTX_SUCCESS) {
      XR_DBG_MSG(
        "Failed to load texture %s, error code %d", file_path, load_result);
    } else {
      if (texture_dimensions) {
        texture_dimensions->x = tex_dimensions.width;
        texture_dimensions->y = tex_dimensions.height;
        texture_dimensions->z = tex_dimensions.depth;
      }
    }
  } catch (const std::exception& ex) {
    XR_DBG_MSG("Failed to open texture file %s", file_path);
  }

  return texture;
}

struct null_height_gen {
  float operator()() const noexcept { return {}; }
};

class terrain_generator {
public:
  template <typename HeightGenerator>
  static void make_terrain(const int32_t            tiles_x,
                           const int32_t            tiles_y,
                           const float              tile_step,
                           const HeightGenerator&   hgen,
                           std::vector<vertex_pnt>& terrain_vertices,
                           std::vector<uint32_t>&   terrain_indices);
};

template <typename HeightGenerator>
void terrain_generator::make_terrain(const int32_t            tiles_x,
                                     const int32_t            tiles_z,
                                     const float              tile_step,
                                     const HeightGenerator&   hgen,
                                     std::vector<vertex_pnt>& terrain_vertices,
                                     std::vector<uint32_t>&   terrain_indices) {

  const auto vertex_count = (tiles_x + 1) * (tiles_z + 1);
  const auto delta_uv     = vec2f{1.0f / static_cast<float>(tiles_x),
                              1.0f / static_cast<float>(tiles_z)};

  const auto hh = (tiles_z + 1) * tile_step * 0.5f;
  const auto hw = (tiles_x + 1) * tile_step * 0.5f;

  for (int32_t z = 0; z <= tiles_z; ++z) {
    for (int32_t x = 0; x <= tiles_x; ++x) {
      vertex_pnt v;
      v.position = vec3f{static_cast<float>(x) * tile_step - hw,
                         0.0f,
                         static_cast<float>(z) * tile_step - hh};
      v.normal   = vec3f::stdc::unit_y;
      v.texcoord = vec2f{static_cast<float>(x) * delta_uv.x,
                         static_cast<float>(z) * delta_uv.y};

      terrain_vertices.push_back(v);
    }
  }

  assert(terrain_vertices.size() == vertex_count);

  const auto index_count = tiles_x * tiles_z * 2 * 3;

  for (int32_t z = 0; z < tiles_z; ++z) {
    for (int32_t x = 0; x < tiles_x; ++x) {
      terrain_indices.push_back((z + 1) * (tiles_x + 1) + x);
      terrain_indices.push_back((z) * (tiles_x + 1) + x + 1);
      terrain_indices.push_back((z) * (tiles_x + 1) + x);

      terrain_indices.push_back((z + 1) * (tiles_x + 1) + x);
      terrain_indices.push_back((z + 1) * (tiles_x + 1) + x + 1);
      terrain_indices.push_back((z) * (tiles_x + 1) + x + 1);
    }
  }

  assert(terrain_indices.size() == index_count);
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

  // {
  //   std::vector<vertex_pnt> v;
  //   std::vector<uint32_t>   i;
  //   terrain_generator::make_terrain(3, 2, 1.0f, null_height_gen{}, v, i);

  //   unique_ptr<FILE, decltype(&fclose)> fp{fopen("terr.txt", "wt"), &fclose};
  //   for_each(begin(v), end(v), [f = fp.get()](const vertex_pnt& v) {
  //     fprintf(
  //       f, "\n%3.3f, %3.3f, %3.3f", v.position.x, v.position.y,
  //       v.position.z);
  //   });

  //   fprintf(fp.get(), "\n###############");
  //   for (size_t j = 0; j < i.size() / 3; ++j) {
  //     fprintf(fp.get(), "\n%u %u %u", i[j * 3 + 0], i[j * 3 + 1], i[j * 3 +
  //     2]);
  //   }

  //   fprintf(fp.get(), "\n###############");

  //   fflush(fp.get());
  //   for_each(begin(v), end(v), [f = fp.get()](const vertex_pnt& v) {
  //     fprintf(f, "\n%3.3f, %3.3f", v.texcoord.x, v.texcoord.y);
  //   });
  // }

  //
  // turn off these so we don't get spammed
  gl::DebugMessageControl(gl::DONT_CARE,
                          gl::DONT_CARE,
                          gl::DEBUG_SEVERITY_NOTIFICATION,
                          0,
                          nullptr,
                          gl::FALSE_);

  vector<vertex_pnt> vertices;
  vector<uint32_t>   indices;
  terrain_generator::make_terrain(
    1023, 1023, 1.0f, null_height_gen{}, vertices, indices);

  _mesh = basic_mesh{vertices.data(),
                     vertices.size(),
                     indices.data(),
                     indices.size(),
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

  gl::CreateSamplers(1, raw_handle_ptr(_sampler));
  const auto smp = raw_handle(_sampler);
  gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR_MIPMAP_LINEAR);
  gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
  gl::SamplerParameteri(smp, gl::TEXTURE_WRAP_S, gl::CLAMP_TO_BORDER);
  gl::SamplerParameteri(smp, gl::TEXTURE_WRAP_T, gl::CLAMP_TO_BORDER);

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

  {

    //    scoped_winding_order_setting set_cw{gl::CW};

    const GLuint bound_textures[] = {raw_handle(_objtex),
                                     raw_handle(_colormap)};

    gl::BindTextures(0, XR_I32_COUNTOF(bound_textures), bound_textures);

    const GLuint bound_samplers[] = {raw_handle(_sampler),
                                     raw_handle(_sampler)};

    gl::BindSamplers(0, XR_I32_COUNTOF(bound_samplers), bound_samplers);

    gl::BindVertexArray(_mesh.vertex_array());

    _vs.set_uniform("world_view_proj_mtx",
                    _camera.projection_view() *
                      mat4f{R3::scale_y(_terrain_opts.scaling)});
    _vs.set_uniform("scale_factor", _terrain_opts.scaling);

    _pipeline.use();

    if (_terrain_opts.wireframe) {
      scoped_polygon_mode_setting set_wireframe{gl::LINE};
      gl::DrawElements(
        gl::TRIANGLES, _mesh.index_count(), gl::UNSIGNED_INT, nullptr);
    } else {
      gl::DrawElements(
        gl::TRIANGLES, _mesh.index_count(), gl::UNSIGNED_INT, nullptr);
    }
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

    ImGui::DragFloat("Scaling", &_terrain_opts.scaling, 0.25f, 1.0f, 16.0f);
    ImGui::Separator();
    ImGui::Checkbox("Draw wireframe", &_terrain_opts.wireframe);
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
