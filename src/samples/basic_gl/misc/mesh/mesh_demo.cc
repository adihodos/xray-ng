#include "misc/mesh/mesh_demo.hpp"
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

struct scoped_polygon_mode_setting {
public:
  explicit scoped_polygon_mode_setting(const int32_t new_mode) {
    gl::GetIntegerv(gl::POLYGON_MODE, &_old_mode);
    gl::PolygonMode(gl::FRONT_AND_BACK, (GLenum) new_mode);
  }

  ~scoped_polygon_mode_setting() {
    gl::PolygonMode(gl::FRONT_AND_BACK, (GLenum) _old_mode);
  }

private:
  GLint _old_mode{gl::NONE};
  XRAY_NO_COPY(scoped_polygon_mode_setting);
};

app::aabb_draw::aabb_draw() {
  _vs = gpu_program_builder{}
          .add_file("shaders/draw_aabb/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _gs = gpu_program_builder{}
          .add_file("shaders/draw_aabb/gs.glsl")
          .build<render_stage::e::geometry>();

  if (!_gs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/draw_aabb/fs.pass.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint pp{};
    gl::CreateProgramPipelines(1, &pp);
    return pp;
  }()};

  _pipeline.use_vertex_program(_vs)
    .use_geometry_program(_gs)
    .use_fragment_program(_fs);

  _valid = true;
}

void app::aabb_draw::draw(const draw_context_t& ctx) {
  gl::Disable(gl::DEPTH_TEST);
  gl::Disable(gl::CULL_FACE);
  gl::BindVertexArray(0);

  struct {
    mat4f     world_view_proj;
    rgb_color line_start;
    rgb_color line_end;
    float     width;
    float     height;
    float     depth;
  } box_params = {ctx.proj_view_matrix * R4::translate(_boundingbox.center()),
                  color_palette::web::red,
                  color_palette::web::green,
                  _boundingbox.width() * 0.5f,
                  _boundingbox.height() * 0.5f,
                  _boundingbox.depth() * 0.5f};

  _gs.set_uniform_block("DrawParams", box_params);
  _pipeline.use();

  gl::DrawArrays(gl::POINTS, 0, 1);
  gl::Enable(gl::CULL_FACE);
  gl::Enable(gl::DEPTH_TEST);
}

app::mesh_demo::mesh_demo() { init(); }

app::mesh_demo::~mesh_demo() {}

void app::mesh_demo::init() {
  assert(!valid());

  if (!_abbdraw)
    return;

  static constexpr auto MODEL_FILE =
    //"SuperCobra.3ds";
    "f4/f4phantom.obj";
  //"sa23/sa23_aurora.obj";
  //"cube_rounded.obj";
  //"stanford/dragon/dragon.obj";
  //"stanford/teapot/teapot.obj";
  //"stanford/sportscar/sportsCar.obj";
  //"stanford/sibenik/sibenik.obj";
  //"stanford/rungholt/rungholt.obj";
  //"stanford/lost-empire/lost_empire.obj";
  //"stanford/cube/cube.obj";
  //"stanford/head/head.OBJ";

  geometry_data_t blob;
  geometry_factory::grid(16.0f, 16.0f, 128, 128, &blob);

  vector<vertex_pnt> verts;
  transform(raw_ptr(blob.geometry),
            raw_ptr(blob.geometry) + blob.vertex_count,
            back_inserter(verts),
            [](const vertex_pntt& vsin) {
              return vertex_pnt{vsin.position, vsin.normal, vsin.texcoords};
            });

  const vertex_ripple_parameters rparams{_rippledata.amplitude,
                                         _rippledata.period * two_pi<float>,
                                         _rippledata.width,
                                         _rippledata.height};

  vertex_effect::ripple(
    gsl::span<vertex_pnt>{verts},
    gsl::span<uint32_t>{raw_ptr(blob.indices),
                        raw_ptr(blob.indices) + blob.index_count},
    rparams);

  //  _mesh = basic_mesh{xr_app_config->model_path(MODEL_FILE).c_str()};
  _mesh = basic_mesh{verts.data(),
                     verts.size(),
                     raw_ptr(blob.indices),
                     blob.index_count,
                     mesh_type::writable};

  if (!_mesh) {
    return;
  }

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/mesh/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/misc/mesh/fs.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _vsnormals = gpu_program_builder{}
                 .add_file("shaders/misc/mesh/vs.pass.glsl")
                 .build<render_stage::e::vertex>();

  if (!_vsnormals)
    return;

  _gsnormals = gpu_program_builder{}
                 .add_file("shaders/misc/mesh/gs.glsl")
                 .build<render_stage::e::geometry>();

  if (!_gsnormals)
    return;

  _fsnormals = gpu_program_builder{}
                 .add_file("shaders/misc/mesh/fs.color.glsl")
                 .build<render_stage::e::fragment>();

  if (!_fsnormals)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  texture_loader tldr{
    xr_app_config->texture_path("uv_grids/ash_uvgrid01.jpg").c_str()};

  if (!tldr) {
    return;
  }

  gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_objtex));
  gl::TextureStorage2D(raw_handle(_objtex),
                       1,
                       tldr.internal_format(),
                       tldr.width(),
                       tldr.height());
  gl::TextureSubImage2D(raw_handle(_objtex),
                        0,
                        0,
                        0,
                        tldr.width(),
                        tldr.height(),
                        tldr.format(),
                        gl::UNSIGNED_BYTE,
                        tldr.data());

  gl::CreateSamplers(1, raw_handle_ptr(_sampler));
  const auto smp = raw_handle(_sampler);
  gl::SamplerParameteri(smp, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
  gl::SamplerParameteri(smp, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

  //  gl::Enable(gl::CULL_FACE);
  gl::Enable(gl::DEPTH_TEST);

  _valid = true;
}

void app::mesh_demo::draw(const xray::rendering::draw_context_t& draw_ctx) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

  gl::ViewportIndexedf(0,
                       0.0f,
                       0.0f,
                       (float) draw_ctx.window_width,
                       (float) draw_ctx.window_height);

  gl::BindVertexArray(_mesh.vertex_array());

  struct transform_matrices {
    mat4f world_view_proj;
    mat4f normals;
    mat4f view;
  } tfmat;

  tfmat.view            = draw_ctx.view_matrix * mat4f{R3::scale(1.2f)};
  tfmat.world_view_proj = draw_ctx.projection_matrix * tfmat.view;
  tfmat.normals         = tfmat.view;

  _vs.set_uniform_block("TransformMatrices", tfmat);
  _fs.set_uniform("IMAGE_TEX", 0);

  _pipeline.use_vertex_program(_vs)
    .use_fragment_program(_fs)
    .disable_stage(render_stage::e::geometry)
    .use();

  const GLuint bound_textures[] = {raw_handle(_objtex)};
  gl::BindTextures(0, 1, bound_textures);

  const GLuint bound_samplers[] = {raw_handle(_sampler)};
  gl::BindSamplers(0, 1, bound_samplers);

  {
    scoped_polygon_mode_setting set_wireframe{
      _drawparams.draw_wireframe ? gl::LINE : gl::FILL};

    gl::DrawElements(
      gl::TRIANGLES, _mesh.index_count(), gl::UNSIGNED_INT, nullptr);
  }

  if (_drawparams.drawnormals) {

    struct {
      mat4f     WORLD_VIEW_PROJECTION;
      rgb_color COLOR_START;
      rgb_color COLOR_END;
      float     NORMAL_LENGTH;
    } gs_uniforms;

    gs_uniforms.WORLD_VIEW_PROJECTION = tfmat.world_view_proj;
    gs_uniforms.COLOR_START           = _drawparams.start_color;
    gs_uniforms.COLOR_END             = _drawparams.end_color;
    gs_uniforms.NORMAL_LENGTH         = _drawparams.normal_len;

    _gsnormals.set_uniform_block("TransformMatrices", gs_uniforms);

    _pipeline.use_vertex_program(_vsnormals)
      .use_geometry_program(_gsnormals)
      .use_fragment_program(_fsnormals)
      .use();

    gl::DrawElements(
      gl::TRIANGLES, _mesh.index_count(), gl::UNSIGNED_INT, nullptr);
  }

  if (_drawparams.draw_boundingbox) {
    _abbdraw.set_aabb(_mesh.aabb());
    _abbdraw.draw(draw_ctx);
  }
}

void app::mesh_demo::update(const float /* delta_ms */) {}

void app::mesh_demo::event_handler(const xray::ui::window_event& evt) {
  // if (evt.type == event_type::mouse_wheel) {
  //   auto mwe = &evt.event.wheel;
  // }
}

void app::mesh_demo::compose_ui() {
  ImGui::SetNextWindowPos({0, 0}, ImGuiSetCond_Always);
  ImGui::Begin("Options");

  ImGui::Checkbox("Draw wireframe", &_drawparams.draw_wireframe);
  ImGui::Checkbox("Draw normals", &_drawparams.drawnormals);

  if (_drawparams.drawnormals) {
    ImGui::SliderFloat("Normal length", &_drawparams.normal_len, 0.1f, 3.0f);
    ImGui::SliderFloat3(
      "Start color", _drawparams.start_color.components, 0.0f, 1.0f);
    ImGui::SliderFloat3(
      "End color", _drawparams.end_color.components, 0.0f, 1.0f);
  }

  // ImGui::Checkbox("Draw bounding box", &_drawparams.draw_boundingbox);

  ImGui::End();

  ImGui::Begin("Mesh parameters");
  bool update_mesh =
    ImGui::SliderFloat("Amplitude", &_rippledata.amplitude, 0.1f, 10.0f);

  update_mesh |= ImGui::SliderFloat("Period", &_rippledata.period, 0.0f, 10.0f);
  update_mesh |= ImGui::SliderFloat("Width", &_rippledata.width, 0.1f, 32.0f);
  update_mesh |= ImGui::SliderFloat("Height", &_rippledata.height, 0.1f, 32.0f);

  ImGui::End();

  if (!update_mesh)
    return;

  const vertex_ripple_parameters rparams{_rippledata.amplitude,
                                         _rippledata.period * two_pi<float>,
                                         _rippledata.width,
                                         _rippledata.height};

  vertex_effect::ripple(_mesh.vertices(), _mesh.indices(), rparams);
  _mesh.update_vertices();
}
