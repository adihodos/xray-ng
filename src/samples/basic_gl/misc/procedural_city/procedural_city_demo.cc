#include "misc/procedural_city/procedural_city_demo.hpp"

#include <array>
#include <random>
#include <span>
#include <vector>

#include <fmt/core.h>
#include <itlib/small_vector.hpp>
#include <Lz/Lz.hpp>

#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/scene/fps_camera_controller.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/user_interface.hpp"
#include <opengl/opengl.hpp>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace xray::scene;
using namespace std;

extern xray::base::ConfigSystem* xr_app_config;

static constexpr uint32_t INSTANCE_COUNT{ 1024u };
static constexpr const char* const TEXTURES[] = { "uv_grids/ash_uvgrid01.jpg", "uv_grids/ash_uvgrid02.jpg",
                                                  "uv_grids/ash_uvgrid03.jpg", "uv_grids/ash_uvgrid04.jpg",
                                                  "uv_grids/ash_uvgrid05.jpg", "uv_grids/ash_uvgrid06.jpg",
                                                  "uv_grids/ash_uvgrid07.jpg", "uv_grids/ash_uvgrid08.jpg" };

app::ProceduralCityDemo::ProceduralCityDemo(PrivateConstructionToken,
                                            const app::init_context_t& initctx,
                                            app::ProceduralCityDemo::RenderState&& renderState)
    : DemoBase(initctx)
    , mRenderState(std::move(renderState))
{
    camera_lens_parameters lp{};
    lp.farplane = 1000.0f;
    lp.aspect_ratio = (float)initctx.surface_width / (float)initctx.surface_height;

    mRenderState._camcontrol.set_lens_parameters(lp);
}

app::ProceduralCityDemo::~ProceduralCityDemo() {}

void
app::ProceduralCityDemo::draw(const xray::ui::window_loop_event& loopEvent)
{

    gl::ViewportIndexedf(0, 0.0f, 0.0f, (float)loopEvent.wnd_width, (float)loopEvent.wnd_height);

    const float CLEAR_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, CLEAR_COLOR);
    gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

    gl::BindBufferBase(gl::SHADER_STORAGE_BUFFER, 0, raw_handle(mRenderState._instancedata));

    //
    // draw fluid
    // if (_demo_options.draw_fluid) {
    //     gl::BindVertexArray(_fluid.mesh().vertex_array());
    //     _vs.set_uniform("INSTANCE_OFFSET", (int32_t)INSTANCE_COUNT);
    //     _vs.set_uniform_block("Transforms", _camera.projection_view());
    //     _pipeline.use();
    //
    //     gl::BindTextureUnit(0, _fluid.texture());
    //     gl::BindSampler(0, raw_handle(_sampler));
    //
    //     scoped_polygon_mode_setting setwf{ gl::LINE };
    //     gl::DrawElementsInstanced(gl::TRIANGLES, _fluid.mesh().index_count(), gl::UNSIGNED_INT, nullptr, 1);
    // }

    //
    // draw buildings
    if (mDemoOptions.draw_buildings) {
        gl::BindVertexArray(mRenderState._mesh.vertex_array());

        mRenderState._pipeline.use(false);

        mRenderState._vs.set_uniform("INSTANCE_OFFSET", 0);
        mRenderState._vs.set_uniform_block("Transforms", mRenderState._camera.projection_view());
        mRenderState._vs.flush_uniforms();

        gl::BindTextureUnit(0, raw_handle(mRenderState._objtex));
        gl::BindSampler(0, raw_handle(mRenderState._sampler));

        gl::DrawElementsInstanced(
            gl::TRIANGLES, mRenderState._mesh.index_count(), gl::UNSIGNED_INT, nullptr, INSTANCE_COUNT);
    }
}

void
app::ProceduralCityDemo::event_handler(const xray::ui::window_event& evt)
{

    if (evt.type == event_type::configure) {
        camera_lens_parameters lp;
        lp.farplane = 1000.0f;
        lp.aspect_ratio = (float)evt.event.configure.width / (float)evt.event.configure.height;

        mRenderState._camcontrol.set_lens_parameters(lp);
        return;
    }

    if (is_input_event(evt)) {
        _ui->input_event(evt);
        if (_ui->wants_input())
            return;

        if (evt.type == event_type::key && evt.event.key.keycode == KeySymbol::escape) {
            _quit_receiver();
            return;
        }

        mRenderState._camcontrol.input_event(evt);
    }
}

void
app::ProceduralCityDemo::loop_event(const RenderEvent& render_event)
{
    const xray::ui::window_loop_event& loopEvent = render_event.loop_event;
    _ui->tick(1.0f / 60.0f);
    mRenderState._camcontrol.update_camera(mRenderState._camera);
    draw(loopEvent);
    draw_ui(loopEvent);
}

void
app::ProceduralCityDemo::draw_ui(const xray::ui::window_loop_event& loopEvent)
{
    _ui->new_frame(loopEvent.wnd_width, loopEvent.wnd_height);

    ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
    if (ImGui::Begin("Options")) {

        ImGui::Checkbox("Draw fluid", &mDemoOptions.draw_fluid);
        if (mDemoOptions.draw_fluid) {
            ImGui::Checkbox("Freeze fluid state", &mDemoOptions.freeze_fluid);
        }
        ImGui::Checkbox("Draw buildings", &mDemoOptions.draw_buildings);

        ImGui::Separator();

        array<char, 512> textBuffer{};

        auto [it, cch] = fmt::format_to_n(textBuffer.data(),
                                          textBuffer.size(),
                                          "::: Camera :::\nOrigin: {:.3f}\nRight: {:.3f}\nUp: {}\nDirection: {:.3f}",
                                          mRenderState._camera.origin(),
                                          mRenderState._camera.right(),
                                          mRenderState._camera.up(),
                                          mRenderState._camera.direction());

        ImGui::TextUnformatted(textBuffer.data(), textBuffer.data() + cch);
    }

    ImGui::End();
    _ui->draw();
}

struct per_instance_data
{
    mat4f tfworld;
    rgb_color color;
    int32_t texid;
    int32_t pad[3];
};

xray::base::unique_pointer<app::DemoBase>
app::ProceduralCityDemo::create(const init_context_t& initContext)
{
    //
    // basic cube mesh that will be instanced to produce our buildings
    basic_mesh cubeMesh = []() {
        geometry_data_t blob;
        geometry_factory::box(1.0f, 1.0f, 1.0f, &blob);

        const itlib::small_vector<vertex_pnt, 36> cubeVertices =
            lz::chain(blob.vertex_span())
                .map([](const vertex_pntt& vsin) {
                    return vertex_pnt{ vsin.position, vsin.normal, vsin.texcoords };
                })
                .to<itlib::small_vector<vertex_pnt, 36>>();

        return basic_mesh{ cubeVertices.data(), cubeVertices.size(), raw_ptr(blob.indices), blob.index_count };
    }();

    random_engine rng{};
    rng.set_float_range(0.0f, 1.0f);

    const vector<per_instance_data> instances =
        lz::chain(lz::range(INSTANCE_COUNT))
            .map([r = &rng](const uint32_t instanceId) {
                const auto xpos = floor(r->next_float() * 200.0f - 100.0f) * 10.0f;
                const auto zpos = floor(r->next_float() * 200.0f - 100.0f) * 10.0f;
                const auto yrot = r->next_float() * two_pi<float>;
                const auto sx = r->next_float() * 50.0f + 10.0f;
                const auto sy = r->next_float() * sx * 8.0f + 8.0f;
                const auto sz = sx;

                per_instance_data data;
                data.tfworld = R4::translate(xpos, 0.0f, zpos) *
                               mat4f{ R3::rotate_y(yrot) * R3::scale_xyz(sx, sy, sz) } *
                               R4::translate(0.0f, 0.5f, 0.0f);
                data.color = rgb_color{ 1.0f - r->next_float() };
                data.texid = instanceId % size(TEXTURES);

                return data;
            })
            .to<vector<per_instance_data>>();

    // auto& water_inst = instances[instances.size() - 1];
    // water_inst.tfworld = mat4f::stdc::identity;
    // water_inst.texid = 0;
    // water_inst.color = color_palette::web::blue_violet;

    scoped_buffer instanceBuffer{ [&instances]() {
        GLuint bufferHandle{};
        gl::CreateBuffers(1, &bufferHandle);
        gl::NamedBufferStorage(bufferHandle, container_bytes_size(instances), instances.data(), 0);
        return bufferHandle;
    }() };

    vertex_program vertexShader =
        gpu_program_builder{}.add_file("shaders/misc/procedural_city/vs.glsl").build<render_stage::e::vertex>();

    if (!vertexShader)
        return nullptr;

    fragment_program fragmentShader =
        gpu_program_builder{}.add_file("shaders/misc/procedural_city/fs.glsl").build<render_stage::e::fragment>();

    if (!fragmentShader)
        return nullptr;

    program_pipeline programPipeline{ []() {
        GLuint pp;
        gl::CreateProgramPipelines(1, &pp);
        return pp;
    }() };

    programPipeline.use_vertex_program(vertexShader).use_fragment_program(fragmentShader);

    scoped_texture textureCollection{};

    gl::CreateTextures(gl::TEXTURE_2D_ARRAY, 1, raw_handle_ptr(textureCollection));
    gl::TextureStorage3D(raw_handle(textureCollection), 1, gl::RGBA8, 1024, 1024, XR_U32_COUNTOF(TEXTURES));

    lz::chain(lz::enumerate(TEXTURES)).forEach([texid = raw_handle(textureCollection)](auto&& instIdWithPath) {
        auto&& [instanceIndex, texturePath] = instIdWithPath;
        texture_loader tex_ldr{ xr_app_config->texture_path(texturePath).c_str() };
        if (!tex_ldr) {
            return;
        }

        gl::TextureSubImage3D(texid,
                              0,
                              0,
                              0,
                              instanceIndex,
                              tex_ldr.width(),
                              tex_ldr.height(),
                              1,
                              tex_ldr.format(),
                              gl::UNSIGNED_BYTE,
                              tex_ldr.data());
    });

    scoped_sampler textureSampler{};
    gl::CreateSamplers(1, raw_handle_ptr(textureSampler));
    gl::SamplerParameteri(raw_handle(textureSampler), gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(raw_handle(textureSampler), gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return xray::base::make_unique<ProceduralCityDemo>(PrivateConstructionToken{},
                                                       initContext,
                                                       RenderState{ std::move(cubeMesh),
                                                                    std::move(instanceBuffer),
                                                                    std::move(vertexShader),
                                                                    std::move(fragmentShader),
                                                                    std::move(programPipeline),
                                                                    std::move(textureCollection),
                                                                    std::move(textureSampler),
                                                                    {}, // default camera
                                                                    fps_camera_controller{ nullptr } });
}

app::simple_fluid::simple_fluid() noexcept
{
    regen_surface(_params);
}

void
app::simple_fluid::parameters::check_numerical_constraints() const noexcept
{
    const float k_speedmax = (cellwidth / 2.0f * timefactor) * sqrt(dampingfactor * timefactor + 2.0f);

    if ((wavespeed < 0.0f) || (wavespeed > k_speedmax)) {
        XR_LOG_DEBUG("Speed constraints not satisfied, waves will diverge "
                     "to infinity");
    }

    const float k0 = wavespeed * wavespeed / cellwidth * cellwidth;
    const float k_timefactormax = (dampingfactor + sqrt(dampingfactor * dampingfactor + (32.0f * k0))) / (8.0f * k0);

    if ((timefactor < 0.0f) || (timefactor > k_timefactormax)) {
        XR_LOG_DEBUG("Time step constraints not satified, waves will diverge "
                     "to infinity");
    }
}

void
app::simple_fluid::parameters::compute_coefficients() noexcept
{
    const float d = cellwidth;
    const float t = timefactor;
    const float c = wavespeed;
    const float mu = dampingfactor;
    const float f1 = c * c * t * t / (d * d);
    const float f2 = 1.0f / (mu * t + 2.0f);

    k1 = (4.0f - 8.0f * f1) * f2;
    k2 = (mu * t - 2.0f) * f2;
    k3 = 2.0f * f1 * f2;
}

void
app::simple_fluid::update(const float delta, app::random_engine& re)
{
    _params.disturb_delta += delta;

    if (_params.disturb_delta > _params.min_disturb_delta) {
        //
        // Disturb at random location;
        const int32_t rmin{ 2 };
        const int32_t rmax = std::min(_params.xquads, _params.zquads) - 2;

        re.set_integer_range(rmin, rmax);

        const auto row_idx = re.next_int();
        const auto col_idx = re.next_int();

        disturb(row_idx, col_idx, _params.wavemagnitude);
        _params.disturb_delta = 0.0f;
    }

    _params.delta += delta;
    if (_params.delta < _params.timefactor) {
        return;
    }

    _params.delta = 0.0f;

    const auto k_num_rows = _params.zquads - 1;
    const auto k_num_cols = _params.xquads - 1;

    //
    // Grid width
    const auto gw = _params.xquads;
    for (int32_t row_idx = 1; row_idx < k_num_rows; ++row_idx) {
        //
        // Solution at t1 (current solution)
        const auto st1 = _current_sol + row_idx * gw;
        //
        // Solution at t0 (previous solution). This gets overwritten and becomes
        // the solution at t1.
        auto st0 = _prev_sol + row_idx * gw;

        for (int32_t col_idx = 1; col_idx < k_num_cols; ++col_idx) {
            //
            // Index of computed element
            const auto k_ec = col_idx;

            //
            // First term in the ecuation.
            const float et0 = _params.k1 * st1[k_ec].position.y;
            //
            // Second termn of the ecuation
            const float et1 = _params.k2 * st0[k_ec].position.y;

            //
            // Element at (row + 1, column) index
            const auto ki0 = k_ec + gw;
            //
            // Element at (row - 1, column) index
            const auto ki1 = k_ec - gw;
            //
            // Element at (row, column -1) index
            const auto ki2 = k_ec - 1;
            //
            // Element at (row, column + 1) index
            const auto ki3 = k_ec + 1;

            //
            // Third term in the ecuation.
            const float et2 =
                _params.k3 * (st1[ki0].position.y + st1[ki1].position.y + st1[ki2].position.y + st1[ki3].position.y);

            st0[k_ec].position.y = et0 + et1 + et2;
        }
    }

    //
    // Current solution buffer.
    const auto cs = _current_sol;
    auto out = _prev_sol;

    for (int32_t row_idx = 1; row_idx < k_num_rows; ++row_idx) {
        for (int32_t col_idx = 1; col_idx < k_num_cols; ++col_idx) {
            //
            // Index of neighbour (row - 1, col)
            const int32_t ki0 = (row_idx - 1) * gw + col_idx;
            //
            // Index of neighbour (row + 1, col)
            const int32_t ki1 = (row_idx + 1) * gw + col_idx;
            //
            // Index of neighbour (row, col - 1)
            const int32_t ki2 = row_idx * gw + col_idx - 1;
            //
            // Index of neightbour (row, col + 1)
            const int32_t ki3 = row_idx * gw + col_idx + 1;

            const float nx = cs[ki0].position.y - cs[ki1].position.y;
            const float ny = 2 * _params.cellwidth;
            const float nz = cs[ki2].position.y - cs[ki3].position.y;

            //
            // Index of the vertex for wich we need to compute the normal.
            const int32_t idx = row_idx * gw + col_idx;
            out[idx].normal.x = nx;
            out[idx].normal.y = ny;
            out[idx].normal.z = nz;
            out[idx].normal = normalize(out[idx].normal);
        }
    }

    copy(out, out + _vertexpool.size() / 2, begin(_mesh.vertices()));
    _mesh.update_vertices();

    std::swap(_prev_sol, _current_sol);
}

void
app::simple_fluid::regen_surface(const parameters& p)
{
    {
        geometry_data_t blob;
        geometry_factory::grid(
            p.cellwidth * (float)p.xquads, p.cellwidth * (float)p.zquads, (size_t)p.zquads, (size_t)p.xquads, &blob);

        transform(raw_ptr(blob.geometry),
                  raw_ptr(blob.geometry) + blob.vertex_count,
                  back_inserter(_vertexpool),
                  [](const vertex_pntt& vin) {
                      return vertex_pnt{ vin.position, vin.normal, vin.texcoords };
                  });

        _mesh = basic_mesh{
            _vertexpool.data(), _vertexpool.size(), raw_ptr(blob.indices), blob.index_count, mesh_type::writable
        };

        if (!_mesh) {
            return;
        }

        _vertexpool.resize(_vertexpool.size() * 2u);
        _current_sol = &_vertexpool[0];
        _prev_sol = &_vertexpool[1];
    }

    gl::CreateTextures(gl::TEXTURE_2D_ARRAY, 1, raw_handle_ptr(_fluid_tex));
    gl::TextureStorage3D(raw_handle(_fluid_tex), 1, gl::RGBA8, 32u, 32u, 1u);
    const uint8_t FLUID_COLOR[] = { 0u, 0u, 255u, 255u };
    gl::ClearTexSubImage(raw_handle(_fluid_tex), 0, 0, 0, 0, 32u, 32u, 1, gl::RGBA, gl::UNSIGNED_BYTE, FLUID_COLOR);
}

void
app::simple_fluid::disturb(const int32_t row_idx, const int32_t col_idx, const float wave_magnitude)
{
    assert((col_idx > 1) && (col_idx < (_params.xquads - 1)));
    assert((row_idx > 1) && (row_idx < (_params.zquads - 1)));

    const auto k_num_cols = _params.xquads;
    const auto kMainVertex = col_idx * k_num_cols + row_idx;
    const auto kVertexNeighbour1 = col_idx * k_num_cols + row_idx - 1;
    const auto kVertexNeighbour2 = (col_idx + 1) * k_num_cols + row_idx - 1;
    const auto kVertexNeighbour3 = (col_idx + 1) * k_num_cols + row_idx;
    const auto kVertexNeighbour4 = (col_idx + 1) * k_num_cols + row_idx + 1;
    const auto kVertexNeighbour5 = (col_idx)*k_num_cols + row_idx + 1;
    const auto kVertexNeighbour6 = (col_idx - 1) * k_num_cols + row_idx + 1;
    const auto kVertexNeighbour7 = (col_idx - 1) * k_num_cols + row_idx;
    const auto kVertexNeighbour8 = (col_idx - 1) * k_num_cols + row_idx - 1;

    const struct vertex_perturb_data
    {
        int32_t vertex_idx;
        float magnitude;
    } perturbed_vertices[] = { { kMainVertex, wave_magnitude },
                               { kVertexNeighbour1, 0.5f * wave_magnitude },
                               { kVertexNeighbour2, 0.5f * wave_magnitude },
                               { kVertexNeighbour3, 0.5f * wave_magnitude },
                               { kVertexNeighbour4, 0.5f * wave_magnitude },
                               { kVertexNeighbour5, 0.5f * wave_magnitude },
                               { kVertexNeighbour6, 0.5f * wave_magnitude },
                               { kVertexNeighbour7, 0.5f * wave_magnitude },
                               { kVertexNeighbour8, 0.5f * wave_magnitude } };

    for (size_t idx = 0; idx < XR_COUNTOF(perturbed_vertices); ++idx) {
        _current_sol[perturbed_vertices[idx].vertex_idx].position.y += perturbed_vertices[idx].magnitude;
    }
}
