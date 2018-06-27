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
#include <noise/noise.h>
#include <noise/noiseutils.h>
#include <opengl/opengl.hpp>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

void compute_normal_map(const int32_t width, const int32_t depth) {
  //
  // sample 3x3 around pixel
  using xray::math::max;

  for (int32_t z = 0; z < depth; ++z) {
    for (int32_t x = 0; x < width; ++x) {

      const int32_t idx = z * depth + x;

      const auto is0 = max((z - 1) * depth + x - 1, 0);
      const auto is1 = max((z - 1) * depth + x, 0);
      const auto is2 = max((z - 1) * depth + x + 1, 0);

      const auto is3 = max(z * depth + x - 1, 0);
      const auto is4 = max(z * depth + x, 0);
      const auto is5 = max(z * depth + x + 1, 0);

      const auto is6 = max((z + 1) * depth + x - 1, 0);
      const auto is7 = max((z + 1) * depth + x, 0);
      const auto is8 = max((z + 1) * depth + x + 1, 0);
    }
  }
}

struct per_instance_transform {
  mat4f    world;
  mat4f    world_view_proj;
  uint32_t instance_vertex_offset;
  uint32_t pad[3];
};

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

  //
  // Create basic grid mesh.
  {
    geometry_data_t grid_geometry;
    geometry_factory::grid(terrain_consts::TILE_WIDTH,
                           terrain_consts::TILE_DEPTH,
                           terrain_consts::VERTICES_X,
                           terrain_consts::VERTICES_Z,
                           &grid_geometry);

    std::vector<vertex_pnt> vertices;
    const auto              vspan = grid_geometry.vertex_span();
    transform(
      begin(vspan),
      end(vspan),
      back_inserter(vertices),
      [](const vertex_pntt& vs_in) {
        return vertex_pnt{vs_in.position, vs_in.normal, vs_in.texcoords};
      });

    _mesh = basic_mesh{vertices.data(),
                       vertices.size(),
                       grid_geometry.index_data(),
                       grid_geometry.index_count,
                       mesh_type::writable};
    if (!_mesh) {
      XR_DBG_MSG("Failed to create grid!");
      return;
    }
  }

  //
  // Setup colormap texture array
  {
    gl::CreateTextures(gl::TEXTURE_2D_ARRAY, 1, raw_handle_ptr(_colormap));
    gl::TextureStorage3D(raw_handle(_colormap),
                         1,
                         gl::RGBA8,
                         terrain_consts::VERTICES_X,
                         terrain_consts::VERTICES_Z,
                         terrain_consts::NUM_TILES);

    constexpr const GLint swizzle_mask[] = {
      gl::ALPHA, gl::BLUE, gl::GREEN, gl::RED};

    gl::TextureParameteriv(
      raw_handle(_colormap), gl::TEXTURE_SWIZZLE_RGBA, swizzle_mask);
  }

  //
  // Generate height maps, color maps
  std::vector<float> heightmap_vertices;
  heightmap_vertices.reserve(_mesh.vertex_count() * terrain_consts::NUM_TILES);

  constexpr const vec4f TILE_BOUNDS[] = {vec4f{0.0f, 4.0f, 0.0f, 4.0f},
                                         vec4f{4.0f, 8.0f, 0.0f, 4.0f},
                                         vec4f{8.0f, 12.0f, 0.0f, 4.0f}};

  noise::module::Perlin              noise_module{};
  noise::utils::NoiseMap             height_map{};
  noise::utils::NoiseMapBuilderPlane heightmap_builder{};
  heightmap_builder.SetSourceModule(noise_module);
  heightmap_builder.SetDestNoiseMap(height_map);
  heightmap_builder.SetDestSize(terrain_consts::VERTICES_X,
                                terrain_consts::VERTICES_Z);
  heightmap_builder.EnableSeamless(true);

  noise::utils::RendererImage renderer{};
  noise::utils::Image         image;
  renderer.SetSourceNoiseMap(height_map);
  renderer.SetDestImage(image);

  for (int32_t i = 0; i < terrain_consts::NUM_TILES; ++i) {
    heightmap_builder.SetBounds(
      TILE_BOUNDS[i].x, TILE_BOUNDS[i].y, TILE_BOUNDS[i].z, TILE_BOUNDS[i].w);
    heightmap_builder.Build();

    renderer.ClearGradient();
    renderer.AddGradientPoint(-1.0000, utils::Color(0, 0, 128, 255)); // deeps
    renderer.AddGradientPoint(-0.2500, utils::Color(0, 0, 255, 255)); // shallow
    renderer.AddGradientPoint(0.0000, utils::Color(0, 128, 255, 255));  // shore
    renderer.AddGradientPoint(0.0625, utils::Color(240, 240, 64, 255)); // sand
    renderer.AddGradientPoint(0.1250, utils::Color(32, 160, 0, 255));   // grass
    renderer.AddGradientPoint(0.3750, utils::Color(224, 224, 0, 255));  // dirt
    renderer.AddGradientPoint(0.7500, utils::Color(128, 128, 128, 255)); // rock
    renderer.AddGradientPoint(1.0000, utils::Color(255, 255, 255, 255)); // snow

    renderer.EnableLight();
    renderer.SetLightContrast(3.0);   // Triple the contrast
    renderer.SetLightBrightness(2.0); // Double the brightness

    renderer.Render();

    //
    // Upload colormap
    gl::TextureSubImage3D(raw_handle(_colormap),
                          0,
                          0,
                          0,
                          i,
                          terrain_consts::VERTICES_X,
                          terrain_consts::VERTICES_Z,
                          1,
                          gl::RGBA,
                          gl::UNSIGNED_BYTE,
                          image.GetSlabPtr());

    //
    // Copy generated height vertices
    for (uint32_t z = 0; z < terrain_consts::VERTICES_Z; ++z) {
      auto slab_ptr = height_map.GetSlabPtr(z);

      for (uint32_t x = 0; x < terrain_consts::VERTICES_X; ++x) {
        heightmap_vertices.push_back(slab_ptr[x]);
      }
    }
  }

  //
  // Per instance heightmap buffer
  {
    gl::CreateBuffers(1, raw_handle_ptr(_per_instance_heightmap));
    gl::NamedBufferStorage(raw_handle(_per_instance_heightmap),
                           container_bytes_size(heightmap_vertices),
                           heightmap_vertices.data(),
                           0);
  }

  //
  // Per instance transform buffer
  {
    gl::CreateBuffers(1, raw_handle_ptr(_per_instance_transforms));
    gl::NamedBufferStorage(raw_handle(_per_instance_transforms),
                           sizeof(per_instance_transform) *
                             terrain_consts::NUM_TILES,
                           nullptr,
                           gl::MAP_WRITE_BIT);
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
  // gl::Enable(gl::CULL_FACE);

  _valid = true;
}

void app::terrain_demo::draw(const float surface_width,
                             const float surface_height) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);
  gl::ViewportIndexedf(0, 0.0f, 0.0f, surface_width, surface_height);

  //
  // Upload instance transforms
  {
    per_instance_transform instance_transforms[terrain_consts::NUM_TILES];

    for (int32_t i = 0; i < terrain_consts::NUM_TILES; ++i) {
      const auto world =
        R4::translate(static_cast<float>(i * (terrain_consts::TILE_DEPTH - 1)),
                      0.0f,
                      0.0f) *
        mat4f{R3::scale_y(_terrain_opts.scaling)};

      instance_transforms[i].world = transpose(world);

      instance_transforms[i].world_view_proj =
        transpose(_camera.projection_view() * world);

      instance_transforms[i].instance_vertex_offset =
        static_cast<uint32_t>(i * _mesh.vertex_count());
    }

    scoped_resource_mapping bmap{raw_handle(_per_instance_transforms),
                                 gl::MAP_WRITE_BIT |
                                   gl::MAP_INVALIDATE_BUFFER_BIT,
                                 sizeof(instance_transforms)};

    if (!bmap) {
      XR_LOG_ERR("Failed to map instance transform buffer for updating!");
      return;
    }

    memcpy(bmap.memory(), instance_transforms, sizeof(instance_transforms));
  }

  {
    gl::BindBufferBase(
      gl::SHADER_STORAGE_BUFFER, 0, raw_handle(_per_instance_heightmap));
    gl::BindBufferBase(
      gl::SHADER_STORAGE_BUFFER, 1, raw_handle(_per_instance_transforms));

    gl::BindTextureUnit(0, raw_handle(_colormap));
    gl::BindSampler(0, raw_handle(_sampler));

    gl::BindVertexArray(_mesh.vertex_array());

    _pipeline.use();

    if (_terrain_opts.wireframe) {
      scoped_polygon_mode_setting set_wireframe{gl::LINE};
      gl::DrawElementsInstanced(gl::TRIANGLES,
                                _mesh.index_count(),
                                gl::UNSIGNED_INT,
                                nullptr,
                                terrain_consts::NUM_TILES);
    } else {
      gl::DrawElementsInstanced(gl::TRIANGLES,
                                _mesh.index_count(),
                                gl::UNSIGNED_INT,
                                nullptr,
                                terrain_consts::NUM_TILES);
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

    ImGui::DragFloat("Scaling", &_terrain_opts.scaling, 0.25f, 1.0f, 64.0f);
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
