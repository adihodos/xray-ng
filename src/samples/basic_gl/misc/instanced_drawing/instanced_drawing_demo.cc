#include "misc/instanced_drawing/instanced_drawing_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <imgui/imgui.h>
#include <iterator>
#include <opengl/opengl.hpp>
#include <span.h>
#include <stlsoft/memory/auto_buffer.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::instanced_drawing_demo::instanced_drawing_demo() {
  _mesh = basic_mesh{xr_app_config->model_path("a4/a4.obj").c_str()};

  if (!_mesh) {
    XR_LOG_ERR("Failed to load model!");
    return;
  }

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/instanced_draw/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs) {
    return;
  }

  //  _gs = gpu_program_builder{}
  //          .add_file("shaders/misc/instanced_draw/gs.glsl")
  //          .build<render_stage::e::geometry>();

  //  if (!_gs) {
  //    return;
  //  }

  _fs = gpu_program_builder{}
          .add_file("shaders/misc/instanced_draw/fs.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs) {
    return;
  }

  _pipeline = program_pipeline{[]() {
    GLuint pp{};
    gl::CreateProgramPipelines(1, &pp);
    return pp;
  }()};

  _pipeline
    .use_vertex_program(_vs)
    //    .use_geometry_program(_gs)
    .use_fragment_program(_fs);

  generate_n(back_inserter(_obj_instances.instances),
             object_instances::instance_count,
             [r = &_rand]() {
               instance_info new_inst;
               new_inst.pitch      = r->next_float(0.0f, two_pi<float>);
               new_inst.roll       = r->next_float(0.0f, two_pi<float>);
               new_inst.yaw        = r->next_float(0.0f, two_pi<float>);
               new_inst.scale      = r->next_float(1.0f, 4.0f);
               new_inst.texture_id = r->next_uint(0, 8);
               new_inst.position   = vec3f{r->next_float(-512.0f, +512.0f),
                                         r->next_float(-512.0f, +512.0f),
                                         r->next_float(-512.0f, +512.0f)};

               return new_inst;
             });

  _obj_instances.buffer_transforms = [objs = &_obj_instances]() {
    GLuint buff{};
    gl::CreateBuffers(1, &buff);
    gl::NamedBufferStorage(buff,
                           sizeof(mat4f) * object_instances::instance_count,
                           nullptr,
                           gl::MAP_WRITE_BIT);

    return scoped_buffer{buff};
  }
  ();

  _obj_instances.buffer_texture_ids = [objs = &_obj_instances]() {
    GLuint buff{};
    gl::CreateBuffers(1, &buff);
    stlsoft::auto_buffer<uint32_t, 32> texid_buff{
      object_instances::instance_count};

    for (uint32_t i = 0; i < object_instances::instance_count; ++i) {
      texid_buff.data()[i] = objs->instances[i].texture_id;
    }

    gl::NamedBufferStorage(
      buff, texid_buff.size() * sizeof(uint32_t), texid_buff.data(), 0);

    return scoped_buffer{buff};
  }
  ();

  instance_descriptor id[2];
  id[0].buffer_handle    = raw_handle(_obj_instances.buffer_transforms);
  id[0].buffer_offset    = 0;
  id[0].element_stride   = sizeof(mat4f);
  id[0].instance_divisor = 1;

  id[1].buffer_handle    = raw_handle(_obj_instances.buffer_texture_ids);
  id[1].element_stride   = sizeof(uint32_t);
  id[1].buffer_offset    = 0;
  id[1].instance_divisor = 1;

  vertex_attribute_descriptor vds[2];
  vds[0].component_count  = 16;
  vds[0].component_index  = 0;
  vds[0].component_offset = 0;
  vds[0].component_type   = gl::FLOAT;
  vds[0].instance_desc    = &id[0];

  vds[1].component_count  = 1;
  vds[1].component_index  = 1;
  vds[1].component_offset = 0;
  vds[1].component_type   = gl::UNSIGNED_INT;
  vds[1].instance_desc    = &id[1];

  _mesh.set_instance_data(id, 2, vds, 2);

  _valid = true;
}

app::instanced_drawing_demo::~instanced_drawing_demo() {}

void app::instanced_drawing_demo::draw(
  const xray::rendering::draw_context_t& ctx) {

  {
    vector<mat4f> instance_transforms;

    transform(begin(_obj_instances.instances),
              end(_obj_instances.instances),
              back_inserter(instance_transforms),
              [ctx](instance_info& ii) {
                return R4::translate(ii.position) * mat4f{R3::scale(ii.scale)};
              });

    scoped_resource_mapping buffermap{
      raw_handle(_obj_instances.buffer_transforms),
      gl::MAP_WRITE_BIT,
      container_bytes_size(instance_transforms)};

    if (!buffermap) {
      XR_LOG_ERR("Failed to map instance transform buffer!");
      return;
    }
  }

  const float CLEAR_COLOR[] = {0.0f, 0.0f, 0.0f, 1.0f};
  gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, CLEAR_COLOR);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  gl::BindVertexArray(_mesh.vertex_array());
  _pipeline.use();
  gl::DrawElementsInstanced(gl::TRIANGLES,
                            _mesh.index_count(),
                            gl::UNSIGNED_INT,
                            nullptr,
                            object_instances::instance_count);
}

void app::instanced_drawing_demo::update(const float /*delta_ms*/) {}

void app::instanced_drawing_demo::event_handler(
  const xray::ui::window_event& evt) {}

void app::instanced_drawing_demo::compose_ui() {}
