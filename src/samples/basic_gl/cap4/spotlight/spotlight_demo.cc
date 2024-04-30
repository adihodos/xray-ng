#include "cap4/spotlight/spotlight_demo.hpp"
#include "helpers.hpp"
#include "material.hpp"
#include "std_assets.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

constexpr uint32_t app::spotlight_demo::NUM_LIGHTS;

app::spotlight_demo::spotlight_demo()
{
    init();
}

app::spotlight_demo::~spotlight_demo() {}

struct ripple_params
{
    float amplitude;
    float period;
    float grid_width;
    float grid_depth;
};

void
ripple_grid(xray::rendering::geometry_data_t* mesh, const ripple_params& rp) noexcept
{

    assert(mesh != nullptr);

    using namespace xray::rendering;
    using namespace xray::base;

    auto vertices = gsl::span<vertex_pntt>{ raw_ptr(mesh->geometry), raw_ptr(mesh->geometry) + mesh->vertex_count };

    const auto scale_x = rp.grid_width * 0.5f;
    const auto scale_z = rp.grid_depth * 0.5f;

    for (auto& vertex : vertices) {
        vertex.normal = vec3f::stdc::zero;
        const auto x_pos = vertex.position.x / scale_x;
        const auto z_pos = vertex.position.z / scale_z;

        vertex.position.y = rp.amplitude * std::sin(rp.period * (x_pos * x_pos + z_pos * z_pos));
    }

    const auto indices = gsl::span<uint32_t>{ raw_ptr(mesh->indices), static_cast<ptrdiff_t>(mesh->index_count) };

    assert((indices.length() % 3) == 0);

    for (uint32_t i = 0, idx_count = indices.length(); i < idx_count; i += 3) {
        auto& v0 = vertices[indices[i + 0]];
        auto& v1 = vertices[indices[i + 1]];
        auto& v2 = vertices[indices[i + 2]];

        const auto normal = cross(v2.position - v0.position, v1.position - v0.position);

        v0.normal += normal;
        v1.normal += normal;
        v2.normal += normal;
    }

    for (auto& vertex : vertices) {
        vertex.normal = normalize(vertex.normal);
    }
}

void
app::spotlight_demo::init()
{
    //
    //
    {
        //
        // shaders bla bla
        const GLuint compiled_shaders[] = { make_shader(gl::VERTEX_SHADER, "shaders/cap4/spotlight/shader.vert"),
                                            make_shader(gl::FRAGMENT_SHADER, "shaders/cap4/spotlight/shader.frag") };

        _draw_prog = gpu_program{ compiled_shaders };
        if (!_draw_prog) {
            XR_LOG_ERR("Failed to create and link program !");
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

        _meshes[0].base_vertex = 0;
        _meshes[0].index_count = mesh.index_count;
        _meshes[0].index_offset = 0;
        _meshes[0].mat = material::stdc::copper;
        _meshes[0].front_ccw = true;
        _meshes[0].translation = { 0.0f, 2.0f, 0.0f };

        geometry_data_t mesh_grid;
        geometry_factory::grid(16.0, 16.0f, 128, 128, &mesh_grid);

        const ripple_params rp{ 0.6f, 3.0f * two_pi<float>, 16.0f, 16.0f };
        ripple_grid(&mesh_grid, rp);

        _meshes[1].base_vertex = mesh.vertex_count;
        _meshes[1].index_count = mesh_grid.index_count;
        _meshes[1].index_offset = _meshes[0].index_count;
        _meshes[1].front_ccw = false;
        _meshes[1].mat = material::stdc::red_plastic;

        vector<vertex_pn> vertices;
        vertices.reserve(mesh.vertex_count + mesh_grid.vertex_count);

        transform(raw_ptr(mesh.geometry),
                  raw_ptr(mesh.geometry) + mesh.vertex_count,
                  back_inserter(vertices),
                  [](const auto& in_vertex) {
                      return vertex_pn{ in_vertex.position, in_vertex.normal };
                  });

        transform(raw_ptr(mesh_grid.geometry),
                  raw_ptr(mesh_grid.geometry) + mesh_grid.vertex_count,
                  back_inserter(vertices),
                  [](const auto& in_vertex) {
                      return vertex_pn{ in_vertex.position, in_vertex.normal };
                  });

        _vertex_buffer = [&vertices]() {
            GLuint vbuff{};
            gl::CreateBuffers(1, &vbuff);
            gl::NamedBufferStorage(vbuff, bytes_size(vertices), &vertices[0], 0);

            return vbuff;
        }();

        vector<uint32_t> indices;
        indices.reserve(_meshes[0].index_count + _meshes[1].index_count);

        copy(raw_ptr(mesh.indices), raw_ptr(mesh.indices) + mesh.index_count, back_inserter(indices));
        copy(raw_ptr(mesh_grid.indices), raw_ptr(mesh_grid.indices) + mesh_grid.index_count, back_inserter(indices));

        _index_buffer = [&indices]() {
            GLuint ibuff{};
            gl::CreateBuffers(1, &ibuff);
            gl::NamedBufferStorage(ibuff, bytes_size(indices), &indices[0], 0);

            return ibuff;
        }();
    }

    //
    //  Vertex array object
    _vertex_array_obj = [vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer)]() {
        GLuint vao{};
        gl::CreateVertexArrays(1, &vao);
        gl::BindVertexArray(vao);

        gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pn));
        gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);

        gl::EnableVertexArrayAttrib(vao, 0);
        gl::EnableVertexArrayAttrib(vao, 1);

        gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, position));
        gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, normal));

        gl::VertexArrayAttribBinding(vao, 0, 0);
        gl::VertexArrayAttribBinding(vao, 1, 0);

        return vao;
    }();

    {
        const rgb_color light_colors[NUM_LIGHTS] = { color_palette::material::white,
                                                     color_palette::material::white,
                                                     color_palette::material::white };

        const vec3f light_pos[NUM_LIGHTS] = { { -10.0f, 10.0f, 0.0f }, { +10.0f, 10.0f, 0.0f }, { 0.0f, 10.0f, 0.0f } };

        const vec3f light_lookat[NUM_LIGHTS] = { { -5.0f, 0.0f, 0.0f }, { +5.0f, 0.0f, 0.0f }, { +0.0f, 0.0f, 0.0f } };

        for (uint32_t idx = 0; idx < spotlight_demo::NUM_LIGHTS; ++idx) {
            //      const auto x_pos =
            //          2.0f * std::cos((two_pi<float> /
            //          static_cast<float>(NUM_LIGHTS) *
            //                           static_cast<float>(idx)));
            //      const auto z_pos =
            //          2.0f * std::sin((two_pi<float> /
            //          static_cast<float>(NUM_LIGHTS)) *
            //                          static_cast<float>(idx));

            _lights[idx].position = light_pos[idx];
            _lights[idx].direction = normalize(light_lookat[idx] - light_pos[idx]);
            _lights[idx].ka = _lights[idx].kd = _lights[idx].ks = light_colors[idx];
            _lights[idx].cutoff_angle = std::cos(radians(60.0f));
            _lights[idx].light_power = 100.0f;
        }
    }

    _valid = true;
}

void
app::spotlight_demo::draw(const xray::rendering::draw_context_t& dc)
{
    assert(valid());

    gl::BindVertexArray(raw_handle(_vertex_array_obj));

    {
        //
        // Set shaders, bla bla
        spotlight scene_lights[NUM_LIGHTS];
        transform(begin(_lights), end(_lights), begin(scene_lights), [&dc](const auto& in_light) {
            auto out_light = in_light;
            out_light.position = mul_point(dc.view_matrix, in_light.position);
            out_light.direction = normalize(mul_vec(dc.view_matrix, in_light.direction));

            return out_light;
        });

        _draw_prog.set_uniform_block("light_sources", scene_lights);
    }

    //  gl::PolygonMode(gl::FRONT_AND_BACK, gl::LINE);

    for (const auto& mesh : _meshes) {
        if (!mesh.front_ccw) {
            gl::FrontFace(gl::CW);
        }

        const auto world = R4::translate(mesh.translation) * mat4f{ R3::rotate_x(mesh.rotation_xy.x) } *
                           mat4f{ R3::rotate_y(mesh.rotation_xy.y) };

        struct matrix_transform_pack
        {
            mat4f world_to_view;
            mat4f normals_to_view;
            mat4f world_view_proj;
        } transform_pack{ dc.view_matrix * world, dc.view_matrix * world, dc.proj_view_matrix * world };

        _draw_prog.set_uniform_block("obj_matrix_pack", transform_pack);
        _draw_prog.set_uniform_block("object_material", mesh.mat);
        _draw_prog.bind_to_pipeline();

        gl::DrawElementsBaseVertex(gl::TRIANGLES,
                                   mesh.index_count,
                                   gl::UNSIGNED_INT,
                                   offset_cast<uint32_t>(mesh.index_offset),
                                   static_cast<GLint>(mesh.base_vertex));

        if (!mesh.front_ccw) {
            gl::FrontFace(gl::CCW);
        }
    }
}

void
app::spotlight_demo::update(const float /*delta_ms*/)
{
    if (!_rotate_mesh)
        return;

    constexpr auto r_speed_x = 0.025f;
    constexpr auto r_speed_y = 0.025f;

    auto& mesh = _meshes[0];
    mesh.rotation_xy.x += r_speed_x;
    mesh.rotation_xy.y += r_speed_y;

    if (mesh.rotation_xy.x > two_pi<float>)
        mesh.rotation_xy.x -= two_pi<float>;

    if (mesh.rotation_xy.y > two_pi<float>)
        mesh.rotation_xy.y -= two_pi<float>;
};

void
app::spotlight_demo::key_event(const int32_t key_code, const int32_t action, const int32_t /*mods*/)
{

    if ((key_code == GLFW_KEY_BACKSPACE) && (action == GLFW_PRESS)) {
        _meshes[0].rotation_xy = vec2f::stdc::zero;
        _rotate_mesh = !_rotate_mesh;
    }
}
