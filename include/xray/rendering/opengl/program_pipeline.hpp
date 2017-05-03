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

/// \file program_pipeline.hpp

#pragma once

#include "xray/xray.hpp"
#include "xray/base/unique_handle.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/render_stage.hpp"
#include <cstring>
#include <opengl/opengl.hpp>
#include <utility>

namespace xray {
namespace rendering {

struct gpu_program_pipeline_handle {
  using handle_type = GLuint;

  static bool is_null(const handle_type handle) noexcept { return handle == 0; }

  static void destroy(const handle_type handle) noexcept {
    if (!is_null(handle))
      gl::DeleteProgramPipelines(1, &handle);
  }

  static handle_type null() noexcept { return 0; }
};

using scoped_program_pipeline_handle =
  xray::base::unique_handle<gpu_program_pipeline_handle>;

class program_pipeline {
public:
  program_pipeline() noexcept { memset(_programs, 0, sizeof(_programs)); }

  explicit program_pipeline(const GLuint existing_handle) noexcept
    : _pipeline{existing_handle} {
    memset(_programs, 0, sizeof(_programs));
  }

  program_pipeline(program_pipeline&& rval) noexcept
    : _pipeline{std::move(rval._pipeline)} {
    memcpy(_programs, rval._programs, sizeof(_programs));
  }

  program_pipeline& operator=(program_pipeline&& rval) noexcept {
    _pipeline = std::move(rval._pipeline);
    memcpy(_programs, rval._programs, sizeof(_programs));
    return *this;
  }

  program_pipeline& use_vertex_program(vertex_program& vertex_prg) noexcept {
    return use_stage(
      vertex_prg, render_stage::e::vertex, gl::VERTEX_SHADER_BIT);
  }

  program_pipeline& use_fragment_program(fragment_program& frag_prg) noexcept {
    return use_stage(
      frag_prg, render_stage::e::fragment, gl::FRAGMENT_SHADER_BIT);
  }

  program_pipeline&
  use_geometry_program(geometry_program& geometry_prg) noexcept {
    return use_stage(
      geometry_prg, render_stage::e::geometry, gl::GEOMETRY_SHADER_BIT);
  }

  program_pipeline& disable_stage(const render_stage::e stage) noexcept {
    static constexpr GLbitfield STAGE_TO_BITFIELD[] = {
      gl::VERTEX_SHADER_BIT,
      gl::TESS_CONTROL_SHADER_BIT,
      gl::TESS_EVALUATION_SHADER_BIT,
      gl::GEOMETRY_SHADER_BIT,
      gl::FRAGMENT_SHADER_BIT};

    gl::UseProgramStages(base::raw_handle(_pipeline),
                         STAGE_TO_BITFIELD[render_stage::to_integer(stage)],
                         0);

    _programs[render_stage::to_integer(stage)] = nullptr;
    return *this;
  }

  void use();

private:
  program_pipeline& use_stage(detail::gpu_program_base& prg,
                              const render_stage::e     stage,
                              const GLbitfield          gl_stage) noexcept {
    gl::UseProgramStages(base::raw_handle(_pipeline), gl_stage, prg.handle());
    _programs[render_stage::to_integer(stage)] = &prg;
    return *this;
  }

  scoped_program_pipeline_handle _pipeline;
  detail::gpu_program_base*      _programs[render_stage::size];

  XRAY_NO_COPY(program_pipeline);
};

} // namespace rendering
} // namespace xray
