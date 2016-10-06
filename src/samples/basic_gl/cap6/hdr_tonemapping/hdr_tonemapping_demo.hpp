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
#include "fwd_app.hpp"
#include "xray/base/base_fwd.hpp"
#include "xray/base/containers/fixed_vectoor.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/scene/point_light.hpp"

namespace app {

class hdr_tonemap : public demo_base {
public:
  enum { edge_detect_none, edge_detect_sobel, edge_detect_frei_chen };

  hdr_tonemap(const init_context_t& init_ctx);

  ~hdr_tonemap();

  void compose_ui();

  virtual void draw(const xray::rendering::draw_context_t&) override;

  virtual void update(const float delta_ms) override;

  virtual void key_event(const int32_t key_code, const int32_t action,
                         const int32_t mods) override;

  virtual void resize_event(const resize_context_t& resize_ctx) override;

  explicit operator bool() const noexcept { return valid(); }

private:
  void init(const init_context_t& ini_ctx);
  void create_framebuffer(const GLsizei r_width, const GLsizei r_height);

  enum { MAX_LIGHTS = 8u };
  enum { MAX_OBJECTS = 2u };

private:
  struct fbo_data {
    xray::rendering::scoped_framebuffer  fbo_object;
    xray::rendering::scoped_renderbuffer fbo_depth;
    xray::rendering::scoped_texture      fbo_texture;
    xray::rendering::scoped_sampler      fbo_sampler;
  } _fbo;

  xray::rendering::geometry_object   _obj_geometries[MAX_OBJECTS];
  xray::rendering::mesh_graphics_rep _obj_graphics[MAX_OBJECTS];
  xray::base::fixed_vector<xray::scene::point_light, MAX_LIGHTS>
      _lights[MAX_LIGHTS];

private:
  XRAY_NO_COPY(hdr_tonemap);
};

} // namespace app
