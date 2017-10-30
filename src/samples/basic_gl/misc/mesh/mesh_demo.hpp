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
#include "init_context.hpp"
#include "xray/base/maybe.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/geometry/aabb_visualizer.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/ui/ui.hpp"
#include <cstdint>

namespace app {

class mesh_demo : public demo_base {
public:
  mesh_demo(const init_context_t& init_ctx);

  ~mesh_demo();

  virtual void event_handler(const xray::ui::window_event& evt) override;
  virtual void poll_start(const xray::ui::poll_start_event&) override;
  virtual void poll_end(const xray::ui::poll_end_event&) override;
  virtual void loop_event(const xray::ui::window_loop_event&) override;

private:
  struct mesh_info {
    uint32_t             vertices;
    uint32_t             indices;
    size_t               vertex_bytes;
    size_t               index_bytes;
    xray::math::aabb3f   bbox;
    xray::math::sphere3f bsphere;
  };

  void init();
  void draw(const float surface_width, const float surface_height);
  void compose_ui();
  xray::base::maybe<mesh_info> load_mesh(const char* model_path);

private:
  xray::ui::user_interface                        _ui;
  xray::rendering::aabb_visualizer                _abbdraw;
  xray::rendering::scoped_buffer                  _vb;
  xray::rendering::scoped_buffer                  _ib;
  xray::rendering::scoped_vertex_array            _vao;
  uint32_t                                        _vertexcount{};
  uint32_t                                        _indexcount{};
  xray::rendering::basic_mesh                     _mesh;
  xray::rendering::vertex_program                 _vs;
  xray::rendering::fragment_program               _fs;
  xray::rendering::program_pipeline               _pipeline;
  xray::rendering::scoped_texture                 _objtex;
  xray::rendering::scoped_texture                 _greentexture;
  xray::rendering::scoped_sampler                 _sampler;
  xray::rendering::vertex_program                 _vsnormals;
  xray::rendering::geometry_program               _gsnormals;
  xray::rendering::fragment_program               _fsnormals;
  xray::math::aabb3f                              _bbox;
  xray::scene::camera                             _camera;
  xray::scene::camera_controller_spherical_coords _camcontrol{
    &_camera, "config/misc/mesh_demo/cam_controller_spherical.conf"};
  xray::base::maybe<mesh_info> _mesh_info{xray::base::nothing{}};
  bool                         _mesh_loaded{false};
  int32_t                      _sel_id{-1};

  struct {
    int32_t                    drawnormals{false};
    int32_t                    draw_boundingbox{false};
    int32_t                    draw_wireframe{false};
    xray::rendering::rgb_color start_color{
      xray::rendering::color_palette::web::red};
    xray::rendering::rgb_color end_color{
      xray::rendering::color_palette::web::medium_spring_green};
    float normal_len{0.1f};
  } _drawparams{};

  xray::rendering::vertex_ripple_parameters _rippledata{
    0.6f, 3.0f, 16.0f, 16.0f};

private:
  XRAY_NO_COPY(mesh_demo);
};

} // namespace app
