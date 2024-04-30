//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
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

#pragma once

#include "demo_base.hpp"
#include "light_source.hpp"
#include "material.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/xray.hpp"

namespace app {

class render_texture_demo : public demo_base
{
  public:
    render_texture_demo();

    ~render_texture_demo();

    void compose_ui();

    virtual void draw(const xray::rendering::draw_context_t&) override;

    virtual void update(const float delta_ms) override;

    virtual void key_event(const int32_t key_code, const int32_t action, const int32_t mods) override;

    explicit operator bool() const noexcept { return valid(); }

  private:
    void init();

  private:
    xray::rendering::scoped_texture _fbo_texture;
    xray::rendering::scoped_sampler _fbo_texture_sampler;
    xray::rendering::scoped_renderbuffer _fbo_depthbuffer;
    xray::rendering::scoped_framebuffer _fbo;
    xray::rendering::scoped_texture _null_texture;
    xray::rendering::simple_mesh _spacecraft_mesh;
    xray::rendering::simple_mesh _cube_mesh;
    xray::rendering::scoped_texture _spacecraft_material;
    xray::rendering::gpu_program _drawprogram;
    xray::rendering::gpu_program _drawprogram_debug;
    float _cube_r_angle{ 0.0f };

    struct debug_options
    {
        bool debug_normals{ false };
        bool draw_wireframe{ false };
        bool draw_main_obj{ true };
        bool object_rotates{ false };
        bool frontface_cw{ false };
        xray::rendering::rgb_color normal_color_beg{ 1.0f, 0.0f, 0.0f, 1.0f };
        xray::rendering::rgb_color normal_color_end{ 0.0f, 1.0f, 0.0f, 1.0f };
        float normal_line_len{ 0.5f };
        xray::math::vec3f light_pos{ 0.0f, 10.0f, 0.0f };
    } _dbg_opts;

    struct render_tex_opts
    {
        xray::math::vec3f cam_pos{ 0.0f, 4.0f, -4.0f };
        xray::math::vec2ui32 rendertarget_size{ 512u, 512u };
        float fov{ 65.0f };
        light_source4 lights[2];
        xray::rendering::rgb_color tex_clear_color;
        xray::rendering::rgb_color scene_clear_color;
    } _rto;

  private:
    XRAY_NO_COPY(render_texture_demo);
};

} // namespace app
