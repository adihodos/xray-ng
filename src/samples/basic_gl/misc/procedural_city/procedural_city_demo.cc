#include "misc/procedural_city/procedural_city_demo.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <imgui/imgui.h>
#include <iterator>
#include <opengl/opengl.hpp>
#include <random>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace xray::scene;
using namespace std;

extern xray::base::app_config* xr_app_config;

static constexpr uint32_t          INSTANCE_COUNT{1024u};
static constexpr const char* const TEXTURES[] = {"uv_grids/ash_uvgrid01.jpg",
                                                 "uv_grids/ash_uvgrid02.jpg",
                                                 "uv_grids/ash_uvgrid03.jpg",
                                                 "uv_grids/ash_uvgrid04.jpg",
                                                 "uv_grids/ash_uvgrid05.jpg",
                                                 "uv_grids/ash_uvgrid06.jpg",
                                                 "uv_grids/ash_uvgrid07.jpg",
                                                 "uv_grids/ash_uvgrid08.jpg"};

class random_engine {
public:
  random_engine(const float rangemin = 0.0f, const float rangemax = 1.0f)
    : _distribution{rangemin, rangemax} {}

  void set_range(const float minval, const float maxval) noexcept {
    _distribution = std::uniform_real_distribution<float>{minval, maxval};
  }

  float next() noexcept { return _distribution(_engine); }

private:
  std::random_device                    _device{};
  std::mt19937                          _engine{_device()};
  std::uniform_real_distribution<float> _distribution{0.0f, 1.0f};
};

app::procedural_city_demo::procedural_city_demo(
  const app::init_context_t& initctx) {

  //_camera.look_at(
  //  vec3f{0.0f, 10.0f, -10.0f}, vec3f::stdc::zero, vec3f::stdc::unit_y);
  //_camera.set_projection(
  //  projection_rh::perspective_symmetric((float) initctx.surface_width,
  //                                       (float) initctx.surface_height,
  //                                       radians(60.0f),
  //                                       0.1f,
  //                                       1000.0f));
  camera_lens_parameters lp{};
  lp.farplane = 1000.0f;
  lp.width    = (float) initctx.surface_width;
  lp.height   = (float) initctx.surface_height;

  _camcontrol.set_lens_parameters(lp);
  init();
}

app::procedural_city_demo::~procedural_city_demo() {}

void app::procedural_city_demo::draw(
  const xray::rendering::draw_context_t& draw_ctx) {

  gl::ViewportIndexedf(0,
                       0.0f,
                       0.0f,
                       (float) draw_ctx.window_width,
                       (float) draw_ctx.window_height);

  const float CLEAR_COLOR[] = {0.0f, 0.0f, 0.0f, 1.0f};

  gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, CLEAR_COLOR);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  gl::BindVertexArray(_mesh.vertex_array());
  gl::BindBufferBase(gl::SHADER_STORAGE_BUFFER, 0, raw_handle(_instancedata));

  _vs.set_uniform_block("Transforms", _camera.projection_view());
  _pipeline.use();

  gl::BindTextureUnit(0, raw_handle(_objtex));
  gl::BindSampler(0, raw_handle(_sampler));

  gl::DrawElementsInstanced(gl::TRIANGLES,
                            _mesh.index_count(),
                            gl::UNSIGNED_INT,
                            nullptr,
                            INSTANCE_COUNT);
}

void app::procedural_city_demo::update(const float delta_ms) {
  _camcontrol.update();
}

void app::procedural_city_demo::event_handler(
  const xray::ui::window_event& evt) {

  if (evt.type == event_type::configure) {
    camera_lens_parameters lp;
    lp.farplane = 1000.0f;
    lp.width    = (float) evt.event.configure.width;
    lp.height   = (float) evt.event.configure.height;
    _camcontrol.set_lens_parameters(lp);
    return;
  }

  if (is_input_event(evt)) {
    _camcontrol.input_event(evt);
  }
}

void app::procedural_city_demo::compose_ui() {}

struct per_instance_data {
  mat4f     tfworld;
  rgb_color color;
  int32_t   texid;
  int32_t   pad[3];
};

void app::procedural_city_demo::init() {
  assert(!valid());

  {
    geometry_data_t blob;
    geometry_factory::box(1.0f, 1.0f, 1.0f, &blob);
    vector<vertex_pnt> vertices;
    transform(raw_ptr(blob.geometry),
              raw_ptr(blob.geometry) + blob.vertex_count,
              back_inserter(vertices),
              [](const vertex_pntt& vsin) {
                return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
              });

    _mesh = basic_mesh{vertices.data(),
                       vertices.size(),
                       raw_ptr(blob.indices),
                       blob.index_count};
  }

  {
    random_engine             re{};
    vector<per_instance_data> instances{INSTANCE_COUNT};

    int32_t invokeid{};
    generate_n(begin(instances), INSTANCE_COUNT, [&invokeid, &re]() {

      const auto xpos = floor(re.next() * 200.0f - 100.0f) * 10.0f;
      const auto zpos = floor(re.next() * 200.0f - 100.0f) * 10.0f;
      const auto yrot = re.next() * two_pi<float>;
      const auto sx   = re.next() * 50.0f + 10.0f;
      const auto sy   = re.next() * sx * 8.0f + 8.0f;
      const auto sz   = sx;

      per_instance_data data;
      data.tfworld = R4::translate(xpos, 0.0f, zpos) *
                     mat4f{R3::rotate_y(yrot) * R3::scale_xyz(sx, sy, sz)} *
                     R4::translate(0.0f, 0.5f, 0.0f);
      data.color = rgb_color{1.0f - re.next()};
      data.texid = invokeid % XR_U32_COUNTOF__(TEXTURES);
      ++invokeid;

      return data;
    });

    gl::CreateBuffers(1, raw_handle_ptr(_instancedata));
    gl::NamedBufferStorage(raw_handle(_instancedata),
                           container_bytes_size(instances),
                           instances.data(),
                           0);
  }

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/procedural_city/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs) {
    return;
  }

  _fs = gpu_program_builder{}
          .add_file("shaders/misc/procedural_city/fs.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs) {
    return;
  }

  _pipeline = program_pipeline{[]() {
    GLuint pp;
    gl::CreateProgramPipelines(1, &pp);
    return pp;
  }()};

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

  {
    gl::CreateTextures(gl::TEXTURE_2D_ARRAY, 1, raw_handle_ptr(_objtex));
    gl::TextureStorage3D(raw_handle(_objtex),
                         1,
                         gl::RGBA8,
                         1024,
                         1024,
                         XR_U32_COUNTOF__(TEXTURES));

    uint32_t id{};
    for_each(begin(TEXTURES),
             end(TEXTURES),
             [&id, texid = raw_handle(_objtex) ](const char* texfile) {
               texture_loader tex_ldr{
                 xr_app_config->texture_path(texfile).c_str()};
               if (!tex_ldr) {
                 return;
               }

               gl::TextureSubImage3D(texid,
                                     0,
                                     0,
                                     0,
                                     id++,
                                     tex_ldr.width(),
                                     tex_ldr.height(),
                                     1,
                                     tex_ldr.format(),
                                     gl::UNSIGNED_BYTE,
                                     tex_ldr.data());
             });
  }

  {
    gl::CreateSamplers(1, raw_handle_ptr(_sampler));
    gl::SamplerParameteri(
      raw_handle(_sampler), gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(
      raw_handle(_sampler), gl::TEXTURE_MAG_FILTER, gl::LINEAR);
  }

  gl::Enable(gl::DEPTH_TEST);
  gl::Enable(gl::CULL_FACE);

  _valid = true;
}