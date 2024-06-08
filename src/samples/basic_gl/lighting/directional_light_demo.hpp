//
// Copyright (c) 2011-2017 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// \file directional_light_demo.hpp

#pragma once

#include "xray/xray.hpp"

#include <bitset>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <itlib/small_vector.hpp>

#include "demo_base.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/file_system_watcher.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/geometry/surface_normal_visualizer.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/math/scalar4x4.hpp"

namespace app {

struct DirectionalLight
{
    xray::rendering::rgb_color ka;
    xray::rendering::rgb_color kd;
    xray::rendering::rgb_color ks;
    xray::math::vec3f direction;
    float shininess{ 20.0f };
    xray::math::vec3f half_vector;
    float strength{ 20.0f };
};

struct GraphicsObjectId
{
    enum
    {
        Ripple,
        Teapot
    };
};

struct GraphicsObject
{
    enum class StateBits : uint8_t
    {
        Visible,
        DrawBoundingBox,
        DrawSurfaceNormals,
        RotateX,
        RotateY,
        RotateZ
    };

    xray::rendering::basic_mesh* mesh{};
    xray::math::vec3f pos{ xray::math::vec3f::stdc::zero };
    xray::math::vec3f rotation{ xray::math::vec3f::stdc::zero };
    xray::math::mat4f world_matrix;
    float scale{ 1.0f };
    std::bitset<8> state_bits{ 0b1 };
    uint32_t material{ 0 };
    bool cull_faces{ true };
};

class DirectionalLightDemo : public DemoBase
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    ~DirectionalLightDemo();

    virtual void event_handler(const xray::ui::window_event& evt) override;
    virtual void loop_event(const RenderEvent& render_event) override;

    static std::string_view short_desc() noexcept { return "Directional lighting."; }
    static std::string_view detailed_desc() noexcept { return "Demonstrates the use of directional lighting."; }
    static xray::base::unique_pointer<DemoBase> create(const init_context_t& initContext);

  private:
    enum class SwitchMeshOpt
    {
        LoadFromFile,
        UseExisting
    };

    void draw();
    void draw_ui(const int32_t surface_w, const int32_t surface_h);
    void update(const float delta_ms);
    void switch_mesh(const SwitchMeshOpt switchMeshOpt);

    void filesys_event_handler(const xray::base::FileSystemEvent& fs_event);

  private:
    struct RenderState
    {
        xray::rendering::surface_normal_visualizer drawNormals;
        itlib::small_vector<xray::rendering::basic_mesh, 4> geometryObjects;
        itlib::small_vector<GraphicsObject, 4> graphicsObjects;
        xray::rendering::vertex_program vs;
        xray::rendering::fragment_program fs;
        xray::rendering::program_pipeline pipeline;
        itlib::small_vector<xray::rendering::scoped_texture> objectTextures;
        xray::rendering::scoped_sampler sampler;
        xray::rendering::RenderDebugDraw mDebugDraw;
    } mRenderState;

    struct DemoOptions
    {
        xray::math::vec3f lightdir{ -1.0f, -1.0f, -1.0f };
        xray::math::vec3f ka{ static_cast<xray::math::vec3f>(xray::rendering::color_palette::web::light_gray) };
        xray::math::vec3f kd{ static_cast<xray::math::vec3f>(xray::rendering::color_palette::web::white) };
        xray::math::vec3f ks{ static_cast<xray::math::vec3f>(xray::rendering::color_palette::web::silver) };
        xray::rendering::rgb_color kd_main{ xray::rendering::color_palette::material::blue300 };
        bool rotate_x{ false };
        bool rotate_y{ true };
        bool rotate_z{ false };
        bool drawnormals{ false };
        float rotate_speed{ 0.01f };
        float normal_len{ 0.5f };
    } mDemoOptions;

    struct ModelInfo
    {
        std::filesystem::path path;
        std::string description;
    };

    struct Scene
    {
        xray::scene::camera cam;
        xray::scene::camera_controller_spherical_coords cam_control{
            "config/lighting/directional_light/cam_controller_spherical.conf"
        };
        std::vector<ModelInfo> modelFiles;
        int32_t current_mesh_idx{};
        std::vector<DirectionalLight> lights{};
        xray::base::timer_highp timer;
        std::string debugOutput;
        uint32_t lightsCount{ 4 };

        static constexpr const uint32_t kMaxLightsCount{ 8 };
    } mScene;

    struct RasterizerState
    {
        xray::rendering::ScopedGlCap mEnableDepthTesting{ gl::DEPTH_TEST, xray::rendering::GlCapabilityState::On };
        xray::rendering::ScopedGlCap mEnableFaceCulling{ gl::CULL_FACE, xray::rendering::GlCapabilityState::On };
        xray::rendering::ScopedCullFaceMode mCullBackFaces{ gl::BACK };
        xray::rendering::ScopedWindingOrder mWindingCW{ gl::CW };
    } mRasterizerState{};

    xray::base::FileSystemWatcher mFsWatcher{};

  public:
    DirectionalLightDemo(PrivateConstructionToken,
                         const init_context_t& ctx,
                         RenderState&& rs,
                         const DemoOptions& ds,
                         Scene&& s);

  private:
    void adjust_model_transforms();

    XRAY_NO_COPY(DirectionalLightDemo);
};

} // namespace app
