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

/// \file point_light_demo.hpp

#pragma once

#include "demo_base.hpp"
#include "init_context.hpp"
#include "light_source.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/xray.hpp"
#include <cstdint>

namespace app {

class point_light_demo : public demo_base
{
  public:
    point_light_demo(const init_context_t* init_ctx);

    ~point_light_demo();

    virtual void draw(const xray::rendering::draw_context_t&) override;
    virtual void update(const float delta_ms) override;
    virtual void event_handler(const xray::ui::window_event& evt) override;
    virtual void compose_ui() override;
    virtual void poll_start(const xray::ui::poll_start_event& ps) override;
    virtual void poll_end(const xray::ui::poll_end_event& pe) override;

  private:
    void init();

  private:
    xray::rendering::scoped_buffer _vertices;
    xray::rendering::scoped_buffer _indices;
    xray::rendering::scoped_buffer _instance_data_ssbo;
    xray::rendering::scoped_vertex_array _vao;
    xray::rendering::vertex_program _vs;
    xray::rendering::fragment_program _fs;
    xray::rendering::program_pipeline _pipeline;
    xray::rendering::scoped_sampler _sampler;

    struct
    {
        point_light_source light;
    } _scene;

    struct
    {
        xray::math::vec3f lightpos{ 0.0f, 10.0f, 0.0f };
        float specular_intensity{ 10.0f };
        bool rotate_x{ false };
        bool rotate_y{ true };
        bool rotate_z{ false };
        bool drawnormals{ false };
        float rotate_speed{ 0.01f };
    } _demo_opts;

  private:
    XRAY_NO_COPY(point_light_demo);
};

} // namespace app
