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

/// \file directional_light_demo.hpp

#pragma once

#include "xray/xray.hpp"
#include "demo_base.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/geometry/surface_normal_visualizer.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include <cstdint>

namespace app {

struct directional_light {
  xray::rendering::rgb_color ka;
  xray::rendering::rgb_color kd;
  xray::rendering::rgb_color ks;
  xray::math::vec3f          direction;
  float                      _pad;
};

struct obj_type {
  enum { ripple, teapot };
};

struct graphics_object {
  xray::rendering::basic_mesh* mesh{};
  xray::math::vec3f            pos{xray::math::vec3f::stdc::zero};
  xray::math::vec3f            rotation{xray::math::vec3f::stdc::zero};
  float                        scale{1.0f};
};

class directional_light_demo : public demo_base {
public:
  directional_light_demo(const init_context_t& init_ctx);

  ~directional_light_demo();

  virtual void event_handler(const xray::ui::window_event& evt) override;
  virtual void loop_event(const xray::ui::window_loop_event&) override;
  virtual void poll_start(const xray::ui::poll_start_event&) override;
  virtual void poll_end(const xray::ui::poll_end_event&) override;

private:
  void init();
  void draw();
  void draw_ui(const int32_t surface_w, const int32_t surface_h);
  void update(const float delta_ms);
  void switch_mesh(const char* mesh_path);

private:
  xray::base::timer_highp                    _timer;
  xray::rendering::surface_normal_visualizer _drawnormals{};
  xray::rendering::basic_mesh                _meshes[2];
  graphics_object                            _objects[2];
  xray::rendering::vertex_program            _vs;
  xray::rendering::fragment_program          _fs;
  xray::rendering::program_pipeline          _pipeline;
  xray::rendering::scoped_texture            _objtex[2];
  xray::rendering::scoped_sampler            _sampler;
  directional_light                          _lights[1];

  struct {
    xray::math::vec3f          lightdir{0.0f, -1.0f, 0.0f};
    xray::math::vec3f          kd{};
    xray::math::vec3f          ks{};
    xray::rendering::rgb_color kd_main{
      xray::rendering::color_palette::material::blue300};
    float specular_intensity{10.0f};
    bool  rotate_x{false};
    bool  rotate_y{true};
    bool  rotate_z{false};
    bool  drawnormals{false};
    float rotate_speed{0.01f};
  } _demo_opts;

  struct {
    xray::scene::camera                             cam;
    xray::scene::camera_controller_spherical_coords cam_control{&cam};
  } _scene;

private:
  XRAY_NO_COPY(directional_light_demo);
};

} // namespace app
