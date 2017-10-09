#include "geometric_shapes_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include <algorithm>
#include <gsl.h>
#include <opengl/opengl.hpp>
#include <vector>
#include <xray/math/scalar3.hpp>
#include <xray/rendering/colors/color_palettes.hpp>
#include <xray/rendering/vertex_format/vertex_pc.hpp>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

struct vertex_pnc {
  vec3f     pos;
  vec3f     normal;
  rgb_color color;
};

app::geometric_shapes_demo::geometric_shapes_demo(
  const init_context_t* init_ctx) {

  geometry_data_t grid;
  geometry_factory::grid(1024.0f, 1024.0f, 1023, 1023, &grid);

  vector<vertex_pnc> vertices;
  vertices.reserve(1024 * 1024);

  for (size_t y = 0; y < 1024; ++y) {
    for (size_t x = 0; x < 1024; ++x) {
      bool color = (y % 16 == 0) || (y % 17 == 0) || (y % 18 == 0) ||
                   (x % 16 == 0) || (x % 17 == 0) || (x % 18 == 0);

      const auto idx = y * 1024 + x;

      vertex_pnc v;
      v.pos    = grid.geometry[idx].position;
      v.normal = grid.geometry[idx].normal;
      v.color =
        color ? color_palette::web::black : color_palette::web::white_smoke;

      vertices.push_back(v);
    }
  }

  gl::CreateBuffers(1, raw_handle_ptr(_render.vertices));
  gl::NamedBufferStorage(raw_handle(_render.vertices),
                         container_bytes_size(vertices),
                         vertices.data(),
                         0);

  gl::CreateBuffers(1, raw_handle_ptr(_render.indices));
  gl::NamedBufferStorage(raw_handle(_render.indices),
                         grid.index_count * sizeof(uint32_t),
                         raw_ptr(grid.indices),
                         0);

  gl::CreateVertexArrays(1, raw_handle_ptr(_render.vertexarray));
  const auto va = raw_handle(_render.vertexarray);
  gl::VertexArrayVertexBuffer(
    va, 0, raw_handle(_render.vertices), 0, sizeof(vertex_pnc));
  gl::VertexArrayElementBuffer(va, raw_handle(_render.indices));
  gl::VertexArrayAttribFormat(
    va, 0, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnc, pos));
  gl::VertexArrayAttribFormat(
    va, 1, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnc, normal));
  gl::VertexArrayAttribFormat(
    va, 2, 4, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnc, color));
  gl::VertexArrayAttribBinding(va, 0, 0);
  gl::VertexArrayAttribBinding(va, 1, 0);
  gl::VertexArrayAttribBinding(va, 2, 0);
  gl::EnableVertexArrayAttrib(va, 0);
  gl::EnableVertexArrayAttrib(va, 1);
  gl::EnableVertexArrayAttrib(va, 2);
}

app::geometric_shapes_demo::~geometric_shapes_demo() {}

void app::geometric_shapes_demo::draw(const xray::rendering::draw_context_t&) {}

void app::geometric_shapes_demo::update(const float delta_ms) {}

void app::geometric_shapes_demo::event_handler(
  const xray::ui::window_event& evt) {}

void app::geometric_shapes_demo::compose_ui() {}
