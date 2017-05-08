#include "xray/rendering/geometry/surface_normal_visualizer.hpp"
#include "xray/base/app_config.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include <opengl/opengl.hpp>

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;

xray::rendering::surface_normal_visualizer::surface_normal_visualizer() {
  _vs = gpu_program_builder{}
          .add_file("shaders/surface_normal_visualizer/vs.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs) {
    return;
  }

  _gs = gpu_program_builder{}
          .add_file("shaders/surface_normal_visualizer/gs.glsl")
          .build<render_stage::e::geometry>();

  if (!_gs) {
    return;
  }

  _fs = gpu_program_builder{}
          .add_file("shaders/surface_normal_visualizer/fs.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs) {
    return;
  }

  _pipeline = program_pipeline{[]() {
    GLuint ph{};
    gl::CreateProgramPipelines(1, &ph);
    return ph;
  }()};

  _pipeline.use_vertex_program(_vs)
    .use_geometry_program(_gs)
    .use_fragment_program(_fs);

  _valid = true;
}

void xray::rendering::surface_normal_visualizer::draw(
  const xray::rendering::draw_context_t& ctx,
  const basic_mesh&                      mesh,
  const xray::math::mat4f&               worldtf,
  const xray::rendering::rgb_color&      draw_color_start,
  const xray::rendering::rgb_color&      draw_color_end,
  const float                            line_length,
  const float                            line_width) {

  assert(valid());

  struct {
    mat4f     WORLD_VIEW_PROJ;
    rgb_color START_COLOR;
    rgb_color END_COLOR;
    float     NLENGTH;
  } gs_ublock;

  gs_ublock.WORLD_VIEW_PROJ = ctx.proj_view_matrix * worldtf;
  gs_ublock.NLENGTH         = line_length;
  gs_ublock.START_COLOR     = draw_color_start;
  gs_ublock.END_COLOR       = draw_color_end;

  _gs.set_uniform_block("TransformMatrices", gs_ublock);

  scoped_line_width_setting lwidth{line_width};
  gl::BindVertexArray(mesh.vertex_array());
  _pipeline.use();
  gl::DrawArrays(gl::TRIANGLES, 0, mesh.index_count());
}
