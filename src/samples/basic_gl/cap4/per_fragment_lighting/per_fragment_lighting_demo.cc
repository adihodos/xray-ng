#include "cap4/per_fragment_lighting/per_fragment_lighting_demo.hpp"
#include "helpers.hpp"
#include "material.hpp"
#include "std_assets.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <algorithm>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

const uint32_t app::per_fragment_lighting_demo::NUM_LIGHTS;

app::per_fragment_lighting_demo::per_fragment_lighting_demo()
{
    init();
}

app::per_fragment_lighting_demo::~per_fragment_lighting_demo() {}

void
app::per_fragment_lighting_demo::init()
{
    //
    // Shaders & program.
    {
        const GLuint compiled_shaders[] = {
            make_shader(gl::VERTEX_SHADER, "shaders/cap4/per_fragment_lighting/vert_shader.glsl"),
            make_shader(gl::FRAGMENT_SHADER, "shaders/cap4/per_fragment_lighting/frag_shader.glsl")
        };

        _draw_prog = gpu_program{ compiled_shaders };
        if (!_draw_prog) {
            XR_LOG_CRITICAL("Failed to create drawing program !");
            return;
        }
    }

    //
    // Vertex + index buffers
    {
        geometry_data_t mesh;
        if (!geometry_factory::load_model(&mesh, MODEL_FILE_SA23, mesh_import_options::remove_points_lines)) {
            geometry_factory::torus(0.7f, 0.3f, 30, 30, &mesh);
        }

        _mesh_index_cnt = mesh.index_count;
        vector<vertex_pn> vertices;
        vertices.reserve(mesh.vertex_count);

        transform(raw_ptr(mesh.geometry),
                  raw_ptr(mesh.geometry) + mesh.vertex_count,
                  back_inserter(vertices),
                  [](const auto& vs_in) {
                      return vertex_pn{ vs_in.position, vs_in.normal };
                  });

        _vertex_buff = [&vertices]() {
            GLuint vbh{};
            gl::CreateBuffers(1, &vbh);
            gl::NamedBufferStorage(vbh, bytes_size(vertices), &vertices[0], 0);

            return vbh;
        }();

        _index_buff = [&mesh]() {
            GLuint ibh{};
            gl::CreateBuffers(1, &ibh);
            gl::NamedBufferStorage(ibh, mesh.index_count * sizeof(uint32_t), raw_ptr(mesh.indices), 0);

            return ibh;
        }();
    }

    //
    // Vertex array object.
    _vertex_arr_obj = [vbh = raw_handle(_vertex_buff), ibh = raw_handle(_index_buff)]() {
        GLuint vao{};
        gl::CreateVertexArrays(1, &vao);
        gl::BindVertexArray(vao);

        gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
        gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pn));

        gl::EnableVertexArrayAttrib(vao, 0);
        gl::EnableVertexArrayAttrib(vao, 1);

        gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, position));
        gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, normal));

        gl::VertexArrayAttribBinding(vao, 0, 0);
        gl::VertexArrayAttribBinding(vao, 1, 0);

        return vao;
    }();

    //
    // Setup lights.
    {
        for (uint32_t idx = 0; idx < per_fragment_lighting_demo::NUM_LIGHTS; ++idx) {
            const auto x_pos = 2.0f * std::cos((two_pi<float> / 5.0f) * static_cast<float>(idx));
            const auto z_pos = 2.0f * std::sin((two_pi<float> / 5.0f) * static_cast<float>(idx));
            _lights[idx].direction = -normalize(vec3f{ x_pos, 1.2f, z_pos + 1.0f });
        }

        _lights[0].ka = { 0.75f, 0.75f, 0.75f, 1.0f };
        _lights[0].kd = { 1.0f, 1.0f, 1.0f, 1.0f };
        _lights[0].ks = { 1.0f, 1.0f, 1.0f, 1.0f };

        _lights[1].ka = { 0.75f, 0.75f, 0.75f, 1.0f };
        _lights[1].kd = { 1.0f, 1.0f, 1.0f, 1.0f };
        _lights[1].ks = { 1.0f, 1.0f, 1.0f, 1.0f };

        _lights[2].ka = { 0.1f, 0.1f, 0.1f, 1.0f };
        _lights[2].kd = { 1.0f, 1.0f, 1.0f, 1.0f };
        _lights[2].ks = { 1.0f, 1.0f, 1.0f, 1.0f };

        _lights[3].ka = { 0.1f, 0.1f, 0.1f, 1.0f };
        _lights[3].kd = { 1.0f, 1.0f, 1.0f, 1.0f };
        _lights[3].ks = { 1.0f, 1.0f, 1.0f, 1.0f };
    }

    _valid = true;
}

void
app::per_fragment_lighting_demo::draw(const xray::rendering::draw_context_t& dc) noexcept
{

    assert(valid());

    gl::BindVertexArray(raw_handle(_vertex_arr_obj));

    {
        //
        // Setup uniforms
        directional_light scene_lights[NUM_LIGHTS];
        transform(begin(_lights), end(_lights), begin(scene_lights), [&dc](const auto& in_light) {
            auto out_light = in_light;
            out_light.direction = normalize(mul_vec(dc.view_matrix, in_light.direction));

            return out_light;
        });

        _draw_prog.set_uniform_block("scene_lights", scene_lights);
        _draw_prog.set_uniform_block("object_material", material::stdc::copper);

        const auto world_mtx = mat4f{ R3::rotate_y(_rotations.y) * R3::rotate_x(_rotations.x) };

        struct matrix_pack
        {
            mat4f world_to_view;
            mat4f norm_to_view;
            mat4f world_view_proj;
        } const matrix_uf_pack{ dc.view_matrix * world_mtx,
                                dc.view_matrix * world_mtx,
                                dc.proj_view_matrix * world_mtx };

        _draw_prog.set_uniform_block("transforms", matrix_uf_pack);
    }

    _draw_prog.bind_to_pipeline();
    gl::DrawElements(gl::TRIANGLES, _mesh_index_cnt, gl::UNSIGNED_INT, nullptr);
}

void
app::per_fragment_lighting_demo::update(const float /*delta_ms*/) noexcept
{
    constexpr auto ROTATION_SPEED_X_AXIS = 0.030f;
    constexpr auto ROTATION_SPEED_Y_AXIS = 0.015f;

    _rotations.x += ROTATION_SPEED_X_AXIS;
    if (_rotations.x > two_pi<float>)
        _rotations.x -= two_pi<float>;

    _rotations.y += ROTATION_SPEED_Y_AXIS;
    if (_rotations.y > two_pi<float>)
        _rotations.y -= two_pi<float>;
}

void
app::per_fragment_lighting_demo::key_event(const int32_t /*key_code*/,
                                           const int32_t /*action*/,
                                           const int32_t /*mods*/) noexcept
{
}
