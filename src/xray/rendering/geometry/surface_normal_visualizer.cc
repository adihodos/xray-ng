#include "xray/rendering/geometry/surface_normal_visualizer.hpp"

#include <opengl/opengl.hpp>

#include "xray/base/app_config.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"

extern xray::base::ConfigSystem* xr_app_config;

using namespace xray::base;
using namespace xray::math;

xray::rendering::surface_normal_visualizer::surface_normal_visualizer()
{
    _vs = gpu_program_builder{}.add_file("shaders/surface_normal_visualizer/vs.glsl").build<render_stage::e::vertex>();

    if (!_vs) {
        return;
    }

    _gs =
        gpu_program_builder{}.add_file("shaders/surface_normal_visualizer/gs.glsl").build<render_stage::e::geometry>();

    if (!_gs) {
        return;
    }

    _fs =
        gpu_program_builder{}.add_file("shaders/surface_normal_visualizer/fs.glsl").build<render_stage::e::fragment>();

    if (!_fs) {
        return;
    }

    _pipeline = program_pipeline{ []() {
        GLuint ph{};
        gl::CreateProgramPipelines(1, &ph);
        return ph;
    }() };

    _pipeline.use_vertex_program(_vs).use_geometry_program(_gs).use_fragment_program(_fs);
}

void
xray::rendering::surface_normal_visualizer::draw(const basic_mesh& mesh,
                                                 const xray::math::mat4f& world_view_proj,
                                                 const xray::rendering::rgb_color& draw_color_start,
                                                 const xray::rendering::rgb_color& draw_color_end,
                                                 const float line_length)
{

    struct GeometryShaderUbo
    {
        mat4f WORLD_VIEW_PROJ;
        rgb_color START_COLOR;
        rgb_color END_COLOR;
        float NLENGTH;
    } const gs_ublock = {
        .WORLD_VIEW_PROJ = world_view_proj,
        .START_COLOR = draw_color_start,
        .END_COLOR = draw_color_end,
        .NLENGTH = line_length,
    };

    _gs.set_uniform_block("TransformMatrices", gs_ublock);
    _gs.flush_uniforms();

    gl::BindVertexArray(mesh.vertex_array());

    _pipeline.use(false);
    // gl::DrawArrays(gl::TRIANGLES, 0, mesh.index_count());
    gl::DrawElements(gl::TRIANGLES, static_cast<GLsizei>(mesh.index_count()), gl::UNSIGNED_INT, nullptr);
}
