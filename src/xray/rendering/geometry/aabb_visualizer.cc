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

#include "xray/rendering/geometry/aabb_visualizer.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include <opengl/opengl.hpp>

using namespace xray::math;

xray::rendering::aabb_visualizer::aabb_visualizer()
{
    gl::CreateBuffers(1, raw_handle_ptr(_vb));
    gl::NamedBufferStorage(raw_handle(_vb), 4, nullptr, 0);

    gl::CreateVertexArrays(1, raw_handle_ptr(_vao));
    gl::VertexArrayVertexBuffer(raw_handle(_vao), 0, raw_handle(_vb), 0, 4);
    gl::VertexArrayAttribFormat(raw_handle(_vao), 0, 1, gl::FLOAT, gl::FALSE_, 0);
    gl::VertexArrayAttribBinding(raw_handle(_vao), 0, 0);

    _vs = gpu_program_builder{}.add_file("shaders/draw_aabb/vs.glsl").build<render_stage::e::vertex>();

    if (!_vs)
        return;

    _gs = gpu_program_builder{}.add_file("shaders/draw_aabb/gs.glsl").build<render_stage::e::geometry>();

    if (!_gs)
        return;

    _fs = gpu_program_builder{}.add_file("shaders/draw_aabb/fs.pass.glsl").build<render_stage::e::fragment>();

    if (!_fs)
        return;

    _pipeline = program_pipeline{ []() {
        GLuint pp{};
        gl::CreateProgramPipelines(1, &pp);
        return pp;
    }() };

    _pipeline.use_vertex_program(_vs).use_geometry_program(_gs).use_fragment_program(_fs);

    _valid = true;
}

void
xray::rendering::aabb_visualizer::draw(const xray::rendering::draw_context_t& ctx,
                                       const xray::math::aabb3f& boundingbox,
                                       const xray::rendering::rgb_color& draw_color,
                                       const float line_width)
{

    assert(valid());

    gl::BindVertexArray(raw_handle(_vao));

    struct
    {
        mat4f world_view_proj;
        rgb_color line_start;
        rgb_color line_end;
        vec3f center;
        float _pad;
        float width;
        float height;
        float depth;
    } box_params = { ctx.proj_view_matrix * R4::translate(boundingbox.center()),
                     draw_color,
                     draw_color,
                     boundingbox.center(),
                     0.0f,
                     boundingbox.width() * 0.5f,
                     boundingbox.height() * 0.5f,
                     boundingbox.depth() * 0.5f };

    _gs.set_uniform_block("DrawParams", box_params);
    _pipeline.use();

    //  scoped_line_width_setting lw{line_width};
    gl::DrawArrays(gl::POINTS, 0, 1);
}
