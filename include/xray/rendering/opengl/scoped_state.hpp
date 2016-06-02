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

#include "xray/xray.hpp"
#include <opengl/opengl.hpp>

namespace xray {
namespace rendering {

struct scoped_triangle_winding {
public:
  explicit scoped_triangle_winding(const GLint new_winding) noexcept {
    gl::GetIntegerv(gl::FRONT_FACE, &_old_winding);
    gl::FrontFace(new_winding);
  }

  ~scoped_triangle_winding() { gl::FrontFace(_old_winding); }

private:
  GLint _old_winding{};

private:
  XRAY_NO_COPY(scoped_triangle_winding);
  XRAY_NO_MOVE(scoped_triangle_winding);
};

struct scoped_vertex_array_binding {
public:
  explicit scoped_vertex_array_binding(const GLuint vertex_array) noexcept {
    gl::GetIntegerv(gl::VERTEX_ARRAY_BINDING, &_old_binding);
    gl::BindVertexArray(vertex_array);
  }

  ~scoped_vertex_array_binding() { gl::BindVertexArray(_old_binding); }

private:
  GLint _old_binding{};

private:
  XRAY_NO_COPY(scoped_vertex_array_binding);
  XRAY_NO_MOVE(scoped_vertex_array_binding);
};

struct scoped_element_array_binding {
public:
  explicit scoped_element_array_binding(const GLuint elem_arr) noexcept {
    gl::GetIntegerv(gl::ELEMENT_ARRAY_BUFFER_BINDING, &_old_binding);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, elem_arr);
  }

  ~scoped_element_array_binding() {
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, _old_binding);
  }

private:
  GLint _old_binding{};

private:
  XRAY_NO_COPY(scoped_element_array_binding);
  XRAY_NO_MOVE(scoped_element_array_binding);
};

//struct scoped_

} // namespace rendering
} // namespace xray
