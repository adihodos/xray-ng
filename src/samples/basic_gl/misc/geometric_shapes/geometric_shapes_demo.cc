#include "geometric_shapes_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/debug_output.hpp"
#include "xray/base/random.hpp"
#include "xray/math/objects/plane.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/scalar4x4_string_cast.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/physics/particle.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/mesh_loader.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include <algorithm>
#include <gsl.h>
#include <opengl/opengl.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::physics;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

static const rgb_color INSTANCE_COLORS[] = {
  color_palette::flat::alizarin300,   color_palette::flat::amethyst100,
  color_palette::flat::asbestos800,   color_palette::flat::belizehole400,
  color_palette::flat::carrot300,     color_palette::flat::clouds500,
  color_palette::flat::concrete900,   color_palette::flat::emerald400,
  color_palette::flat::greensea200,   color_palette::flat::midnightblue200,
  color_palette::flat::nephritis400,  color_palette::flat::orange400,
  color_palette::flat::peterriver100, color_palette::flat::pomegranate400,
  color_palette::flat::pumpkin600,    color_palette::flat::silver800,
  color_palette::flat::sunflower200,  color_palette::flat::turquoise300,
  color_palette::flat::wetasphalt500, color_palette::flat::wisteria900};

struct vertex_pnc {
  vec3f     pos;
  vec3f     normal;
  rgb_color color;
};

app::geometric_shapes_demo::geometric_shapes_demo(
  const init_context_t* init_ctx) {

  {
    _scene.lights[0].position = vec3f{0.0f, -1.0f, 0.0f};
    _scene.lights[0].ka       = color_palette::web::black;
    _scene.lights[0].kd       = color_palette::material::yellow100;

    _scene.lights[1].position = vec3f{0.0f, 0.0f, 1.0f};
    _scene.lights[1].ka       = color_palette::web::black;
    _scene.lights[1].kd       = color_palette::material::yellow100;

    _scene.lights[2].position = vec3f{-1.0f, 0.0f, 0.0f};
    _scene.lights[2].ka       = color_palette::web::black;
    _scene.lights[2].kd       = color_palette::material::yellow100;

    _scene.lights[3].position = vec3f{+1.0f, 0.0f, 0.0f};
    _scene.lights[3].ka       = color_palette::web::black;
    _scene.lights[3].kd       = color_palette::material::yellow100;
  }

  {
    geometry_data_t sphere;
    geometry_factory::geosphere(4.0f, 32u, &sphere);

    vector<vertex_pnc> sphere_vertices{};
    transform(raw_ptr(sphere.geometry),
              raw_ptr(sphere.geometry) + sphere.vertex_count,
              back_inserter(sphere_vertices),
              [](const vertex_pntt& vx) {
                vertex_pnc vsout;
                vsout.pos    = vx.position;
                vsout.normal = vx.normal;
                vsout.color  = color_palette::web::white_smoke;

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

  {
    const auto w = 16.0f;
    const auto d = 16.0f;
    float      x = -64.0f;
    float      z = 64.0f;

    random_number_generator rnd{};

    for (;;) {
      if (x > 64.0f) {
        x = -64.0f;
        z -= d;
      }

      if (z < -64.0f) {
        break;
      }

      particle_graphics pg;
      pg.starting_pos = vec3f{x, rnd.next_float(100.0f, 150.0f), z};
      pg.color =
        INSTANCE_COLORS[rnd.next_uint(0, XR_U32_COUNTOF(INSTANCE_COLORS))];
      _obj_instances.pgraphics.push_back(pg);

      particle p;
      p.position = pg.starting_pos;
      p.radius   = 4.0f;
      //      p.mass = r
      _obj_instances.instances.push_back(p);

      x += w;
    }

    gl::CreateBuffers(1, raw_handle_ptr(_render.instances_ssbo));
    gl::NamedBufferStorage(raw_handle(_render.instances_ssbo),
                           _obj_instances.instances.size() *
                             sizeof(gpu_instance_info),
                           nullptr,
                           gl::MAP_WRITE_BIT);
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
                 .add_string("#version 450 core\n")
                 .add_string("#define NO_INSTANCING\n")
                 .add_file("shaders/misc/geometric_shapes/vs.glsl")
                 .build<render_stage::e::vertex>();

  _render.vs_instancing = gpu_program_builder{}
                            .add_string("#version 450 core\n")
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

  _scene.camera.set_projection(projections_rh::perspective_symmetric(
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
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  //
  // update instance data buffer
  {
    scoped_resource_mapping instbuff{
      raw_handle(_render.instances_ssbo),
      gl::MAP_WRITE_BIT | gl::MAP_INVALIDATE_BUFFER_BIT,
      static_cast<uint32_t>(_obj_instances.instances.size() *
                            sizeof(gpu_instance_info))};

    if (!instbuff) {
      XR_LOG_ERR("Failed to map instances buffer for update");
      return;
    }

    auto gpu_inst_span = gsl::span<gpu_instance_info>{
      static_cast<gpu_instance_info*>(instbuff.memory()),
      static_cast<gpu_instance_info*>(instbuff.memory()) +
        (ptrdiff_t) _obj_instances.instances.size()};

    for (size_t i = 0, cnt = _obj_instances.instances.size(); i < cnt; ++i) {
      const auto world_tf = R4::translate(_obj_instances.instances[i].position);

      gpu_inst_span[i].world_view_proj =
        transpose(_scene.camera.projection_view() * world_tf);
      gpu_inst_span[i].tex_id  = 0;
      gpu_inst_span[i].view    = transpose(_scene.camera.view() * world_tf);
      gpu_inst_span[i].normals = transpose(_scene.camera.view() * world_tf);
      gpu_inst_span[i].color   = _obj_instances.pgraphics[i].color;
    }
  }

  gl::BindVertexArray(raw_handle(_render.vertexarray));
  gl::VertexArrayVertexBuffer(raw_handle(_render.vertexarray),
                              0,
                              raw_handle(_render.vertices_sphere),
                              0,
                              sizeof(vertex_pnc));
  gl::VertexArrayElementBuffer(raw_handle(_render.vertexarray),
                               raw_handle(_render.indices_sphere));

  gl::BindBufferBase(
    gl::SHADER_STORAGE_BUFFER, 0, raw_handle(_render.instances_ssbo));

  light_source ls_view[XR_COUNTOF(_scene.lights)];
  transform(begin(_scene.lights),
            end(_scene.lights),
            begin(ls_view),
            [s = &_scene](const light_source& in_light) {
              light_source out_light{in_light};
              out_light.position =
                normalize(mul_vec(s->camera.view(), in_light.position));
              return out_light;
            });

  _render.fs.set_uniform_block("light_setup", ls_view);
  _render.pipeline.use_vertex_program(_render.vs_instancing);
  _render.pipeline.use();

  {
    scoped_winding_order_setting set_cw{gl::CW};
    gl::DrawElementsInstanced(gl::TRIANGLES,
                              _render.index_count_sphere,
                              gl::UNSIGNED_INT,
                              nullptr,
                              _obj_instances.instances.size());
  }

  gl::VertexArrayVertexBuffer(raw_handle(_render.vertexarray),
                              0,
                              raw_handle(_render.vertices),
                              0,
                              sizeof(vertex_pnc));
  gl::VertexArrayElementBuffer(raw_handle(_render.vertexarray),
                               raw_handle(_render.indices));

  _render.pipeline.use_vertex_program(_render.vs);

  struct {
    mat4f world_view_proj;
    mat4f world_view;
    mat4f normals;
  } const transform_matrices{_scene.camera.projection_view(),
                             _scene.camera.view(),
                             _scene.camera.view()};

  _render.vs.set_uniform_block("Transforms", transform_matrices);
  _render.pipeline.use();

  {
    scoped_winding_order_setting set_cw{gl::CW};
    gl::DrawElements(
      gl::TRIANGLES, _render.index_count, gl::UNSIGNED_INT, nullptr);
  }
}

void app::geometric_shapes_demo::update(const float delta_ms) {
  const auto wind_direction = normalize(vec3f{-1.0f, -1.0f, -1.0f});

  uint32_t idx{};
  for_each(begin(_obj_instances.instances),
           end(_obj_instances.instances),
           [inst = &_obj_instances, &wind_direction, &idx](particle& p) {

             p.compute_loads(object_instances::drag_coefficient,
                             wind_direction,
                             object_instances::wind_speed);

             p.update_body_euler(object_instances::time_step);

             if ((p.position.y - 4.0f) <= 0.0f) {
               p.position.y = 150.0f;
             }

             //             const auto is_out_of_bounds =
             //               (p.position.x < -512.0f) || (p.position.x >
             //               512.0f) || (p.position.z < -512.0f) ||
             //               (p.position.z > 512.0f) ||
             //               ((p.position.y - p.radius) <= 0.0f);

             //             if (is_out_of_bounds) {
             //               p.position = inst->pgraphics[idx++].starting_pos;
             //             }
           });
}

void app::geometric_shapes_demo::event_handler(
  const xray::ui::window_event& evt) {

  if (evt.type == event_type::configure) {
    _scene.camera.set_projection(projections_rh::perspective_symmetric(
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
