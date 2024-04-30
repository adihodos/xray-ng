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

#include "xray/base/unique_handle.hpp"
#include "xray/xray.hpp"
#include <cstring>
#include <opengl/opengl.hpp>

namespace xray {
namespace rendering {

inline void
mark_gl_object(const GLenum type, const GLuint obj, const char* label) noexcept
{
    gl::ObjectLabel(type, obj, strlen(label), label);
}

#define GL_MARK_BUFFER(obj, label)                                                                                     \
    do {                                                                                                               \
        mark_gl_object(gl::BUFFER, obj, label);                                                                        \
    } while (0)

#define GL_MARK_FRAMEBUFFER(obj, label)                                                                                \
    do {                                                                                                               \
        mark_gl_object(gl::FRAMEBUFFER, obj, label);                                                                   \
    } while (0)

#define GL_MARK_PROGRAM_PIPELINE(obj, label)                                                                           \
    do {                                                                                                               \
        mark_gl_object(gl::PROGRAM_PIPELINE, obj, label);                                                              \
    } while (0)

#define GL_MARK_PROGRAM(obj, label)                                                                                    \
    do {                                                                                                               \
        mark_gl_object(gl::PROGRAM, obj, label);                                                                       \
    } while (0)

#define GL_MARK_TEXTURE(obj, label)                                                                                    \
    do {                                                                                                               \
        mark_gl_object(gl::TEXTURE, obj, label);                                                                       \
    } while (0)

#define GL_MARK_VERTEXARRAY(obj, label)                                                                                \
    do {                                                                                                               \
        mark_gl_object(gl::VERTEX_ARRAY, obj, label);                                                                  \
    } while (0)

} // namespace rendering
} // namespace xray
