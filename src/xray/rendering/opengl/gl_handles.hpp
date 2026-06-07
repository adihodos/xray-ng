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
#include <opengl/opengl.hpp>

namespace xray {
namespace rendering {

struct gl_object_base
{
    using handle_type = GLuint;

    static bool is_null(const handle_type handle) noexcept { return handle == 0; }

    static handle_type null() noexcept { return 0; }
};

struct buffer_handle : public gl_object_base
{
    static void destroy(const handle_type handle) noexcept { gl::DeleteBuffers(1, &handle); }
};

struct vertex_array_handle : public gl_object_base
{
    static void destroy(const handle_type handle) noexcept { gl::DeleteVertexArrays(1, &handle); }
};

struct texture_handle : public gl_object_base
{
    static void destroy(const handle_type tex_handle) noexcept
    {
        if (tex_handle)
            gl::DeleteTextures(1, &tex_handle);
    }
};

struct sampler_handle : public gl_object_base
{
    static void destroy(const handle_type smp_handle) noexcept
    {
        if (smp_handle)
            gl::DeleteSamplers(1, &smp_handle);
    }
};

struct framebuffer_handle : public gl_object_base
{
    static void destroy(const handle_type fbo) noexcept
    {
        if (fbo)
            gl::DeleteFramebuffers(1, &fbo);
    }
};

struct renderbuffer_handle : public gl_object_base
{
    static void destroy(const handle_type rbo) noexcept
    {
        if (rbo)
            gl::DeleteRenderbuffers(1, &rbo);
    }
};

using scoped_buffer = base::unique_handle<buffer_handle>;
using scoped_vertex_array = base::unique_handle<vertex_array_handle>;
using scoped_texture = base::unique_handle<texture_handle>;
using scoped_sampler = base::unique_handle<sampler_handle>;
using scoped_framebuffer = base::unique_handle<framebuffer_handle>;
using scoped_renderbuffer = base::unique_handle<renderbuffer_handle>;

} // namespace rendering
} // namespace xray
