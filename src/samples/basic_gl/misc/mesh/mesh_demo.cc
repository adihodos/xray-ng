#include "misc/mesh/mesh_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <imgui/imgui.h>
#include <iterator>
#include <opengl/opengl.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::mesh_demo::mesh_demo() { init(); }

app::mesh_demo::~mesh_demo() {}

void app::mesh_demo::init() {
  assert(!valid());

  _mesh = basic_mesh{xr_app_config->model_path("f4/f4phantom.obj").c_str()};
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

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

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

  tfmat.view            = draw_ctx.view_matrix;
  tfmat.world_view_proj = draw_ctx.proj_view_matrix;
  tfmat.normals         = draw_ctx.view_matrix;

  _vs.set_uniform_block("TransformMatrices", tfmat);
  _fs.set_uniform("IMAGE_TEX", 0);
  _pipeline.use();

  const GLuint bound_textures[] = {raw_handle(_objtex)};
  gl::BindTextures(0, 1, bound_textures);

  const GLuint bound_samplers[] = {raw_handle(_sampler)};
  gl::BindSamplers(0, 1, bound_samplers);

  gl::Enable(gl::CULL_FACE);
  gl::Enable(gl::DEPTH_TEST);

  gl::DrawElements(
    gl::TRIANGLES, _mesh.index_count(), gl::UNSIGNED_INT, nullptr);
}

void app::mesh_demo::update(const float /* delta_ms */) {}

void app::mesh_demo::event_handler(const xray::ui::window_event& evt) {
  if (evt.type == event_type::mouse_wheel) {
    auto mwe = &evt.event.wheel;
  }
}

void app::mesh_demo::compose_ui() {
  // ImGui::SetNextWindowPos({0, 0}, ImGuiSetCond_Always);
  // ImGui::Begin("Options");
  // ImGui::Combo("Shapes", &_shape_idx, &get_next_shape_description, nullptr,
  //             (int32_t)XR_COUNTOF__(NICE_SHAPES),
  //             (int32_t)XR_COUNTOF__(NICE_SHAPES) / 2);

  // ImGui::SliderInt("Iterations", &_iterations, 32, 4096, nullptr);
  // ImGui::End();
}
