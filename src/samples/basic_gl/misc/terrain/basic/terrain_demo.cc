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

#include <png.h>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

class image_loader {
public:
  image_loader() = default;

  void load(const char* path);

  uint32_t width() const noexcept { return _width; }

  uint32_t height() const noexcept { return _height; }

  uint32_t pixel_size() const noexcept { return _pixel_size; }

  uint32_t pixel_format() const noexcept { return _gl_fmt; }

  uint32_t pixel_store_format() const noexcept { return _gl_internal_fmt; }

  const uint8_t* pixels() const noexcept {
    return xray::base::raw_ptr(_pixels);
  }

  explicit operator bool() const noexcept { return valid(); }

private:
  struct malloc_deleter {
    void operator()(void* p) const noexcept { free(p); }
  };

  bool valid() const noexcept { return _pixels != nullptr; }

  struct impl_details;
  xray::base::unique_pointer<impl_details>            _impl;
  uint32_t                                            _width{};
  uint32_t                                            _height{};
  int32_t                                             _bitdepth{};
  int32_t                                             _colortype{};
  int32_t                                             _channels{};
  xray::base::unique_pointer<uint8_t, malloc_deleter> _pixels;
  uint32_t                                            _pixel_size{};
  uint32_t                                            _gl_fmt{};
  uint32_t                                            _gl_internal_fmt{};
  XRAY_NO_COPY(image_loader);
};

struct image_loader::impl_details {
  png_structp png_ptr;
  png_infop   png_info;
  png_infop   png_end_info;
};

void image_loader::load(const char* path) {
  _impl = xray::base::make_unique<impl_details>();
  unique_pointer<FILE, decltype(&fclose)> fp{fopen(path, "rb"), &fclose};
  if (!fp) {
    XR_DBG_MSG("Failed to open image {}", path);
    return;
  }

  png_byte header[8];
  fread(header, sizeof(header), 1, raw_ptr(fp));

  if (png_sig_cmp(header, 0, 8)) {
    XR_DBG_MSG("PNG signature invalid!");
    return;
  }

  _impl->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (!_impl->png_ptr) {
    return;
  }

  _impl->png_info = png_create_info_struct(_impl->png_ptr);
  if (!_impl->png_info) {
    return;
  }

  png_init_io(_impl->png_ptr, raw_ptr(fp));
  png_set_sig_bytes(_impl->png_ptr, sizeof(header));

  png_read_info(_impl->png_ptr, _impl->png_info);

  _width     = png_get_image_width(_impl->png_ptr, _impl->png_info);
  _height    = png_get_image_width(_impl->png_ptr, _impl->png_info);
  _bitdepth  = png_get_bit_depth(_impl->png_ptr, _impl->png_info);
  _colortype = png_get_color_type(_impl->png_ptr, _impl->png_info);
  _channels  = png_get_channels(_impl->png_ptr, _impl->png_info);

  switch (_colortype) {
  case PNG_COLOR_TYPE_GRAY:
    _gl_fmt = gl::RED;
    break;

  case PNG_COLOR_TYPE_PALETTE:
  case PNG_COLOR_TYPE_RGB:
    _gl_fmt = gl::RGB;
    break;

  case PNG_COLOR_TYPE_RGBA:
    _gl_fmt = gl::RGBA;
    break;

  default:
    assert(false && "Unhandled color type!");
    break;
  }

  if (_colortype == PNG_COLOR_TYPE_GRAY && _bitdepth < 8) {
    png_set_expand_gray_1_2_4_to_8(_impl->png_ptr);
    _bitdepth = 8;
  }

  if (_colortype == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(_impl->png_ptr);
  }

  if (png_get_valid(_impl->png_ptr, _impl->png_info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(_impl->png_ptr);
  }

  assert(_bitdepth % 8 == 0);

  const uint32_t internal_fmt[][3] = {{gl::R8, gl::R16, gl::R32F},
                                      {gl::RG8, gl::RG16, gl::RG32F},
                                      {gl::RGB8, gl::RGB16, gl::RGB32F},
                                      {gl::RGBA8, gl::RGBA16, gl::RGBA32F}};

  const uint32_t(&fmtref)[3] = internal_fmt[_bitdepth / 8];
  //  const auto int_fmt         = [fmtref](const uint32_t ctype) {
  //    switch (ctype) { case PNG_COLOR_TYPE_GRAY: }
  //  }();

  switch (_bitdepth) {
  case 8:
    _pixel_size = gl::UNSIGNED_BYTE;
    break;

  case 16:
    _pixel_size = gl::UNSIGNED_SHORT;
    break;

  case 32:
    _pixel_size = gl::UNSIGNED_INT;
    break;

  default:
    assert(false && "Unhandled bit depth!");
    break;
  }

  png_read_update_info(_impl->png_ptr, _impl->png_info);

  auto row_bytes = png_get_rowbytes(_impl->png_ptr, _impl->png_info);
  row_bytes += 3 - ((row_bytes - 1) % 4);

  _pixels = unique_pointer<uint8_t, malloc_deleter>{
    static_cast<uint8_t*>(malloc(row_bytes * _height * (_bitdepth / 8) + 15))};

  if (!_pixels) {
    return;
  }

  unique_pointer<png_byte*, malloc_deleter> row_ptrs{
    static_cast<png_byte**>(malloc(_height * sizeof(png_byte*)))};

  if (!row_ptrs) {
    return;
  }

  auto p = raw_ptr(row_ptrs);
  for (uint32_t i = 0; i < _height; ++i) {
    p[_height - 1 - i] = raw_ptr(_pixels) + i * row_bytes;
  }

  png_read_image(_impl->png_ptr, raw_ptr(row_ptrs));
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

class heightmap_loader {
public:
  heightmap_loader(const char* path);
  heightmap_loader(const std::string& path) : heightmap_loader{path.c_str()} {}

  gsl::span<const uint16_t> data() const noexcept {
    return gsl::as_span(_data, _width * _height);
  }

  int32_t width() const noexcept { return _width; }

  int32_t height() const noexcept { return _height; }

  explicit operator bool() const noexcept {
    return _data && (_width != 0) && (_height != 0);
  }

private:
  platformstl::memory_mapped_file _file;
  const uint16_t*                 _data{};
  int32_t                         _width{};
  int32_t                         _height{};

  XRAY_NO_COPY(heightmap_loader);
};

heightmap_loader::heightmap_loader(const char* path) {
  try {
    _file   = platformstl::memory_mapped_file{path};
    _data   = static_cast<const uint16_t*>(_file.memory());
    _width  = 513;
    _height = 513;
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

  {
    std::vector<vertex_pnt> v;
    std::vector<uint32_t>   i;
    terrain_generator::make_terrain(3, 2, 1.0f, null_height_gen{}, v, i);

    unique_ptr<FILE, decltype(&fclose)> fp{fopen("terr.txt", "wt"), &fclose};
    for_each(begin(v), end(v), [f = fp.get()](const vertex_pnt& v) {
      fprintf(
        f, "\n%3.3f, %3.3f, %3.3f", v.position.x, v.position.y, v.position.z);
    });

    fprintf(fp.get(), "\n###############");
    for (size_t j = 0; j < i.size() / 3; ++j) {
      fprintf(fp.get(), "\n%u %u %u", i[j * 3 + 0], i[j * 3 + 1], i[j * 3 + 2]);
    }

    fprintf(fp.get(), "\n###############");

    fflush(fp.get());
    for_each(begin(v), end(v), [f = fp.get()](const vertex_pnt& v) {
      fprintf(f, "\n%3.3f, %3.3f", v.texcoord.x, v.texcoord.y);
    });
  }

  //
  // turn off these so we don't get spammed
  gl::DebugMessageControl(gl::DONT_CARE,
                          gl::DONT_CARE,
                          gl::DEBUG_SEVERITY_NOTIFICATION,
                          0,
                          nullptr,
                          gl::FALSE_);

  //  heightmap_loader hmldr{
  //    xr_app_config->texture_path("worlds/test/terrain_heightmap.r16").c_str()};

  //  if (!hmldr) {
  //    XR_DBG_MSG("Failed to load heightmap!");
  //    return;
  //  }

  //  geometry_data_t terrain;
  //  geometry_factory::grid(static_cast<float>(_terrain_opts.width - 1),
  //                         static_cast<float>(_terrain_opts.height - 1),
  //                         static_cast<size_t>(_terrain_opts.width - 1),
  //                         static_cast<size_t>(_terrain_opts.height - 1),
  //                         &terrain);

  //  vector<vertex_pnt> vertices;
  //  vertices.reserve(terrain.vertex_count);
  //  const auto vspan = terrain.vertex_span();
  //  const auto hspan = hmldr.data();

  //  transform(begin(vspan),
  //            end(vspan),
  //            back_inserter(vertices),
  //            [](const vertex_pntt& vs_in) {
  //              return vertex_pnt{vs_in.position, vs_in.normal,
  //              vs_in.texcoords};
  //            });

  vector<vertex_pnt> vertices;
  vector<uint32_t>   indices;
  terrain_generator::make_terrain(
    1023, 1023, 1.0f, null_height_gen{}, vertices, indices);

  // for (int32_t z = 0; z < _terrain_opts.height; ++z) {
  //  for (int32_t x = 0; x < _terrain_opts.width; ++x) {
  //    // const vec3f pos{vspan[z * _terrain_opts.width + x].position.x,
  //    //                static_cast<float>(hspan[z * _terrain_opts.width + x])
  //    /
  //    //                  512.0f,
  //    //                vspan[z * _terrain_opts.width + x].position.z};

  //    // vertices.push_back({pos,
  //    //                    vspan[z * _terrain_opts.width + x].normal,
  //    //                    vspan[z * _terrain_opts.width + x].texcoords});
  //  }
  //}

  //  transform(begin(vspan),
  //            end(vspan),
  //            back_inserter(vertices),
  //            [hmap = hmldr.data()](const vertex_pntt& vsin) {
  //              return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
  //            });

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

  {
    image_loader tldr;
    tldr.load(
      xr_app_config->texture_path("worlds/test/heightmap01.png").c_str());

    if (!tldr) {
      return;
    }
    //    texture_loader tldr{
    //      xr_app_config->texture_path("worlds/test/heightmap01.png").c_str()};

    //    if (!tldr) {
    //      return;
    //    }

    gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_objtex));

    gl::TextureStorage2D(raw_handle(_objtex),
                         1,
                         tldr.pixel_store_format(),
                         tldr.width(),
                         tldr.height());

    //    gl::TextureStorage2D(raw_handle(_objtex), 1, gl::R8, 3, 2);

    gl::TextureSubImage2D(raw_handle(_objtex),
                          0,
                          0,
                          0,
                          tldr.width(),
                          tldr.height(),
                          tldr.pixel_format(),
                          tldr.pixel_size(),
                          tldr.pixels());

    //    const uint8_t ccolor = 0;
    //    gl::ClearTexImage(
    //      raw_handle(_objtex), 0, gl::RED, gl::UNSIGNED_BYTE, &ccolor);
  }

  {
    texture_loader tldr{
      xr_app_config->texture_path("uv_grids/ash_uvgrid01.jpg").c_str()};
    if (!tldr) {
      XR_DBG_MSG("Failed to load color map for terrain!");
      return;
    }

    gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_colormap));
    gl::TextureStorage2D(raw_handle(_colormap),
                         1,
                         tldr.internal_format(),
                         tldr.width(),
                         tldr.height());
    gl::TextureSubImage2D(raw_handle(_colormap),
                          0,
                          0,
                          0,
                          tldr.width(),
                          tldr.height(),
                          tldr.format(),
                          gl::UNSIGNED_BYTE,
                          tldr.data());
  }

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
