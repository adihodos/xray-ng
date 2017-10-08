#include "misc/instanced_drawing/instanced_drawing_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/math/objects/sphere_math.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/opengl/gl_misc.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/opengl/scoped_state.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <imgui/imgui.h>
#include <iterator>
#include <opengl/opengl.hpp>
#include <platformstl/filesystem/readdir_sequence.hpp>
#include <span.h>
#include <stlsoft/memory/auto_buffer.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

struct DrawElementsIndirectCommand {
  uint count;
  uint instanceCount;
  uint firstIndex;
  uint baseVertex;
  uint baseInstance;
};

template <typename InputIterator, typename OutputIterator, typename Accumulator>
void exclusive_scan(InputIterator  first,
                    InputIterator  last,
                    OutputIterator out,
                    Accumulator    accum) {

  *out = decltype(*first){};

  while (first != last) {
    auto v = accum(*out++, *first++);
    *out   = v;
  }
}

app::instanced_drawing_demo::instanced_drawing_demo(
  const app::init_context_t* init_ctx) {

  //
  // turn off these so we don't get spammed
  gl::DebugMessageControl(gl::DONT_CARE,
                          gl::DONT_CARE,
                          gl::DEBUG_SEVERITY_NOTIFICATION,
                          0,
                          nullptr,
                          gl::FALSE_);

  _scene.cam_control.update();
  _scene.camera.set_projection(projection::perspective_symmetric(
    static_cast<float>(init_ctx->surface_width),
    static_cast<float>(init_ctx->surface_height),
    radians(70.0f),
    0.1f,
    1000.0f));

  struct {
    vector<vertex_pnt> vertices;
    vector<uint32_t>   indices;
  } geometries[2];

  static constexpr auto MODEL_FILE    = "f15/f15c.obj";
  static constexpr auto MODEL_FILE_F4 = "f4/f4phantom.obj";
  {

    const auto path = xr_app_config->model_path(MODEL_FILE);

    if (!basic_mesh::load_mesh(path.c_str(),
                               &geometries[0].vertices,
                               &geometries[0].indices,
                               mesh_load_option::remove_points_lines)) {
      XR_LOG_ERR("Failed to load model {}", path.c_str());
      return;
    }
  }

  {

    const auto path = xr_app_config->model_path(MODEL_FILE_F4);

    if (!basic_mesh::load_mesh(path.c_str(),
                               &geometries[1].vertices,
                               &geometries[1].indices,
                               mesh_load_option::remove_points_lines)) {
      XR_LOG_ERR("Failed to load model {}", path.c_str());
      return;
    }
  }

  _vertices = [g = &geometries[0]]() {
    GLuint vb{};
    gl::CreateBuffers(1, &vb);

    const auto store_size =
      static_cast<GLsizeiptr>(container_bytes_size(g[0].vertices) +
                              container_bytes_size(g[1].vertices));

    gl::NamedBufferStorage(vb, store_size, nullptr, gl::MAP_WRITE_BIT);

    scoped_resource_mapping bmap{vb, gl::MAP_WRITE_BIT, (uint32_t) store_size};
    if (bmap) {
      memcpy(bmap.memory(),
             g[0].vertices.data(),
             container_bytes_size(g[0].vertices));

      memcpy((uint8_t*) bmap.memory() + container_bytes_size(g[0].vertices),
             g[1].vertices.data(),
             container_bytes_size(g[1].vertices));
    }

    return scoped_buffer{vb};
  }
  ();

  GL_MARK_BUFFER(raw_handle(_vertices), "Combined vertex buffer");

  {
    gl::CreateBuffers(1, raw_handle_ptr(_indices));
    const auto store_size =
      static_cast<GLsizeiptr>(container_bytes_size(geometries[0].indices) +
                              container_bytes_size(geometries[1].indices));
    gl::NamedBufferStorage(
      raw_handle(_indices), store_size, nullptr, gl::MAP_WRITE_BIT);

    scoped_resource_mapping buffmap{raw_handle(_indices),
                                    gl::MAP_WRITE_BIT,
                                    static_cast<uint32_t>(store_size)};

    memcpy(buffmap.memory(),
           geometries[0].indices.data(),
           container_bytes_size(geometries[0].indices));

    auto dst = static_cast<uint8_t*>(buffmap.memory()) +
               container_bytes_size(geometries[0].indices);

    memcpy(dst,
           geometries[1].indices.data(),
           container_bytes_size(geometries[1].indices));
  }

  GL_MARK_BUFFER(raw_handle(_indices), "Combined index buffer");

  {
    vector<uint32_t> draw_call_ids;
    uint32_t         instance_id{};
    generate_n(back_inserter(draw_call_ids),
               object_instances::instance_count,
               [&instance_id]() {
                 const auto id = instance_id++;
                 return id;
               });

    gl::CreateBuffers(1, raw_handle_ptr(_draw_ids));
    gl::NamedBufferStorage(raw_handle(_draw_ids),
                           container_bytes_size(draw_call_ids),
                           draw_call_ids.data(),
                           0);

    GL_MARK_BUFFER(raw_handle(_draw_ids), "Instance id buffer");
  }

  _vertexarray = [
    vb = raw_handle(_vertices),
    ib = raw_handle(_indices),
    di = raw_handle(_draw_ids)
  ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);

    gl::VertexArrayVertexBuffer(vao, 0, vb, 0, sizeof(vertex_pnt));
    gl::VertexArrayVertexBuffer(vao, 1, di, 0, sizeof(uint32_t));
    gl::VertexArrayElementBuffer(vao, ib);

    gl::VertexArrayAttribFormat(
      vao, 0, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, position));
    gl::VertexArrayAttribFormat(
      vao, 1, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, normal));
    gl::VertexArrayAttribFormat(
      vao, 2, 2, gl::FLOAT, gl::FALSE_, offsetof(vertex_pnt, texcoord));
    gl::VertexArrayAttribIFormat(vao, 3, 1, gl::UNSIGNED_INT, 0);

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);
    gl::VertexArrayAttribBinding(vao, 2, 0);
    gl::VertexArrayAttribBinding(vao, 3, 1);

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);
    gl::EnableVertexArrayAttrib(vao, 2);
    gl::EnableVertexArrayAttrib(vao, 3);

    gl::VertexArrayBindingDivisor(vao, 1, 1);

    return scoped_vertex_array{vao};
  }
  ();

  {
    DrawElementsIndirectCommand draw_cmds[2];

    draw_cmds[0].count         = geometries[0].indices.size();
    draw_cmds[0].instanceCount = 16;
    draw_cmds[0].firstIndex    = 0;
    draw_cmds[0].baseVertex    = 0;
    draw_cmds[0].baseInstance  = 0;

    draw_cmds[1].count         = geometries[1].indices.size();
    draw_cmds[1].instanceCount = 16;
    draw_cmds[1].firstIndex    = draw_cmds[0].count;
    draw_cmds[1].baseVertex    = geometries[0].vertices.size();
    draw_cmds[1].baseInstance  = draw_cmds[0].instanceCount;

    gl::CreateBuffers(1, raw_handle_ptr(_indirect_draw_cmd_buffer));
    gl::NamedBufferStorage(
      raw_handle(_indirect_draw_cmd_buffer), sizeof(draw_cmds), draw_cmds, 0);

    GL_MARK_BUFFER(raw_handle(_indirect_draw_cmd_buffer),
                   "Indirect draw commands buffer");
  }

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/instanced_draw/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs) {
    return;
  }

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

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

  //
  // load textures
  _textures = []() {

    GLuint tex{};
    gl::CreateTextures(gl::TEXTURE_2D_ARRAY, 1, &tex);
    bool store_allocated{false};

    platformstl::readdir_sequence dir_contents_reader{
      xr_app_config->texture_path("uv_grids").c_str(),
      platformstl::readdir_sequence::files |
        platformstl::readdir_sequence::fullPath};

    const auto textures_count =
      std::distance(dir_contents_reader.begin(), dir_contents_reader.end());

    XR_LOG_INFO("Texture count {}", textures_count);

    GLint tex_idx{};
    for_each(begin(dir_contents_reader),
             end(dir_contents_reader),
             [tex, &store_allocated, textures_count, &tex_idx](
               const auto& texture_file) {

               texture_loader tl{texture_file, texture_load_options::flip_y};
               if (!tl) {
                 XR_LOG_ERR("Failed to load texture {}", texture_file);
                 return;
               }

               if (!store_allocated) {
                 gl::TextureStorage3D(tex,
                                      1,
                                      tl.internal_format(),
                                      tl.width(),
                                      tl.height(),
                                      textures_count);
                 store_allocated = true;
               }

               gl::TextureSubImage3D(tex,
                                     0,
                                     0,
                                     0,
                                     tex_idx++,
                                     tl.width(),
                                     tl.height(),
                                     1,
                                     tl.format(),
                                     gl::UNSIGNED_BYTE,
                                     tl.data());

             });

    return scoped_texture{tex};
  }();

  _sampler = []() {
    GLuint smp{};
    gl::CreateSamplers(1, &smp);
    gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return scoped_sampler{smp};
  }();

  //
  // Create instances
  uint32_t idx{};
  generate_n(back_inserter(_obj_instances.instances),
             object_instances::instance_count,
             [ r = &_rand, &idx ]() {

               static constexpr auto OBJ_DST = 50.0f;

               instance_info new_inst;
               new_inst.pitch      = r->next_float(0.0f, two_pi<float>);
               new_inst.roll       = r->next_float(0.0f, two_pi<float>);
               new_inst.yaw        = r->next_float(0.0f, two_pi<float>);
               new_inst.scale      = idx < 16 ? 1.0f : 2.5f;
               new_inst.texture_id = r->next_uint(0, 10);
               new_inst.position   = vec3f{r->next_float(-OBJ_DST, +OBJ_DST),
                                         r->next_float(0.0f, +OBJ_DST),
                                         r->next_float(-OBJ_DST, +OBJ_DST)};

               ++idx;
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

  gl::Enable(gl::DEPTH_TEST);
  gl::Enable(gl::CULL_FACE);
  gl::ViewportIndexedf(0,
                       0.0f,
                       0.0f,
                       static_cast<float>(init_ctx->surface_width),
                       static_cast<float>(init_ctx->surface_height));

  _valid = true;
}

app::instanced_drawing_demo::~instanced_drawing_demo() {}

void app::instanced_drawing_demo::draw(
  const xray::rendering::draw_context_t& ctx) {

  {
    for_each(begin(_obj_instances.instances),
             end(_obj_instances.instances),
             [](instance_info& ii) {
               ii.pitch += instance_info::rotation_speed;
               ii.roll += instance_info::rotation_speed;
               ii.yaw += instance_info::rotation_speed;

               if (ii.pitch > two_pi<float>) {
                 ii.pitch -= two_pi<float>;
               }

               if (ii.roll > two_pi<float>) {
                 ii.roll -= two_pi<float>;
               }

               if (ii.yaw > two_pi<float>) {
                 ii.yaw -= two_pi<float>;
               }
             });

    vector<mat4f> instance_transforms;

    transform(begin(_obj_instances.instances),
              end(_obj_instances.instances),
              back_inserter(instance_transforms),
              [this](instance_info& ii) {

                const auto obj_rotation =
                  mat4f{R3::rotate_xyz(ii.roll, ii.yaw, ii.pitch)};

                return transpose(_scene.camera.projection_view() *
                                 R4::translate(ii.position) * obj_rotation *
                                 mat4f{R3::scale(ii.scale)});
              });

    scoped_resource_mapping buffermap{
      raw_handle(_obj_instances.buffer_transforms),
      gl::MAP_WRITE_BIT,
      static_cast<uint32_t>(container_bytes_size(instance_transforms))};

    if (!buffermap) {
      XR_LOG_ERR("Failed to map instance transform buffer!");
      return;
    }

    memcpy(buffermap.memory(),
           instance_transforms.data(),
           container_bytes_size(instance_transforms));
  }

  gl::ViewportIndexedf(
    0, 0.0f, 0.0f, (float) ctx.window_width, (float) ctx.window_height);

  const GLuint ssbos[] = {raw_handle(_obj_instances.buffer_transforms),
                          raw_handle(_obj_instances.buffer_texture_ids)};

  gl::BindBuffersBase(gl::SHADER_STORAGE_BUFFER, 0, 2, ssbos);

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::white.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  scoped_winding_order_setting set_cw_winding{gl::CW};

  gl::BindVertexArray(raw_handle(_vertexarray));
  gl::BindTextureUnit(0, raw_handle(_textures));
  gl::BindSampler(0, raw_handle(_sampler));

  _pipeline.use();

  scoped_indirect_draw_buffer_binding draw_indirect_buffer_binding{
    raw_handle(_indirect_draw_cmd_buffer)};

  gl::MultiDrawElementsIndirect(gl::TRIANGLES, gl::UNSIGNED_INT, nullptr, 2, 0);
}

void app::instanced_drawing_demo::update(const float /*delta_ms*/) {
  _scene.cam_control.update();
}

void app::instanced_drawing_demo::event_handler(
  const xray::ui::window_event& evt) {

  if (evt.type == event_type::configure) {
    _scene.camera.set_projection(projection::perspective_symmetric(
      static_cast<float>(evt.event.configure.width),
      static_cast<float>(evt.event.configure.height),
      radians(70.0f),
      0.1f,
      1000.0f));

    gl::ViewportIndexedf(0,
                         0.0f,
                         0.0f,
                         static_cast<float>(evt.event.configure.width),
                         static_cast<float>(evt.event.configure.height));

    return;
  }

  if (is_input_event(evt)) {
    _scene.cam_control.input_event(evt);
    return;
  }
}

void app::instanced_drawing_demo::compose_ui() {}
