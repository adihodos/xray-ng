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
#include "xray/base/unique_handle.hpp"
#include <cstdint>
#include <opengl/opengl.hpp>

namespace xray {
namespace rendering {

enum class shader_type : uint8_t {
  vertex,
  tesselation_control,
  tesselation_eval,
  geometry,
  fragment,
  compute
};

struct shader_handle {
  using handle_type = GLuint;

  static bool is_null(const handle_type handle) noexcept { return handle == 0; }

  static void destroy(const handle_type handle) noexcept {
    if (!is_null(handle))
      gl::DeleteShader(handle);
  }

  static handle_type null() noexcept { return 0; }
};

using scoped_shader_handle = xray::base::unique_handle<shader_handle>;

GLuint make_shader(const uint32_t shader_type, const char* const* src_code_strings,
                   const uint32_t strings_count) noexcept;

GLuint make_shader(const uint32_t shader_type,
                   const char*    source_file) noexcept;

} // namespace rendering
} // namespace xray
