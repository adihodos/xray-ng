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

#include <cstdint>
#include <random>

#include <tl/optional.hpp>

#include "demo_base.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/geometry/surface_normal_visualizer.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/fps_camera_controller.hpp"
#include "xray/xray.hpp"

namespace app {

class random_engine
{
  public:
    random_engine() {}

    void set_float_range(const float minval, const float maxval) noexcept
    {
        _f32distribution = std::uniform_real_distribution<float>{ minval, maxval };
    }

    void set_integer_range(const int32_t minval, const int32_t maxval) noexcept
    {
        _i32distribution = std::uniform_int_distribution<int32_t>{ minval, maxval };
    }

    float next_float() noexcept { return _f32distribution(_engine); }
    int32_t next_int() noexcept { return _i32distribution(_engine); }

  private:
    std::random_device _device{};
    std::mt19937 _engine{ _device() };
    std::uniform_real_distribution<float> _f32distribution{ 0.0f, 1.0f };
    std::uniform_int_distribution<int32_t> _i32distribution{ 0, 1 };
};

class simple_fluid
{
  public:
    simple_fluid() noexcept;

    struct parameters
    {
        float cellwidth{ 0.8f };
        float delta{ 0.0f };
        float timefactor{ 0.05f };
        float wavespeed{ 3.25f };
        float dampingfactor{ 0.4f };
        float wavemagnitude{ 0.935f };
        float min_disturb_delta{ 2.5f };
        float disturb_delta{ 0.0f };
        float k1{};
        float k2{};
        float k3{};
        int32_t xquads{ 255 };
        int32_t zquads{ 255 };

        void check_numerical_constraints() const noexcept;
        void compute_coefficients() noexcept;
    } _params{};

    explicit operator bool() const noexcept { return static_cast<bool>(_mesh); }

    void update(const float delta_ms, random_engine& re);

    void regen_surface(const parameters& p);

    void disturb(const int32_t row_idx, const int32_t col_idx, const float wave_magnitude);

    void draw(const xray::rendering::draw_context_t&);

    const xray::rendering::basic_mesh& mesh() const noexcept { return _mesh; }
    GLuint texture() const noexcept { return xray::base::raw_handle(_fluid_tex); }

  private:
    xray::rendering::basic_mesh _mesh;
    std::vector<xray::rendering::vertex_pnt> _vertexpool;
    xray::rendering::vertex_pnt* _current_sol{};
    xray::rendering::vertex_pnt* _prev_sol{};
    xray::rendering::scoped_texture _fluid_tex{};
};

class procedural_city_demo : public demo_base
{
  public:
    ~procedural_city_demo();

    virtual void event_handler(const xray::ui::window_event& evt) override;
    virtual void loop_event(const xray::ui::window_loop_event&) override;
    virtual void poll_start(const xray::ui::poll_start_event&) override;
    virtual void poll_end(const xray::ui::poll_end_event&) override;

    static std::string_view short_desc() noexcept { return "Procedural city."; }

    static std::string_view detailed_desc() noexcept { return "Procedurally generated city."; }

    static tl::optional<demo_bundle_t> create(const init_context_t& initContext);

  private:
    void init();
    void draw_ui(const xray::ui::window_loop_event& loopEvent);
	void draw(const xray::ui::window_loop_event& loopEvent);

  private:
    struct RenderState
    {
        // xray::rendering::surface_normal_visualizer _drawnormals{};
        xray::rendering::basic_mesh _mesh;
        xray::rendering::scoped_buffer _instancedata;
        xray::rendering::vertex_program _vs;
        xray::rendering::fragment_program _fs;
        xray::rendering::program_pipeline _pipeline;
        xray::rendering::scoped_texture _objtex;
        xray::rendering::scoped_sampler _sampler;
        xray::scene::camera _camera;
        xray::scene::fps_camera_controller _camcontrol;
        // random_engine _rand{};
        // simple_fluid _fluid{};
    } mRenderState;

    struct DemoOptions
    {
        bool draw_fluid{ true };
        bool freeze_fluid{ false };
        bool draw_buildings{ true };
    } mDemoOptions;

	struct RasterizerState {
		xray::rendering::ScopedGlCap mEnabledDepthTesting{gl::DEPTH_TEST, xray::rendering::GlCapabilityState::On};
		xray::rendering::ScopedGlCap mEnableFaceCulling{gl::CULL_FACE, xray::rendering::GlCapabilityState::On};
		xray::rendering::ScopedCullFaceMode mCullBackFaces{gl::BACK};
	} mRasterizerState{};

  private:
    procedural_city_demo(const init_context_t& initCtx, RenderState&& renderState);
    XRAY_NO_COPY(procedural_city_demo);
};

} // namespace app
