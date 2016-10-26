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
#include "xray/math/scalar3.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/scene/point_light.hpp"

namespace app {

class edge_detect_demo : public demo_base {
public:
  enum { edge_detect_none, edge_detect_sobel, edge_detect_frei_chen };

  edge_detect_demo(const init_context_t& init_ctx);

  ~edge_detect_demo();

  void compose_ui();

  virtual void draw(const xray::rendering::draw_context_t& draw_ctx) override;

  virtual void update(const float delta_ms) override;

  virtual void key_event(const int32_t key_code, const int32_t action,
                         const int32_t mods) override;

  virtual void resize_event(const resize_context_t& rctx) override;

  explicit operator bool() const noexcept { return valid(); }

private:
  void init(const init_context_t& init_ctx);
  void create_framebuffer(const GLsizei r_width, const GLsizei r_height);

  enum { max_lights = 8u };

private:
  struct fbo_data {
    xray::rendering::scoped_framebuffer  fbo_object;
    xray::rendering::scoped_renderbuffer fbo_depth;
    xray::rendering::scoped_texture      fbo_texture;
    xray::rendering::scoped_sampler      fbo_sampler;
  } _fbo;

  xray::rendering::simple_mesh       _object;
  xray::rendering::mesh_graphics_rep _obj_graphics;
  xray::rendering::simple_mesh       _fsquad;
  xray::rendering::mesh_graphics_rep _fsquad_graphics;
  xray::rendering::scoped_sampler    _sampler_obj;
  xray::rendering::scoped_texture    _obj_material;
  xray::rendering::scoped_texture    _obj_diffuse_map;
  xray::scene::point_light           _lights[edge_detect_demo::max_lights];
  uint32_t                           _lightcount{2};
  float                              _mat_spec_pwr{50.0f};
  xray::rendering::vertex_program    _vertex_prg;
  xray::rendering::fragment_program  _frag_prg;
  xray::rendering::vertex_program    _vs_edge_detect;
  xray::rendering::fragment_program  _fs_edge_detect;
  xray::rendering::scoped_program_pipeline_handle _prog_pipeline;
  float                                           _edge_treshold{0.1f};
  uint32_t _detect_method{edge_detect_none};
  bool     _rotate_object{false};
  float    _rx{0.0f};
  float    _ry{0.0f};
  float    _rz{0.0f};

private:
  XRAY_NO_COPY(edge_detect_demo);
};

} // namespace app
