//
// Copyright (c) 2011-2016 Adrian Hodos
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

/// \file fractal_demo.hpp

#pragma once

#include "demo_base.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/xray.hpp"

#include <cstdint>
#include <string_view>
#include <tuple>

#include <tl/optional.hpp>

namespace app {

struct fractal_params
{
    float width{ 0.0f };
    float height{ 0.0f };
    float shape_re{ 0.0f };
    float shape_im{ 0.0f };
    uint32_t iterations{ 0 };
};

class fractal_demo : public demo_base
{
	private:
		struct ConstructToken {
		explicit ConstructToken() = default;
	};
	
  public:
    fractal_demo(ConstructToken tok,
				 const init_context_t& ctx,
                 xray::rendering::scoped_buffer vb,
                 xray::rendering::scoped_buffer ib,
                 xray::rendering::scoped_vertex_array layout,
                 xray::rendering::vertex_program vs,
                 xray::rendering::fragment_program fs,
                 xray::rendering::program_pipeline pp)
        : demo_base{ ctx }
        , _quad_vb{ std::move(vb) }
        , _quad_ib{ std::move(ib) }
        , _quad_layout{ std::move(layout) }
        , _vs{ std::move(vs) }
        , _fs{ std::move(fs) }
        , _pipeline{ std::move(pp) }
    {
    }

    ~fractal_demo();

    XRAY_DEFAULT_MOVE(fractal_demo);

    virtual void event_handler(const xray::ui::window_event& evt) override;
    virtual void loop_event(const xray::ui::window_loop_event&) override;

    static std::string_view short_desc() noexcept { return "Julia fractal."; }

    static std::string_view detailed_desc() noexcept
    {
        return "Julia fractal eplorer. Navigate the awesome and mesmerizing Julia "
               "fractal.";
    }

    static tl::optional<demo_bundle_t> create(const init_context_t& initContext);

  private:
    void init();
    void draw(const int32_t surface_w, const int32_t surface_h);
    void draw_ui(const int32_t surface_w, const int32_t surface_h);

  private:
    int32_t _shape_idx{ 0u };
    int32_t _iterations{ 256 };
    fractal_params _fp{};
    xray::rendering::scoped_buffer _quad_vb;
    xray::rendering::scoped_buffer _quad_ib;
    xray::rendering::scoped_vertex_array _quad_layout;
    xray::rendering::vertex_program _vs;
    xray::rendering::fragment_program _fs;
    xray::rendering::program_pipeline _pipeline;

	struct RasterizerState {
		xray::rendering::ScopedGlCap mDisabledDepthTesting{gl::DEPTH_TEST, xray::rendering::GlCapabilityState::Off};
	} mRasterizerState{};

  private:
    XRAY_NO_COPY(fractal_demo);
};

} // namespace app
