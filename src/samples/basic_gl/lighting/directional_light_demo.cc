#include "lighting/directional_light_demo.hpp"

#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/syscall_wrapper.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar3_string_cast.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar3x3_string_cast.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/user_interface.hpp"

#include <opengl/opengl.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <itertools.hpp>
#include <pipes/pipes.hpp>
#include <rfl.hpp>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <numeric>
#include <queue>
#include <random>
#include <span>
#include <system_error>
#include <variant>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::ConfigSystem* xr_app_config;

app::DirectionalLightDemo::DirectionalLightDemo(PrivateConstructionToken,
                                                const init_context_t& ctx,
                                                RenderState&& rs,
                                                const DemoOptions& ds,
                                                Scene&& s)
    : DemoBase{ ctx }
    , mRenderState{ std::move(rs) }
    , mDemoOptions{ ds }
    , mScene{ std::move(s) }
{
    mRenderState.geometryObjects >>= pipes::transform([](basic_mesh& bm) {
        return GraphicsObject{
            &bm, xray::math::vec3f::stdc::zero, xray::math::vec3f::stdc::zero, mat4f::stdc::identity, 1.0f
        };
    }) >>= pipes::push_back(mRenderState.graphicsObjects);

    mRenderState.graphicsObjects[0].cull_faces = false;
    mRenderState.graphicsObjects[1].material = 1;

    mScene.cam.set_projection(perspective_symmetric(
        static_cast<float>(ctx.surface_width) / static_cast<float>(ctx.surface_height), radians(70.0f), 0.1f, 100.0f));

    switch_mesh(SwitchMeshOpt::UseExisting);
    mScene.debugOutput.reserve(1024);

    mFsWatcher.add_watch(
        ctx.cfg->shader_root(), make_delegate(*this, &DirectionalLightDemo::filesys_event_handler), true);

    _ui->set_global_font("Roboto-Regular");
    mScene.timer.start();
}

app::DirectionalLightDemo::~DirectionalLightDemo() {}

vector<app::DirectionalLight>
make_lights()
{
    //
    // generate 4 initial light directions from this vector
    const vec3f startingDir{ -1.0f, -1.0f, -1.0f };
    const float rotation_step{ radians(90.0f) };

    itlib::small_vector<vec3f, 8> lightsDirections = iter::range(4) >>= pipes::transform([=](const int quadrant) {
        const float rotation_angle = static_cast<float>(quadrant) * rotation_step;

        const mat3f rotationMtx{ R3::rotate_y(rotation_angle) };
        const vec3f newDir{ mul_vec(rotationMtx, startingDir) };

        XR_LOG_INFO("light vector {:3.3f}", newDir);

        return normalize(newDir);
    }) >>= pipes::to_<itlib::small_vector<vec3f, 8>>();

    // negate the initial 4 directions
    for (size_t idx = 0, numLights = lightsDirections.size(); idx < numLights; ++idx) {
        lightsDirections.push_back(-lightsDirections[idx]);
    }

    mt19937 randEngine{ 0xdeadbeef };
    uniform_int_distribution<uint8_t> rngDistrib{ 0, 127 };

    struct KaKdKs
    {
        rgb_color ka;
        rgb_color kd;
        rgb_color ks;
    };

    itlib::small_vector<KaKdKs> lightColors = lightsDirections >>= pipes::transform([&](const vec3f&) mutable {
        const uint8_t r{ static_cast<uint8_t>(rngDistrib(randEngine) + 127) };
        const uint8_t g{ static_cast<uint8_t>(rngDistrib(randEngine) + 127) };
        const uint8_t b{ static_cast<uint8_t>(rngDistrib(randEngine) + 127) };

        return KaKdKs{ color_palette::web::black,
                       rgb_color{ static_cast<uint8_t>(rngDistrib(randEngine) + 127),
                                  static_cast<uint8_t>(rngDistrib(randEngine) + 127),
                                  static_cast<uint8_t>(rngDistrib(randEngine) + 127) },

                       color_palette::web::white_smoke

        };
    }) >>= pipes::to_<itlib::small_vector<KaKdKs>>();

    mt19937 randEngineLights{ 0xbeefdead };
    uniform_int_distribution<uint8_t> lightStrength{ 20, 32 };

    vector<app::DirectionalLight> lightsCollection;
    pipes::mux(lightsDirections, lightColors) >>= pipes::transform([&](const vec3f dir, const KaKdKs colors) {
        const auto& [ka, kd, ks] = colors;
        return app::DirectionalLight{
            .ka = ka,
            .kd = kd,
            .ks = kd,
            .direction = dir,
            .shininess = static_cast<float>(lightStrength(randEngineLights)),
            .half_vector = vec3f::stdc::zero,
            .strength = static_cast<float>(lightStrength(randEngineLights)),
        };
    }) >>= pipes::push_back(lightsCollection);

    return lightsCollection;
}

xray::base::unique_pointer<app::DemoBase>
app::DirectionalLightDemo::create(const app::init_context_t& initContext)
{
    namespace fs = std::filesystem;
    queue<fs::path> pathQueue;
    pathQueue.push(xr_app_config->model_root().c_str());

    error_code errCode{};
    assert(fs::is_directory(pathQueue.front(), errCode));

    DirectionalLightDemo::Scene s{};

    while (!pathQueue.empty()) {
        fs::path currentPath{ pathQueue.front() };
        pathQueue.pop();

        auto meshFileFilterFn = [](const fs::path& path) {
            return path.extension().c_str() == std::string_view{ ".bin" };
        };

        if (fs::is_directory(currentPath, errCode)) {
            for (const fs::directory_entry& dirEntry : fs::recursive_directory_iterator{ currentPath }) {
                if (dirEntry.is_directory()) {
                    pathQueue.push(dirEntry.path());
                } else if (dirEntry.is_regular_file() && meshFileFilterFn(dirEntry)) {
                    s.modelFiles.emplace_back(dirEntry.path(), dirEntry.path().stem().string());
                }
            }

            continue;
        }

        if (fs::is_regular_file(currentPath, errCode) && meshFileFilterFn(currentPath)) {
            s.modelFiles.emplace_back(currentPath, currentPath.stem().string());
        }
    }

    s.modelFiles >>= pipes::for_each([](const ModelInfo& mi) { XR_LOG_DEBUG("::: {} :::", mi.path.c_str()); });

    const DemoOptions demoOptions{};
    s.lights = make_lights();

    //
    // geometry
    geometry_data_t blob;
    geometry_factory::grid(16.0f, 16.0f, 128, 128, &blob);

    vector<vertex_pnt> verts;
    blob.vertex_span() >>= pipes::transform([](const vertex_pntt vsin) {
        return vertex_pnt{ vsin.position, vsin.normal, vsin.texcoords };
    }) >>= pipes::push_back(verts);

    const vertex_ripple_parameters rparams{ 1.0f, 3.0f * two_pi<float>, 16.0f, 16.0f };

    vertex_effect::ripple(std::span<vertex_pnt>{ verts }, blob.index_span(), rparams);

    itlib::small_vector<basic_mesh, 4> geometryObjects{};

    geometryObjects.emplace_back(std::span{ verts }, blob.index_span());

    auto viperMdlItr = find_if(cbegin(s.modelFiles), cend(s.modelFiles), [](const ModelInfo& mdl) {
        return string_view{ mdl.path.c_str() }.find("viper") != string_view::npos;
    });

    mesh_loader::load(viperMdlItr != cend(s.modelFiles) ? viperMdlItr->path.c_str() : s.modelFiles.front().path.c_str())
        .map([&geometryObjects](mesh_loader loadedModel) {
            const aabb3f bounding_box{ loadedModel.bounding().axis_aligned_bbox };
            XR_LOG_DEBUG(
                "aabb {:3.3f} x {:3.3f} -> center {:3.3f}", bounding_box.min, bounding_box.max, bounding_box.center());
            geometryObjects.emplace_back(loadedModel);
        });

    vertex_program vertShader = gpu_program_builder{}
                                    .add_file(initContext.cfg->shader_path("lighting/directional.vert").c_str())
                                    .build<render_stage::e::vertex>();

    if (!vertShader)
        return nullptr;

    fragment_program fragShader = gpu_program_builder{}
                                      .add_file(initContext.cfg->shader_path("lighting/directional.frag").c_str())
                                      .build<render_stage::e::fragment>();

    if (!fragShader)
        return nullptr;

    program_pipeline pipeline = program_pipeline{ []() {
        GLuint ppl{};
        gl::CreateProgramPipelines(1, &ppl);
        return ppl;
    }() };

    pipeline.use_vertex_program(vertShader).use_fragment_program(fragShader);

    const tuple<uint32_t, rgb_color> texColorsData[] = { { 256, color_palette::web::dark_orange },
                                                         { 256, color_palette::web::navy } };

    itlib::small_vector<scoped_texture> colorTextures;
    texColorsData >>= pipes::transform([](const tuple<uint32_t, rgb_color>& texData) {
        GLuint newTexture{};

        gl::CreateTextures(gl::TEXTURE_2D, 1, &newTexture);

        const auto [texDimensions, texColor] = texData;

        gl::TextureStorage2D(newTexture, 1, gl::RGBA8, texDimensions, texDimensions);
        gl::ClearTexImage(newTexture, 0, gl::RGBA, gl::FLOAT, texColor.components);

        return scoped_texture{ newTexture };
    }) >>= pipes::push_back(colorTextures);

    scoped_sampler sampler{ []() {
        GLuint samplerId{};
        gl::CreateSamplers(1, &samplerId);

        gl::SamplerParameteri(samplerId, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
        gl::SamplerParameteri(samplerId, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

        return scoped_sampler{ samplerId };
    }() };

    tl::optional<RenderDebugDraw> debugDraw{ RenderDebugDraw::create() };
    if (!debugDraw)
        return nullptr;

    return xray::base::make_unique<DirectionalLightDemo>( // base
        PrivateConstructionToken{},
        initContext,
        // renderstate
        RenderState{ surface_normal_visualizer{},
                     std::move(geometryObjects),
                     {},
                     std::move(vertShader),
                     std::move(fragShader),
                     std::move(pipeline),
                     std::move(colorTextures),
                     std::move(sampler),
                     std::move(*debugDraw.take()) },
        // demo options
        demoOptions,
        // scene
        std::move(s));
}

void
app::DirectionalLightDemo::loop_event(const RenderEvent& render_event)
{
    const xray::ui::window_loop_event& wle = render_event.loop_event;
    mFsWatcher.poll();
    _ui->tick(1.0f / 60.0f);
    mScene.timer.update_and_reset();
    update(mScene.timer.elapsed_millis());
    draw();
    draw_ui(wle.wnd_width, wle.wnd_height);
}

void
app::DirectionalLightDemo::draw()
{
    gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, color_palette::web::black.components);
    gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0);

    struct
    {
        DirectionalLight lights[Scene::kMaxLightsCount];
        uint32_t lightscount;
    } scene_lights;

    mScene.lights >>= pipes::take(mScene.lightsCount) >>= pipes::transform([this](const DirectionalLight& inputLight) {
        DirectionalLight dl{ inputLight };
        dl.direction = normalize(mul_vec(mScene.cam.view(), -inputLight.direction));
        dl.half_vector = normalize(dl.direction + mScene.cam.direction());

        return dl;
    }) >>= pipes::override(scene_lights.lights);
    scene_lights.lightscount = static_cast<uint32_t>(mScene.lightsCount);

    struct VsUniformBuffer
    {
        mat4f world_view_proj;
        mat4f model_matrix;
        mat4f normal_matrix;
        vec3f eye_pos;
    };

    mRenderState.pipeline.use(false);

    mRenderState.graphicsObjects >>=
        pipes::for_each([r = &mRenderState, s = &mScene, &scene_lights](GraphicsObject& obj) {
            if (!obj.state_bits[static_cast<uint8_t>(GraphicsObject::StateBits::Visible)]) {
                return;
            }

            gl::BindVertexArray(obj.mesh->vertex_array());
            gl::BindTextureUnit(0, raw_handle(r->objectTextures[obj.material]));
            gl::BindSampler(0, raw_handle(r->sampler));

            const auto pitch = quaternionf{ obj.rotation.x, vec3f::stdc::unit_x };
            const auto roll = quaternionf{ obj.rotation.z, vec3f::stdc::unit_z };
            const auto yaw = quaternionf{ obj.rotation.y, vec3f::stdc::unit_y };
            const auto object_rotation = rotation_matrix(yaw * pitch * roll);

            obj.world_matrix = R4::translate(obj.pos) * object_rotation * mat4f{ R3::scale(obj.scale) };

            const VsUniformBuffer vertex_shader_transforms = { .world_view_proj =
                                                                   s->cam.projection_view() * obj.world_matrix,
                                                               .model_matrix = obj.world_matrix,
                                                               .normal_matrix = object_rotation,
                                                               .eye_pos = s->cam.origin() };

            r->vs.set_uniform_block("TransformMatrices", vertex_shader_transforms);
            r->vs.flush_uniforms();

            r->fs.set_uniform("DIFFUSE_MAP", 0);
            r->fs.set_uniform_block("LightData", scene_lights);
            r->fs.flush_uniforms();

            if (!obj.cull_faces)
                gl::Disable(gl::CULL_FACE);

            gl::DrawElements(gl::TRIANGLES, obj.mesh->index_count(), gl::UNSIGNED_INT, nullptr);

            if (!obj.cull_faces)
                gl::Enable(gl::CULL_FACE);
        });

    mRenderState.mDebugDraw.draw_coord_sys(vec3f::stdc::zero,
                                           vec3f::stdc::unit_x,
                                           vec3f::stdc::unit_y,
                                           vec3f::stdc::unit_z,
                                           24.0f,
                                           color_palette::web::red,
                                           color_palette::web::green,
                                           color_palette::web::blue);

    mScene.lights >>= pipes::take(mScene.lightsCount) >>=
        pipes::for_each([dbgDraw = &mRenderState.mDebugDraw](const DirectionalLight& dl) {
            dbgDraw->draw_directional_light(dl.direction, 32.0f, dl.kd);
        });

    mRenderState.graphicsObjects >>= pipes::filter([](const GraphicsObject& obj) {
        return obj.state_bits[static_cast<uint8_t>(GraphicsObject::StateBits::Visible)] &&
               (obj.state_bits[static_cast<uint8_t>(GraphicsObject::StateBits::DrawSurfaceNormals)] ||
                obj.state_bits[static_cast<uint8_t>(GraphicsObject::StateBits::DrawBoundingBox)]);
    }) >>= pipes::for_each([cam = &mScene.cam, dbg_draw = &mRenderState.mDebugDraw](const GraphicsObject& obj) {
        if (obj.state_bits[static_cast<uint8_t>(GraphicsObject::StateBits::DrawSurfaceNormals)]) {
            dbg_draw->draw_surface_normals(*obj.mesh,
                                           cam->projection_view() * obj.world_matrix,
                                           color_palette::web::green,
                                           color_palette::web::red,
                                           2.0f);
        }

        if (obj.state_bits[static_cast<uint8_t>(GraphicsObject::StateBits::DrawBoundingBox)]) {
            const aabb3f world_box = xray::math::transform(obj.world_matrix, obj.mesh->aabb());
            dbg_draw->draw_axis_aligned_box(world_box.min, world_box.max, color_palette::web::light_cyan);
        }
    });

    // const vec3f text_box_pts[] = {
    // 	// near
    // 	{-1.0f, -1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {1.0f, -1.0, -1.0f},
    // 	// far
    // 	{-9.0f, -9.0f, 9.0f}, {-9.0f, 9.0f, 9.0f}, {9.0f, 9.0f, 9.0f},  {9.0f, -9.0f, 9.0f}
    // };
    // mRenderState.mDebugDraw.draw_box(std::span{text_box_pts}, color_palette::web::azure);
    static MatrixWithInvertedMatrixPair4f cam_data;
    static bool doing_ur_mom{ false };
    if (!doing_ur_mom) {
        cam_data = mScene.cam.projection_view_bijection();
        doing_ur_mom = true;
    }

    // const MatrixWithInvertedMatrixPair4f test_cam{look_at({0.0f, 0.0f, -10.0f}, vec3f::stdc::zero,
    // vec3f::stdc::unit_y)}; const MatrixWithInvertedMatrixPair4f test_frustrum{ perspective_symmetric(
    //     1920.0f / 1200.0f, radians(70.0f), 1.0f, 50.0f) };
    //
    // const MatrixWithInvertedMatrixPair4f test{
    // 	.transform = test_frustrum.transform * test_cam.transform,
    // 	.inverted = test_cam.inverted * test_frustrum.inverted
    // };

    // mRenderState.mDebugDraw.draw_frustrum(mScene.cam.projection_view_bijection(), color_palette::web::red);
    mRenderState.mDebugDraw.draw_sphere(vec3f::stdc::zero, 16.0f, color_palette::web::orchid);
    mRenderState.mDebugDraw.draw_grid(
        vec3f::stdc::zero, vec3f::stdc::unit_x, vec3f::stdc::unit_z, 4, 4, 4.0f, color_palette::web::white_smoke);

    mRenderState.mDebugDraw.render(mScene.cam.projection_view());
}

void
app::DirectionalLightDemo::update(const float delta_ms)
{
    mRenderState.graphicsObjects >>= pipes::for_each([opts = &mDemoOptions](GraphicsObject& obj) {
        const initializer_list<GraphicsObject::StateBits> rotations{ GraphicsObject::StateBits::RotateX,
                                                                     GraphicsObject::StateBits::RotateY,
                                                                     GraphicsObject::StateBits::RotateZ };

        for (const GraphicsObject::StateBits state_bit : rotations) {
            const size_t state_idx{ static_cast<uint8_t>(state_bit) };
            if (!obj.state_bits[state_idx])
                continue;

            const size_t idx{ state_idx - static_cast<uint8_t>(GraphicsObject::StateBits::RotateX) };
            obj.rotation.components[idx] += opts->rotate_speed;
            if (obj.rotation.components[idx] > two_pi<float>)
                obj.rotation.components[idx] -= two_pi<float>;
        }
    });

    mScene.cam_control.update_camera(mScene.cam);
    _ui->tick(delta_ms);
}

void
app::DirectionalLightDemo::event_handler(const xray::ui::window_event& evt)
{
    if (is_input_event(evt)) {

        _ui->input_event(evt);
        if (_ui->wants_input()) {
            return;
        }

        if (evt.type == event_type::key) {
            if (evt.event.key.keycode == KeySymbol::backspace) {
                mDemoOptions = {};
                return;
            }

            if (evt.event.key.keycode == KeySymbol::insert) {
                // reset mesh scale, pos, rot, etc
                adjust_model_transforms();
            }

            if (evt.event.key.keycode == KeySymbol::escape) {
                _quit_receiver();
                return;
            }
        }

        mScene.cam_control.input_event(evt);
        return;
    }

    if (evt.type == event_type::configure) {
        mScene.cam.set_projection(perspective_symmetric(static_cast<float>(evt.event.configure.width) /
                                                            static_cast<float>(evt.event.configure.height),
                                                        radians(70.0f),
                                                        0.1f,
                                                        100.0f));

        return;
    }
}
enum class StateBits
{
    Doing,
    Ur,
    Mom
};

void
app::DirectionalLightDemo::draw_ui(const int32_t surface_w, const int32_t surface_h)
{
    _ui->new_frame(surface_w, surface_h);

    // ImGui::ShowDemoWindow();

    ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Appearing);

    if (ImGui::Begin("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {

        if (ImGui::CollapsingHeader("Objects", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            mRenderState.graphicsObjects >>= pipes::for_each([](GraphicsObject& obj) {
                ImGui::PushID(&obj);

                ImGui::Text("::: Object :::");
                ImGui::Separator();

                rfl::get_underlying_enumerator_array<GraphicsObject::StateBits>() >>=
                    pipes::for_each([&obj](const pair<string_view, uint8_t> enum_name_id_pair) {
                        const auto [state_name, state_idx] = enum_name_id_pair;

                        bool state_bit{ obj.state_bits[state_idx] };
                        if (ImGui::Checkbox(state_name.data(), &state_bit)) {
                            obj.state_bits[state_idx] = state_bit;
                        }
                    });

                if (ImGui::Button("Reset rotation"))
                    obj.rotation = vec3f::stdc::zero;

                ImGui::Separator();
                char dbg_txt[512] = {};
                fmt::format_to_n(dbg_txt,
                                 size(dbg_txt),
                                 "position: {:3.3f}, rotation: {:3.3f}, scale: {:3.3f}\n",
                                 obj.pos,
                                 obj.rotation,
                                 obj.scale);

                const rgb_color txt_color{ color_palette::web::lime };
                ImGui::TextColored(ImVec4{ txt_color.r, txt_color.g, txt_color.b, txt_color.a }, "%s", dbg_txt);
                ImGui::Separator();

                ImGui::PopID();
            });
        }

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            if (ImGui::Combo(
                    "Select a model",
                    &mScene.current_mesh_idx,
                    [](void* p, int32_t idx, const char** out) {
                        auto obj = static_cast<DirectionalLightDemo*>(p);
                        *out = obj->mScene.modelFiles[idx].description.c_str();
                        return true;
                    },
                    this,
                    static_cast<int32_t>(mScene.modelFiles.size()),
                    5)) {
                switch_mesh(SwitchMeshOpt::LoadFromFile);
            }

            ImGui::Separator();
            ImGui::SliderFloat("Scale", &mRenderState.graphicsObjects[GraphicsObjectId::Teapot].scale, 0.1f, 10.0f);

            ImGui::Separator();
            if (ImGui::ColorPicker3(
                    "Object diffuse color", mDemoOptions.kd_main.components, ImGuiColorEditFlags_NoAlpha)) {
                gl::ClearTexImage(raw_handle(mRenderState.objectTextures[GraphicsObjectId::Teapot]),
                                  0,
                                  gl::RGBA,
                                  gl::FLOAT,
                                  mDemoOptions.kd_main.components);
            }
        }

        if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_Framed)) {
            mScene.lights >>= pipes::filter([](const DirectionalLight& dl) {
                return ImGui::TreeNodeEx(&dl, ImGuiTreeNodeFlags_CollapsingHeader, "Light ");
            }) >>= pipes::for_each([](DirectionalLight& dl) mutable {
                ImGui::SliderFloat3("Direction", dl.direction.components, -1.0f, +1.0f);
                ImGui::Separator();

                ImGui::ColorPicker3("Ambient (ka)", dl.ka.components, ImGuiColorEditFlags_NoAlpha);
                ImGui::Separator();

                ImGui::ColorPicker3("Diffuse (kd)", dl.kd.components, ImGuiColorEditFlags_NoAlpha);
                ImGui::Separator(); //

                ImGui::ColorPicker3("Specular (ks)", dl.ks.components, ImGuiColorEditFlags_NoAlpha);
                ImGui::Separator();

                ImGui::SliderFloat("Specular intensity", &dl.shininess, 1.0f, 256.0f);
                ImGui::Separator();

                ImGui::SliderFloat("Light intensity", &dl.strength, 1.0f, 256.0f);
            });
        }

        int32_t lightsCount = (int32_t)mScene.lightsCount;
        if (ImGui::SliderInt("Lights count:", &lightsCount, 0, Scene::kMaxLightsCount)) {
            mScene.lightsCount = static_cast<uint32_t>(lightsCount);
        }

        if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_Framed)) {
            ImGui::Separator();

            mScene.debugOutput.clear();
            mRenderState.graphicsObjects >>= pipes::for_each([this](const GraphicsObject& graphicsObj) mutable {
                fmt::format_to(back_inserter(mScene.debugOutput),
                               "position: {:3.3f}, rotation: {:3.3f}, scale: {:3.3f}\n",
                               graphicsObj.pos,
                               graphicsObj.rotation,
                               graphicsObj.scale);
            });
            ImGui::TextUnformatted(mScene.debugOutput.c_str());
        }
    }

    ImGui::End();

    _ui->draw();
}

void
app::DirectionalLightDemo::switch_mesh(const SwitchMeshOpt switchMeshOpt)
{
    if (switchMeshOpt == SwitchMeshOpt::LoadFromFile) {
        basic_mesh loaded_mesh{ mScene.modelFiles[mScene.current_mesh_idx].path.c_str() };
        if (!loaded_mesh) {
            XR_LOG_DEBUG("Failed to create mesh!");
            return;
        }
        mRenderState.geometryObjects[GraphicsObjectId::Teapot] = std::move(loaded_mesh);

        XR_LOG_DEBUG("Loaded model {}", mScene.modelFiles[mScene.current_mesh_idx].path.c_str());
        XR_LOG_DEBUG("AABB center {}", mRenderState.graphicsObjects[GraphicsObjectId::Teapot].mesh->aabb().center());
    }

    adjust_model_transforms();
}

void
app::DirectionalLightDemo::adjust_model_transforms()
{
    GraphicsObject* teapotObj = &mRenderState.graphicsObjects[GraphicsObjectId::Teapot];
    const float teapotBoundingSphRadius = teapotObj->mesh->bounding_sphere().radius;

    const GraphicsObject* rippleObj = &mRenderState.graphicsObjects[GraphicsObjectId::Ripple];
    const float rippleBoundingSphRadius = rippleObj->mesh->bounding_sphere().radius;

    const float k = rippleBoundingSphRadius / teapotBoundingSphRadius;

    teapotObj->scale = 1.0f;
    // teapotObj->pos = vec3f{ 0.0f, rippleObj->mesh->aabb().height() * 0.5f + k * teapotBoundingSphRadius, 0.0f };
    teapotObj->pos = vec3f::stdc::zero;
    // -teapotObj->mesh->aabb().center();
    teapotObj->rotation = vec3f::stdc::zero;
}

void
app::DirectionalLightDemo::filesys_event_handler(const xray::base::FileSystemEvent& fs_event)
{
    if (const FileModifiedEvent* e = get_if<FileModifiedEvent>(&fs_event)) {
        if (e->file.extension() == ".vert") {
            vertex_program prg = gpu_program_builder{}.add_file(e->file.c_str()).build<render_stage::e::vertex>();
            if (prg) {
                XR_LOG_INFO("::: reloaded vertex program - file {} :::", e->file.string());
                mRenderState.vs = std::move(prg);
                mRenderState.pipeline.use_vertex_program(mRenderState.vs);
            }

            return;
        }

        if (e->file.extension() == ".frag") {
            fragment_program prg = gpu_program_builder{}.add_file(e->file.c_str()).build<render_stage::e::fragment>();
            if (prg) {
                XR_LOG_INFO("::: reloaded fragment program - file {} ::: ", e->file.string());
                mRenderState.fs = std::move(prg);
                mRenderState.pipeline.use_fragment_program(mRenderState.fs);
            }
            return;
        }
    }
}
