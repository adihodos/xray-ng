#include "misc/procedural_city/procedural_city_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
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
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

static constexpr uint32_t INSTANCE_COUNT{4u};

app::procedural_city_demo::procedural_city_demo() { init(); }

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

  _vs.set_uniform_block("Transforms", mat4f::stdc::identity);
  _pipeline.use();

  gl::DrawElementsInstanced(gl::TRIANGLES,
                            _mesh.index_count(),
                            gl::UNSIGNED_INT,
                            nullptr,
                            INSTANCE_COUNT);
}

void app::procedural_city_demo::update(const float delta_ms) {}

void app::procedural_city_demo::event_handler(
  const xray::ui::window_event& /* evt */) {}

void app::procedural_city_demo::compose_ui() {}

void app::procedural_city_demo::init() {
  assert(!valid());

  {
    geometry_data_t blob;
    geometry_factory::fullscreen_quad(&blob);
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

  struct per_instance_data {
    mat4f     tfworld;
    rgb_color color;
  } instances[INSTANCE_COUNT];

  instances[0].tfworld =
    R4::translate(-0.75f, -0.75f, 0.0f) * mat4f{R3::scale(0.25f)};
  instances[0].color = color_palette::web::red;

  instances[1].tfworld =
    R4::translate(-0.75f, +0.75f, 0.0f) * mat4f{R3::scale(0.25f)};
  instances[1].color = color_palette::web::green;

  instances[2].tfworld =
    R4::translate(+0.75f, +0.75f, 0.0f) * mat4f{R3::scale(0.25f)};
  instances[2].color = color_palette::web::blue;

  instances[3].tfworld =
    R4::translate(+0.75f, -0.75f, 0.0f) * mat4f{R3::scale(0.25f)};
  instances[3].color = color_palette::web::yellow;

  {
    gl::CreateBuffers(1, raw_handle_ptr(_instancedata));
    gl::NamedBufferStorage(
      raw_handle(_instancedata), sizeof(instances), instances, 0);
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

  _valid = true;
}
