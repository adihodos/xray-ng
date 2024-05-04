#include "lighting/directional_light_demo.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/syscall_wrapper.hpp"
#include "xray/math/constants.hpp"
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

app::DirectionalLightDemo::DirectionalLightDemo(const init_context_t& ctx,
                                                RenderState&& rs,
                                                const DemoOptions& ds,
                                                Scene&& s)
    : demo_base{ ctx }
    , mRenderState{ std::move(rs) }
    , mDemoOptions{ ds }
    , mScene{ std::move(s) }
{
    mRenderState.geometryObjects >>= pipes::transform([](basic_mesh& bm) {
        return graphics_object{ &bm, xray::math::vec3f::stdc::zero, xray::math::vec3f::stdc::zero, 1.0f };
    }) >>= pipes::push_back(mRenderState.graphicsObjects);

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
    const vec3f startingDir{ 1.0f, 1.0f, 1.0f };
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
            .half_vector = vec3f::stdc::zero,
            .shininess = static_cast<float>(lightStrength(randEngineLights)),
            .strength = static_cast<float>(lightStrength(randEngineLights)),
        };
    }) >>= pipes::push_back(lightsCollection);

    return lightsCollection;
}

tl::optional<app::demo_bundle_t>
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
        .map([&geometryObjects](mesh_loader loadedModel) { geometryObjects.emplace_back(loadedModel); });

    vertex_program vertShader = gpu_program_builder{}
                                    .add_file(initContext.cfg->shader_path("lighting/directional.vert").c_str())
                                    .build<render_stage::e::vertex>();

    if (!vertShader)
        return tl::nullopt;

    fragment_program fragShader = gpu_program_builder{}
                                      .add_file(initContext.cfg->shader_path("lighting/directional.frag").c_str())
                                      .build<render_stage::e::fragment>();

    if (!fragShader)
        return tl::nullopt;

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
        return tl::nullopt;

    unique_pointer<DirectionalLightDemo> object{ new DirectionalLightDemo{ // base
                                                                           initContext,
                                                                           // renderstate
                                                                           { surface_normal_visualizer{},
                                                                             move(geometryObjects),
                                                                             {},
                                                                             move(vertShader),
                                                                             move(fragShader),
                                                                             move(pipeline),
                                                                             move(colorTextures),
                                                                             move(sampler),
                                                                             move(*debugDraw.take()) },
                                                                           // demo options
                                                                           demoOptions,
                                                                           // scene
                                                                           move(s) } };

    auto winEventHandler = make_delegate(*object, &DirectionalLightDemo::event_handler);
    auto loopEventHandler = make_delegate(*object, &DirectionalLightDemo::loop_event);

    return tl::make_optional<demo_bundle_t>(std::move(object), winEventHandler, loopEventHandler);
}

void
app::DirectionalLightDemo::loop_event(const xray::ui::window_loop_event& wle)
{
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

    gl::BindTextureUnit(0, raw_handle(mRenderState.objectTextures[obj_type::ripple]));
    gl::BindSampler(0, raw_handle(mRenderState.sampler));

    mRenderState.pipeline.use(false);

    auto ripple = &mRenderState.graphicsObjects[obj_type::ripple];

    assert(ripple->mesh != nullptr);
    gl::BindVertexArray(ripple->mesh->vertex_array());

    const mat4f rippleToWorld{ R4::translate(ripple->pos) };
    struct VsUniformBuffer
    {
        mat4f world_view_proj;
        mat4f view;
        mat4f normals;
    } const rippleVertShaderTransforms = { mScene.cam.projection_view() * rippleToWorld,
                                           mScene.cam.view() * rippleToWorld,
                                           rippleToWorld };

    mRenderState.vs.set_uniform_block("TransformMatrices", rippleVertShaderTransforms);
    mRenderState.fs.set_uniform("DIFFUSE_MAP", 0);

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

    mRenderState.fs.set_uniform_block("LightData", scene_lights);
    mRenderState.vs.flush_uniforms();
    mRenderState.fs.flush_uniforms();

    gl::Disable(gl::CULL_FACE);
    gl::DrawElements(gl::TRIANGLES, ripple->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

    auto teapot = &mRenderState.graphicsObjects[obj_type::teapot];

    if (teapot->mesh) {
        gl::BindVertexArray(teapot->mesh->vertex_array());
        gl::BindTextureUnit(0, raw_handle(mRenderState.objectTextures[obj_type::teapot]));

        const auto pitch = quaternionf{ teapot->rotation.x, vec3f::stdc::unit_x };
        const auto roll = quaternionf{ teapot->rotation.z, vec3f::stdc::unit_z };
        const auto yaw = quaternionf{ teapot->rotation.y, vec3f::stdc::unit_y };
        const auto qrot = yaw * pitch * roll;
        const auto teapot_rot = rotation_matrix(qrot);

        const auto teapot_world = R4::translate(teapot->pos) * teapot_rot * mat4f{ R3::scale(teapot->scale) };

        const VsUniformBuffer teapotVertShaderTransforms = { .world_view_proj =
                                                                 mScene.cam.projection_view() * teapot_world,
                                                             .view = mScene.cam.view() * teapot_world,
                                                             .normals = teapot_rot };

        mRenderState.vs.set_uniform_block("TransformMatrices", teapotVertShaderTransforms);
        mRenderState.vs.flush_uniforms();

        gl::Enable(gl::CULL_FACE);
        gl::DrawElements(gl::TRIANGLES, teapot->mesh->index_count(), gl::UNSIGNED_INT, nullptr);

        if (mDemoOptions.drawnormals) {
            const draw_context_t dc{
                0u, 0u, mScene.cam.view(), mScene.cam.projection(), mScene.cam.projection_view(), &mScene.cam, nullptr
            };

            mRenderState.drawNormals.draw(dc,
                                          *teapot->mesh,
                                          R4::translate(teapot->pos) * teapot_rot,
                                          color_palette::web::green,
                                          color_palette::web::green,
                                          mDemoOptions.normal_len);
        }
    }

    if (mDemoOptions.drawnormals) {
        const draw_context_t dc{
            0u, 0u, mScene.cam.view(), mScene.cam.projection(), mScene.cam.projection_view(), &mScene.cam, nullptr
        };

        mRenderState.drawNormals.draw(dc,
                                      *ripple->mesh,
                                      R4::translate(ripple->pos),
                                      color_palette::web::cadet_blue,
                                      color_palette::web::cadet_blue,
                                      mDemoOptions.normal_len);
    }

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

    mRenderState.mDebugDraw.render(mScene.cam.projection_view());
}

void
app::DirectionalLightDemo::update(const float delta_ms)
{
    auto teapot = &mRenderState.graphicsObjects[obj_type::teapot];

    if (mDemoOptions.rotate_x) {
        teapot->rotation.x += mDemoOptions.rotate_speed;
        if (teapot->rotation.x > two_pi<float>) {
            teapot->rotation.x -= two_pi<float>;
        }
    }

    if (mDemoOptions.rotate_y) {
        teapot->rotation.y += mDemoOptions.rotate_speed;

        if (teapot->rotation.y > two_pi<float>) {
            teapot->rotation.y -= two_pi<float>;
        }
    }

    if (mDemoOptions.rotate_z) {
        teapot->rotation.z += mDemoOptions.rotate_speed;

        if (teapot->rotation.z > two_pi<float>) {
            teapot->rotation.z -= two_pi<float>;
        }
    }

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
            if (evt.event.key.keycode == key_sym::e::backspace) {
                mDemoOptions = {};
                return;
            }

            if (evt.event.key.keycode == key_sym::e::insert) {
                // reset mesh scale, pos, rot, etc
                adjust_model_transforms();
            }

            if (evt.event.key.keycode == key_sym::e::escape) {
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

void
app::DirectionalLightDemo::draw_ui(const int32_t surface_w, const int32_t surface_h)
{
    _ui->new_frame(surface_w, surface_h);

    ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Appearing);

    if (ImGui::Begin("Options",
                     nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoMove)) {

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
            ImGui::SliderFloat("Scale", &mRenderState.graphicsObjects[obj_type::teapot].scale, 0.1f, 10.0f);

            ImGui::Separator();
            if (ImGui::ColorPicker3(
                    "Object diffuse color", mDemoOptions.kd_main.components, ImGuiColorEditFlags_NoAlpha)) {
                gl::ClearTexImage(raw_handle(mRenderState.objectTextures[obj_type::teapot]),
                                  0,
                                  gl::RGBA,
                                  gl::FLOAT,
                                  mDemoOptions.kd_main.components);
            }
        }

        if (ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed)) {
            ImGui::Checkbox("X", &mDemoOptions.rotate_x);
            ImGui::Checkbox("Y", &mDemoOptions.rotate_y);
            ImGui::Checkbox("Z", &mDemoOptions.rotate_z);
            ImGui::SliderFloat("Speed", &mDemoOptions.rotate_speed, 0.001f, 0.5f);
        }

        if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_Framed)) {
            DirectionalLight* dl = &mScene.lights[0];

            ImGui::SliderFloat3("Direction", dl->direction.components, -1.0f, +1.0f);

            ImGui::Separator();

            ImGui::ColorPicker3("Ambient (ka)", dl->ka.components, ImGuiColorEditFlags_NoAlpha);
            ImGui::Separator();

            ImGui::ColorPicker3("Diffuse (kd)", dl->kd.components, ImGuiColorEditFlags_NoAlpha);
            ImGui::Separator();

            ImGui::ColorPicker3("Specular (ks)", dl->ks.components, ImGuiColorEditFlags_NoAlpha);
            ImGui::Separator();

            ImGui::SliderFloat("Specular intensity", &dl->shininess, 1.0f, 256.0f);
            ImGui::Separator();

            ImGui::SliderFloat("Light intensity", &dl->strength, 1.0f, 256.0f);
        }

        int32_t lightsCount = (int32_t)mScene.lightsCount;
        if (ImGui::SliderInt("Lights count:", &lightsCount, 0, Scene::kMaxLightsCount)) {
            mScene.lightsCount = static_cast<uint32_t>(lightsCount);
        }

        if (ImGui::CollapsingHeader("Debug", ImGuiTreeNodeFlags_Framed)) {
            ImGui::Checkbox("Draw surface normals", &mDemoOptions.drawnormals);
            ImGui::Separator();
            ImGui::SliderFloat("Normal length", &mDemoOptions.normal_len, 0.1f, 5.0f);
            ImGui::Separator();

            mScene.debugOutput.clear();
            mRenderState.graphicsObjects >>= pipes::for_each([this](const graphics_object& graphicsObj) mutable {
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
app::DirectionalLightDemo::poll_start(const xray::ui::poll_start_event&)
{
}

void
app::DirectionalLightDemo::poll_end(const xray::ui::poll_end_event&)
{
}

void
app::DirectionalLightDemo::switch_mesh(const SwitchMeshOpt switchMeshOpt)
{
    if (switchMeshOpt == SwitchMeshOpt::LoadFromFile) {
        basic_mesh loaded_mesh{ mScene.modelFiles[mScene.current_mesh_idx].path.c_str() };
        if (!loaded_mesh) {
            XR_DBG_MSG("Failed to create mesh!");
            return;
        }
        mRenderState.geometryObjects[obj_type::teapot] = std::move(loaded_mesh);
    }

    adjust_model_transforms();
}

void
app::DirectionalLightDemo::adjust_model_transforms()
{
    graphics_object* teapotObj = &mRenderState.graphicsObjects[obj_type::teapot];
    const float teapotBoundingSphRadius = teapotObj->mesh->bounding_sphere().radius;

    const graphics_object* rippleObj = &mRenderState.graphicsObjects[obj_type::ripple];
    const float rippleBoundingSphRadius = rippleObj->mesh->bounding_sphere().radius;

    const float k = rippleBoundingSphRadius / teapotBoundingSphRadius;

    teapotObj->scale = k;
    teapotObj->pos = vec3f{ 0.0f, rippleObj->mesh->aabb().height() * 0.5f + k * teapotBoundingSphRadius, 0.0f };
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
                mRenderState.vs = move(prg);
                mRenderState.pipeline.use_vertex_program(mRenderState.vs);
            }

            return;
        }

        if (e->file.extension() == ".frag") {
            fragment_program prg = gpu_program_builder{}.add_file(e->file.c_str()).build<render_stage::e::fragment>();
            if (prg) {
                XR_LOG_INFO("::: reloaded fragment program - file {} ::: ", e->file.string());
                mRenderState.fs = move(prg);
                mRenderState.pipeline.use_fragment_program(mRenderState.fs);
            }
            return;
        }
    }
}
