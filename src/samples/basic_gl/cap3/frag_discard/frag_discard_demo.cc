#include "frag_discard_demo.hpp"
#include "light_source.hpp"
#include "material.hpp"
#include "std_assets.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <gsl.h>
#include <span.h>
#include <vector>

using namespace xray::math;
using namespace xray::base;
using namespace xray::rendering;
using namespace std;

static constexpr app::light_source LIGHT_SOURCE{ { 0.0f, 12.0f, 0.0f },
                                                 0.0f,
                                                 { 0.1f, 0.1f, 0.1f },
                                                 { 1.0f, 1.0f, 1.0f },
                                                 { 1.0f, 1.0f, 1.0f } };

static constexpr app::material STD_MATERIALS[] = {
    //
    // polished copper
    { { 0.0f, 0.0f, 0.0f },
      { 0.2295f, 0.08825f, 0.0275f },
      { 0.5508f, 0.2118f, 0.066f },
      { 0.580594f, 0.223257f, 0.0695701f, 51.2f } },
    //
    // polished gold
    { { 0.0f, 0.0f, 0.0f },
      { 0.24725f, 0.2245f, 0.0645f },
      { 0.34615f, 0.3143f, 0.0903f },
      { 0.797357f, 0.723991f, 0.208006f, 83.2f } }
};

app::frag_discard_demo::frag_discard_demo()
{
    init();
}

app::frag_discard_demo::~frag_discard_demo() {}

void
app::frag_discard_demo::update(const float /*delta_ms*/) noexcept
{
}

void
app::frag_discard_demo::init()
{
    //
    // load mesh
    {
        geometry_data_t mesh;
        if (!geometry_factory::load_model(&mesh, MODEL_FILE_SA23, mesh_import_options::remove_points_lines)) {
            geometry_factory::torus(0.7f, 0.3f, 30, 30, &mesh);
        }

        vector<vertex_pnt> vertices;
        vertices.reserve(mesh.vertex_count);

        auto input_verts = gsl::span<vertex_pntt>{ raw_ptr(mesh.geometry), raw_ptr(mesh.geometry) + mesh.vertex_count };

        transform(begin(input_verts), end(input_verts), back_inserter(vertices), [](const auto& in_vert) {
            return vertex_pnt{ in_vert.position, in_vert.normal, in_vert.texcoords };
        });

        vertex_buff_ = [&vertices]() {
            GLuint vbuff{ 0 };
            gl::CreateBuffers(1, &vbuff);
            gl::NamedBufferStorage(vbuff, vertices.size() * sizeof(vertices[0]), &vertices[0], gl::MAP_READ_BIT);

            return vbuff;
        }();

        index_buff_ = [&mesh]() {
            GLuint bhandle{ 0 };
            gl::CreateBuffers(1, &bhandle);
            gl::NamedBufferStorage(
                bhandle, mesh.index_count * sizeof(uint32_t), raw_ptr(mesh.indices), gl::MAP_READ_BIT);
            return bhandle;
        }();

        mesh_indices_ = mesh.index_count;
    }

    //
    // create vertex layout
    {
        vertex_layout_ = [ibh = raw_handle(index_buff_), vbh = raw_handle(vertex_buff_)]() {
            GLuint vao{};

            gl::CreateVertexArrays(1, &vao);
            gl::BindVertexArray(vao);

            gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pnt));
            gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);

            gl::EnableVertexArrayAttrib(vao, 0);
            gl::EnableVertexArrayAttrib(vao, 1);
            gl::EnableVertexArrayAttrib(vao, 2);

            gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, position));
            gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, normal));
            gl::VertexArrayAttribFormat(vao, 2, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, texcoord));

            gl::VertexArrayAttribBinding(vao, 0, 0);
            gl::VertexArrayAttribBinding(vao, 1, 0);
            gl::VertexArrayAttribBinding(vao, 2, 0);

            return vao;
        }();
    }

    //
    // shaders
    {
        constexpr const char* const VS_FILE = "shaders/cap3/frag_discard/vert_shader.glsl";
        constexpr const char* const FS_FILE = "shaders/cap3/frag_discard/frag_shader.glsl";

        auto vert_shader = make_shader(gl::VERTEX_SHADER, VS_FILE);
        if (!vert_shader) {
            XR_LOG_CRITICAL("Failed to compile shader {}", VS_FILE);
            return;
        }

        auto frag_shader = make_shader(gl::FRAGMENT_SHADER, FS_FILE);
        if (!frag_shader) {
            XR_LOG_CRITICAL("Failed to compile shader {}", FS_FILE);
            return;
        }

        const GLuint shader_handles[] = { vert_shader, frag_shader };

        draw_prog_ = gpu_program{ shader_handles };
        if (!draw_prog_) {
            XR_LOG_CRITICAL("Failed to link program !");
            return;
        }
    }

    valid_ = true;
}

void
app::frag_discard_demo::key_event(const int32_t /*key_code*/, const int32_t /*action*/, const int32_t /*mods*/) noexcept
{
}

void
app::frag_discard_demo::draw(const xray::rendering::draw_context_t& dc) noexcept
{

    assert(valid());

    gl::BindVertexArray(raw_handle(vertex_layout_));
    //  gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buff_));

    //
    // set uniforms
    {
        auto light_src = LIGHT_SOURCE;
        light_src.position = mul_point(dc.view_matrix, light_src.position);

        draw_prog_.set_uniform_block("light_source", light_src);
        draw_prog_.set_uniform_block("material", STD_MATERIALS[0]);

        struct transforms
        {
            mat4f mv;
            mat4f nv;
            mat4f mvp;
        } const tf_uniform{ dc.view_matrix, dc.view_matrix, dc.proj_view_matrix };

        draw_prog_.set_uniform_block("transforms", tf_uniform);
    }

    draw_prog_.bind_to_pipeline();
    gl::DrawElements(gl::TRIANGLES, mesh_indices_, gl::UNSIGNED_INT, nullptr);
}
