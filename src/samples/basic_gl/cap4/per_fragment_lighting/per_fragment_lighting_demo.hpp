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
#include "light_source.hpp"
#include "material.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/rendering_fwd.hpp"
#include <cstdint>

namespace app {

class per_fragment_lighting_demo {
public:
  per_fragment_lighting_demo();
  ~per_fragment_lighting_demo();

  void draw(const xray::rendering::draw_context_t& dc) noexcept;
  void update(const float delta_ms) noexcept;
  void key_event(const int32_t key_code, const int32_t action,
                 const int32_t mods) noexcept;

  bool valid() const noexcept { return _valid; }

  explicit operator bool() const noexcept { return valid(); }

private:
  static const uint32_t NUM_LIGHTS = 4;

  void init();

private:
  bool                                 _valid{false};
  uint32_t                             _mesh_index_cnt{};
  xray::rendering::scoped_buffer       _vertex_buff;
  xray::rendering::scoped_buffer       _index_buff;
  xray::rendering::scoped_vertex_array _vertex_arr_obj;
  xray::rendering::gpu_program         _draw_prog;
  directional_light                    _lights[NUM_LIGHTS];
  xray::math::vec2f                    _rotations{0.0f, 0.0f};

private:
  XRAY_NO_COPY(per_fragment_lighting_demo);
};

} // namespace app
