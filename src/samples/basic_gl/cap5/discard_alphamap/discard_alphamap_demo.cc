#include "cap5/discard_alphamap/discard_alphamap_demo.hpp"
#include "helpers.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
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
#include "xray/rendering/texture_loader.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

template<bool set_on = true>
struct scoped_gl_state
{
  public:
    explicit scoped_gl_state(const GLint state_id) noexcept
        : _state_id{ state_id }
    {
        gl::GetIntegerv(_state_id, &_saved_state);
        set_on ? gl::Enable(_state_id) : gl::Disable(_state_id);
    }

    ~scoped_gl_state() noexcept { _saved_state ? gl::Enable(_state_id) : gl::Disable(_state_id); }

  private:
    GLint _state_id{};
    GLint _saved_state{};

  private:
    XRAY_NO_COPY(scoped_gl_state);
    XRAY_NO_MOVE(scoped_gl_state);
};

using scoped_render_state_on = scoped_gl_state<true>;
using scoped_render_state_off = scoped_gl_state<false>;

extern xray::base::app_config* xr_app_config;

app::discard_alphamap_demo::discard_alphamap_demo()
{
    init();
}

app::discard_alphamap_demo::~discard_alphamap_demo() {}

void
app::discard_alphamap_demo::draw(const xray::rendering::draw_context_t& dc)
{
    assert(valid());

    gl::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

    scoped_render_state_off cullface_off{ gl::CULL_FACE };

    gl::BindVertexArray(raw_handle(_vertex_array));

    const GLuint bound_textures[] = { raw_handle(_base_texture), raw_handle(_discard_map) };
    gl::BindTextures(0, 2, bound_textures);

    const GLuint bound_samplers[] = { raw_handle(_sampler), raw_handle(_sampler) };
    gl::BindSamplers(0, 2, bound_samplers);

    const auto world_xf = mat4f{ R3::rotate_x(_rotation_xy.x) * R3::rotate_y(_rotation_xy.y) };

    struct matrix_pack
    {
        mat4f world_view;
        mat4f normal_view;
        mat4f world_view_projection;
    } const obj_transforms{ dc.view_matrix * world_xf, dc.view_matrix * world_xf, dc.proj_view_matrix * world_xf };

    _draw_program.set_uniform_block("obj_transforms", obj_transforms);

    spotlight scene_lights[NUM_LIGHTS] = { _lights[0], _lights[1] };
    for (auto& l : scene_lights) {
        l.position = mul_point(dc.view_matrix, l.position);
        l.direction = normalize(mul_vec(dc.view_matrix, l.direction));
    }

    _draw_program.set_uniform_block("scene_lights", scene_lights);
    _draw_program.set_uniform("obj_material", 0);
    _draw_program.set_uniform("discard_map", 1);
    _draw_program.bind_to_pipeline();

    gl::DrawElements(gl::TRIANGLES, _mesh_index_count, gl::UNSIGNED_INT, nullptr);
}

void
app::discard_alphamap_demo::update(const float /*delta_ms*/)
{
    _rotation_xy.x += 0.01f;
    if (_rotation_xy.x > two_pi<float>)
        _rotation_xy.x -= two_pi<float>;

    _rotation_xy.y -= 0.01f;
    if (_rotation_xy.y < -two_pi<float>)
        _rotation_xy.y += two_pi<float>;
}

void
app::discard_alphamap_demo::key_event(const int32_t /*key_code*/, const int32_t /*action*/, const int32_t /*mods*/)
{
}

void
app::discard_alphamap_demo::init()
{
    {
        const GLuint shaders[] = { make_shader(gl::VERTEX_SHADER, "shaders/cap5/discard_alphamap/shader.vert"),
                                   make_shader(gl::FRAGMENT_SHADER, "shaders/cap5/discard_alphamap/shader.frag") };

        _draw_program = gpu_program{ shaders };
        if (!_draw_program) {
            XR_LOG_ERR("Failed to compile/link shaders/program");
            return;
        }
    }

    {
        geometry_data_t mesh;
        if (!geometry_factory::load_model(&mesh,
                                          xr_app_config->model_path("f4/f4phantom.obj").c_str(),
                                          mesh_import_options::remove_points_lines)) {
            XR_LOG_ERR("Failed to load model !");
            return;
        }

        vector<vertex_pnt> vertices;
        vertices.reserve(mesh.vertex_count);

        transform(raw_ptr(mesh.geometry),
                  raw_ptr(mesh.geometry) + mesh.vertex_count,
                  back_inserter(vertices),
                  [](const auto& vs_in) {
                      return vertex_pnt{ vs_in.position, vs_in.normal, vs_in.texcoords };
                  });

        _vertex_buffer = [&vertices]() {
            GLuint vbuff{};
            gl::CreateBuffers(1, &vbuff);
            gl::NamedBufferStorage(vbuff, bytes_size(vertices), &vertices[0], 0);

            return vbuff;
        }();

        _index_buffer = [&mesh]() {
            GLuint ibh{};
            gl::CreateBuffers(1, &ibh);
            gl::NamedBufferStorage(ibh, bytes_size(raw_ptr(mesh.indices), mesh.index_count), raw_ptr(mesh.indices), 0);

            return ibh;
        }();

        _mesh_index_count = mesh.index_count;
    }

    _vertex_array = [vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer)]() {
        GLuint vao{};
        gl::CreateVertexArrays(1, &vao);
        gl::BindVertexArray(vao);
        gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
        gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pnt));

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

    _base_texture = []() {
        texture_loader tex_ldr{ c_str_ptr(xr_app_config->texture_path("uv_grids/ash_uvgrid01.jpg")) };

        if (!tex_ldr)
            return GLuint{};

        GLuint texh{};
        gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
        gl::TextureStorage2D(texh, 1, gl::RGB8, tex_ldr.width(), tex_ldr.height());
        gl::TextureSubImage2D(
            texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(), gl::RGB, gl::UNSIGNED_BYTE, tex_ldr.data());
        return texh;
    }();

    _discard_map = []() {
        texture_loader tex_ldr{ c_str_ptr(xr_app_config->texture_path("moss.png")) };

        if (!tex_ldr)
            return GLuint{};

        GLuint texh{};
        gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
        gl::TextureStorage2D(texh, 1, gl::RGBA8, tex_ldr.width(), tex_ldr.height());
        gl::TextureSubImage2D(
            texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(), gl::RGBA, gl::UNSIGNED_BYTE, tex_ldr.data());
        return texh;
    }();

    _sampler = []() {
        GLuint smph{};
        gl::CreateSamplers(1, &smph);
        gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
        gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);
        return smph;
    }();

    _lights[0].cutoff_angle = radians(40.0f);
    _lights[0].position = vec3f{ 10.0f, 10.0f, 0.0f };
    _lights[0].light_power = 80.0f;
    _lights[0].direction = normalize(vec3f{ -1.0f, -1.0f, 0.0f });
    _lights[0].ka = _lights[0].kd = color_palette::material::yellow100;

    _lights[1].cutoff_angle = radians(40.0f);
    _lights[1].position = vec3f{ -10.0f, 10.0f, 0.0f };
    _lights[1].light_power = 80.0f;
    _lights[1].direction = normalize(vec3f{ 1.0f, -1.0f, 0.0f });
    _lights[1].ka = _lights[1].kd = color_palette::material::yellow100;

    _valid = true;
}
