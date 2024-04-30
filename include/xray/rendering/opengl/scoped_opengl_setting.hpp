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

/// \file aabb_visualizer.hpp

#pragma once

#include "xray/xray.hpp"
#include <cstdint>
#include <opengl/opengl.hpp>

namespace xray {
namespace rendering {

struct scoped_polygon_mode_setting
{
  public:
    explicit scoped_polygon_mode_setting(const int32_t new_mode) noexcept
    {
        gl::GetIntegerv(gl::POLYGON_MODE, &_old_mode);
        gl::PolygonMode(gl::FRONT_AND_BACK, (GLenum)new_mode);
    }

    ~scoped_polygon_mode_setting() noexcept { gl::PolygonMode(gl::FRONT_AND_BACK, (GLenum)_old_mode); }

  private:
    GLint _old_mode{ gl::NONE };
    XRAY_NO_COPY(scoped_polygon_mode_setting);
};

struct scoped_line_width_setting
{
  public:
    explicit scoped_line_width_setting(const float new_width) noexcept
    {
        gl::GetFloatv(gl::LINE_WIDTH, &_old_width);
        gl::LineWidth(new_width);
    }

    ~scoped_line_width_setting() noexcept { gl::LineWidth(_old_width); }

  private:
    float _old_width{};
    XRAY_NO_COPY(scoped_line_width_setting);
};

struct scoped_winding_order_setting
{
  public:
    explicit scoped_winding_order_setting(const GLint new_winding) noexcept
    {
        gl::GetIntegerv(gl::FRONT_FACE, &_old_setting);
        gl::FrontFace(new_winding);
    }

    ~scoped_winding_order_setting() noexcept { gl::FrontFace(_old_setting); }

  private:
    GLint _old_setting{};
    XRAY_NO_COPY(scoped_winding_order_setting);
};

} // namespace rendering
} // namespace xray
