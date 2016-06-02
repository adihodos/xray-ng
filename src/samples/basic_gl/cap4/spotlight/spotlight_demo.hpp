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
#include "demo_base.hpp"
#include "light_source.hpp"
#include "material.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"

namespace app {

class spotlight_demo : public demo_base {
public:
  static constexpr uint32_t NUM_LIGHTS{3u};

  spotlight_demo();

  virtual ~spotlight_demo();

  virtual void draw(const xray::rendering::draw_context_t&) override;

  virtual void update(const float delta_ms) override;

  virtual void key_event(const int32_t key_code, const int32_t action,
                         const int32_t mods) override;

  explicit operator bool() const noexcept { return valid(); }

private:
  void init();

private:
  struct mesh_draw_data {
    uint32_t             index_offset{};
    uint32_t             index_count{};
    uint32_t             base_vertex{};
    bool                 front_ccw{};
    material             mat{};
    xray::math::float3   translation{xray::math::float3::stdc::zero};
    xray::math::float2   rotation_xy{xray::math::float2::stdc::zero};
  };

  xray::rendering::scoped_buffer _vertex_buffer;
  xray::rendering::scoped_buffer _index_buffer;
  xray::rendering::scoped_vertex_array    _vertex_array_obj;
  xray::rendering::gpu_program          _draw_prog;
  spotlight                             _lights[NUM_LIGHTS];
  mesh_draw_data                        _meshes[2];
  bool                                  _rotate_mesh{true};

private:
  XRAY_NO_COPY(spotlight_demo);
};

} // namespace app
