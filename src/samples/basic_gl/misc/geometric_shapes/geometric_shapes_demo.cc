#include "geometric_shapes_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/random.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
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
    //    geometry_data_t sphere;
    //    geometry_factory::geosphere(16.0f, 32u, &sphere);

    //    vector<vertex_pnc> sphere_vertices{};
    //    transform(raw_ptr(sphere.geometry),
    //              raw_ptr(sphere.geometry) + sphere.vertex_count,
    //              back_inserter(sphere_vertices),
    //              [](const vertex_pntt& vx) {
    //                vertex_pnc vsout;
    //                vsout.pos    = vx.position;
    //                vsout.normal = vx.normal;
    //                vsout.color  = color_palette::web::white_smoke;

    //                return vsout;
    //              });

    //    gl::CreateBuffers(1, raw_handle_ptr(_render.vertices_sphere));
    //    gl::NamedBufferStorage(raw_handle(_render.vertices_sphere),
    //                           container_bytes_size(sphere_vertices),
    //                           sphere_vertices.data(),
    //                           0);

    //    gl::CreateBuffers(1, raw_handle_ptr(_render.indices_sphere));
    //    gl::NamedBufferStorage(raw_handle(_render.indices_sphere),
    //                           sphere.index_count * sizeof(uint32_t),
    //                           raw_ptr(sphere.indices),
    //                           0);

    //    _render.index_count_sphere = sphere.index_count;
  }

  {
    const char* model_file =
      //"leo2/leo2a6.bin";
      //      "typhoon/typhoon.bin";
      //      "a4/a4.bin";
      "a10/a10.bin";

    mesh_loader mloader{init_ctx->cfg->model_path(model_file).c_str()};
    if (!mloader) {
      return;
    }

    XR_LOG_INFO("Mesh bbox {} <-> {}, sphere {} : {}",
                string_cast(mloader.bounding().axis_aligned_bbox.min),
                string_cast(mloader.bounding().axis_aligned_bbox.max),
                string_cast(mloader.bounding().bounding_sphere.center),
                mloader.bounding().bounding_sphere.radius);

    {
      const auto w = mloader.bounding().axis_aligned_bbox.width();
      const auto d = mloader.bounding().axis_aligned_bbox.depth();
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

        instance_info ii;
        ii.position = vec3f{x, 10.0f, z};
        ii.pitch    = rnd.next_float(0.0f, two_pi<float>);
        ii.roll     = rnd.next_float(0.0f, two_pi<float>);
        ii.yaw      = rnd.next_float(0.0f, two_pi<float>);
        _obj_instances.instances.push_back(ii);

        x += w;
      }

      gl::CreateBuffers(1, raw_handle_ptr(_render.instances_ssbo));
      gl::NamedBufferStorage(raw_handle(_render.instances_ssbo),
                             _obj_instances.instances.size() *
                               sizeof(gpu_instance_info),
                             nullptr,
                             gl::MAP_WRITE_BIT);
    }

    vector<vertex_pnc> vertices;
    const auto         vspan = mloader.vertex_span();
    transform(begin(vspan),
              end(vspan),
              back_inserter(vertices),
              [](const vertex_pnt& v) {
                return vertex_pnc{
                  v.position, v.normal, color_palette::material::grey200};
              });

    gl::CreateBuffers(1, raw_handle_ptr(_render.vertices_sphere));
    gl::NamedBufferStorage(raw_handle(_render.vertices_sphere),
                           container_bytes_size(vertices),
                           vertices.data(),
                           0);

    gl::CreateBuffers(1, raw_handle_ptr(_render.indices_sphere));
    gl::NamedBufferStorage(raw_handle(_render.indices_sphere),
                           mloader.index_bytes(),
                           mloader.index_data(),
                           0);

    _render.index_count_sphere = mloader.header().index_count;
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
      _obj_instances.instances.size() * sizeof(gpu_instance_info)};

    if (!instbuff) {
      XR_LOG_ERR("Failed to map instances buffer for update");
      return;
    }

    auto gpu_inst_span = gsl::span<gpu_instance_info>{
      static_cast<gpu_instance_info*>(instbuff.memory()),
      static_cast<gpu_instance_info*>(instbuff.memory()) +
        (ptrdiff_t) _obj_instances.instances.size()};

    uint32_t instance_id{};
    transform(begin(_obj_instances.instances),
              end(_obj_instances.instances),
              begin(gpu_inst_span),
              [ s = &_scene, &instance_id ](const instance_info& ii) {

                const auto world_tf =
                  R4::translate(ii.position) * mat4f{R3::rotate_y(ii.yaw)} *
                  mat4f{R3::rotate_x(ii.pitch)} * mat4f{R3::rotate_z(ii.roll)};

                gpu_instance_info gi;

                gi.world_view_proj =
                  transpose(s->camera.projection_view() * world_tf);
                gi.tex_id  = 0;
                gi.view    = transpose(s->camera.view() * world_tf);
                gi.normals = transpose(s->camera.view() * world_tf);
                gi.color =
                  INSTANCE_COLORS[instance_id % XR_COUNTOF(INSTANCE_COLORS)];

                ++instance_id;

                return gi;
              });
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

  gl::DrawElements(
    gl::TRIANGLES, _render.index_count, gl::UNSIGNED_INT, nullptr);
}

void app::geometric_shapes_demo::update(const float delta_ms) {
  for_each(begin(_obj_instances.instances),
           end(_obj_instances.instances),
           [](instance_info& ii) {
             static constexpr auto ROTATION_SPEED = 0.1f;

             ii.roll += ROTATION_SPEED;
             ii.pitch += ROTATION_SPEED;
             ii.yaw += ROTATION_SPEED;

             if (ii.roll > two_pi<float>) {
               ii.roll -= two_pi<float>;
             }

             if (ii.yaw > two_pi<float>) {
               ii.yaw -= two_pi<float>;
             }

             if (ii.pitch > two_pi<float>) {
               ii.pitch -= two_pi<float>;
             }
           });
}

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
