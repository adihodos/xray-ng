#include "geometric_shapes_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include <algorithm>
#include <gsl.h>
#include <opengl/opengl.hpp>
#include <vector>

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

  {
    geometry_data_t sphere;
    geometry_factory::geosphere(16.0f, 32u, &sphere);

    vector<vertex_pnc> sphere_vertices{};
    transform(raw_ptr(sphere.geometry),
              raw_ptr(sphere.geometry) + sphere.vertex_count,
              back_inserter(sphere_vertices),
              [](const vertex_pntt& vx) {
                vertex_pnc vsout;
                vsout.pos    = vx.position;
                vsout.normal = vx.normal;
                vsout.color  = color_palette::flat::orange500;

                return vsout;
              });

    gl::CreateBuffers(1, raw_handle_ptr(_render.vertices_sphere));
    gl::NamedBufferStorage(raw_handle(_render.vertices_sphere),
                           container_bytes_size(sphere_vertices),
                           sphere_vertices.data(),
                           0);

    gl::CreateBuffers(1, raw_handle_ptr(_render.indices_sphere));
    gl::NamedBufferStorage(raw_handle(_render.indices_sphere),
                           sphere.index_count * sizeof(uint32_t),
                           raw_ptr(sphere.indices),
                           0);

    _render.index_count_sphere = sphere.index_count;
  }

  //
  // Floor plane
  {
    geometry_data_t grid;
    geometry_factory::grid(1024.0f, 1024.0f, 1023, 1023, &grid);

    _render.index_count = grid.index_count;

    vector<vertex_pnc> vertices;
    vertices.reserve(1024 * 1024);

    for (size_t y = 0; y < 1024; ++y) {
      for (size_t x = 0; x < 1024; ++x) {
        //      bool color = (y % 16 == 0) || (y % 17 == 0) || (y % 18 == 0) ||
        //                   (x % 16 == 0) || (x % 17 == 0) || (x % 18 == 0);
        bool color = (y % 16 == 0) || (x % 16 == 0);

        const auto idx = y * 1024 + x;

        vertex_pnc v;
        v.pos    = grid.geometry[idx].position;
        v.normal = grid.geometry[idx].normal;
        v.color =
          color ? color_palette::web::black : color_palette::flat::silver500;

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
  }

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

  _render.vs = gpu_program_builder{}
                 .add_file("shaders/misc/geometric_shapes/vs.glsl")
                 .build<render_stage::e::vertex>();

  _render.fs = gpu_program_builder{}
                 .add_file("shaders/misc/geometric_shapes/fs.glsl")
                 .build<render_stage::e::fragment>();

  if (!_render.vs || !_render.fs) {
    return;
  }

  _render.pipeline = program_pipeline{[]() {
    GLuint pp = {};
    gl::CreateProgramPipelines(1, &pp);
    return pp;
  }()};

  _render.pipeline.use_vertex_program(_render.vs)
    .use_fragment_program(_render.fs);

  _scene.camera.set_projection(projection::perspective_symmetric(
    static_cast<float>(init_ctx->surface_width),
    static_cast<float>(init_ctx->surface_height),
    radians(70.0f),
    0.1f,
    1000.0f));
  _scene.cam_control.update();

  gl::Enable(gl::DEPTH_TEST);
  gl::Enable(gl::CULL_FACE);

  _valid = true;
}

app::geometric_shapes_demo::~geometric_shapes_demo() {}

void app::geometric_shapes_demo::draw(const xray::rendering::draw_context_t&) {
  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::white.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  _render.vs.set_uniform_block("Transforms",
                               _scene.camera.projection_view().components);
  _render.pipeline.use();

  gl::VertexArrayVertexBuffer(raw_handle(_render.vertexarray),
                              0,
                              raw_handle(_render.vertices),
                              0,
                              sizeof(vertex_pnc));
  gl::VertexArrayElementBuffer(raw_handle(_render.vertexarray),
                               raw_handle(_render.indices));

  gl::BindVertexArray(raw_handle(_render.vertexarray));
  gl::DrawElements(
    gl::TRIANGLES, _render.index_count, gl::UNSIGNED_INT, nullptr);

  gl::VertexArrayVertexBuffer(raw_handle(_render.vertexarray),
                              0,
                              raw_handle(_render.vertices_sphere),
                              0,
                              sizeof(vertex_pnc));
  gl::VertexArrayElementBuffer(raw_handle(_render.vertexarray),
                               raw_handle(_render.indices_sphere));

  scoped_winding_order_setting set_cw{gl::CW};
  gl::DrawElements(
    gl::TRIANGLES, _render.index_count_sphere, gl::UNSIGNED_INT, nullptr);
}

void app::geometric_shapes_demo::update(const float delta_ms) {}

void app::geometric_shapes_demo::event_handler(
  const xray::ui::window_event& evt) {

  if (evt.type == event_type::configure) {
    _scene.camera.set_projection(projection::perspective_symmetric(
      static_cast<float>(evt.event.configure.width),
      static_cast<float>(evt.event.configure.height),
      radians(70.0f),
      0.1f,
      1000.0f));

    return;
  }

  if (is_input_event(evt)) {
    _scene.cam_control.input_event(evt);
    _scene.cam_control.update();
    return;
  }
}

void app::geometric_shapes_demo::compose_ui() {}
