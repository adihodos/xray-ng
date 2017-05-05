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

/// \file mesh_demo.hpp

#pragma once

#include "xray/xray.hpp"
#include "demo_base.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include <cstdint>

namespace app {

class bufferless_draw_demo : public demo_base {
public:
  bufferless_draw_demo();

  ~bufferless_draw_demo();

  virtual void draw(const xray::rendering::draw_context_t&) override;
  virtual void update(const float delta_ms) override;
  virtual void event_handler(const xray::ui::window_event& evt) override;
  virtual void compose_ui() override;

private:
  void init();

private:
  xray::rendering::scoped_buffer       _vb;
  xray::rendering::scoped_vertex_array _vao;
  xray::rendering::vertex_program      _vs;
  xray::rendering::geometry_program    _gs;
  xray::rendering::fragment_program    _fs;
  xray::rendering::program_pipeline    _pipeline;

private:
  XRAY_NO_COPY(bufferless_draw_demo);
};

} // namespace app
