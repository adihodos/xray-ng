#include "soubroutine_test.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/geometry/vertex_format.hpp"
#include "xray/rendering/opengl/gl_core_4_4.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <cassert>
#include <vector>

using namespace std;
using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;

app::basic_object::basic_object()
{
    init();
}

app::basic_object::~basic_object() {}

void
app::basic_object::init()
{
    assert(!valid());
    constexpr auto MODEL_FILE_PATH = "/home/ahodos/assets/viper.obj";

    {
        geometry_data_t mesh_data;
        if (!geometry_factory::load_model(&mesh_data, MODEL_FILE_PATH, mesh_import_options::remove_points_lines)) {
            geometry_factory::torus(0.7f, 0.3f, 30, 30, &torus_mesh);
        }

        vector<vertex_pn> mesh_verts{ mesh_data.vertex_count };
        transform(
            begin(mesh_data.geometry()), end(mesh_data.geometry()), begin(mesh_verts), [](const vertex_pntt_t& vs_in) {
                return vertex_pn{ vs_in.position, vs_in.normal };
            });

        vertex_buffer_ = make_buffer(gl::ARRAY_BUFFER, 0, &mesh_verts[0], mesh_verts.size());

        if (!vertex_buffer_) {
            OUTPUT_DBG_MSG("Failed to create vertex buffer!");
            return;
        }

        index_buffer_ = make_buffer(gl::ELEMENT_ARRAY_BUFFER, 0, raw_ptr(mesh_data.indices), mesh_data.index_count);

        if (!index_buffer_) {
            OUTPUT_DBG_MSG("Failed to create index buffer!");
            return;
        }

        index_count_ = mesh_data.index_count;
    }

    {
        layout_ = make_vertex_array();
        gl::BindVertexArray(raw_handle(layout_));
        gl::BindVertexBuffer(0, raw_handle(vertex_buffer_), 0, sizeof(vertex_pn));
        gl::EnableVertexAttribArray(0);
        gl::EnableVertexAttribArray(1);
        gl::VertexAttribFormat(0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, position));
        gl::VertexAttribFormat(1, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, normal));
        gl::VertexAttribBinding(0, 0);
        gl::VertexAttribBinding(1, 0);
    }

    {
        scoped_shader_handle vertex_shader{ make_shader(gl::VERTEX_SHADER, "less4/lighting.glsl") };

        if (!vertex_shader) {
            OUTPUT_DBG_MSG("Failed to compile vertex shader !");
            return;
        }

        scoped_shader_handle fragment_shader{ make_shader(gl::FRAGMENT_SHADER, "less4/frag_shader.glsl") };

        if (!fragment_shader) {
            OUTPUT_DBG_MSG("Failed to compile fragment shader !");
            return;
        }

        const GLuint shader_handles[] = { raw_handle(vertex_shader), raw_handle(fragment_shader) };

        draw_prog_ = gpu_program{ shader_handles };
        if (!draw_prog_) {
            OUTPUT_DBG_MSG("Failed to create gpu program !");
            return;
        }
    }

    valid_ = true;
}

void
app::basic_object::draw(const xray::rendering::draw_context_t& draw_context)
{
    gl::BindVertexArray(raw_handle(layout_));
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buffer_));

    //
    // set uniforms
    {
    }
    gl::DrawElements(gl::TRIANGLES, index_count_, gl::UNSIGNED_INT, nuillptr);
}

void
app::basic_object::update(const float /* delta */)
{
}