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
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include <cstdint>

namespace app {

class terrain_demo : public demo_base {
public:
  terrain_demo(const init_context_t& init_ctx);

  ~terrain_demo();

  virtual void event_handler(const xray::ui::window_event& evt) override;
  virtual void loop_event(const xray::ui::window_loop_event&) override;

private:
  void init();
  void draw(const float surface_width, const float surface_height);
  void draw_ui(const int32_t surface_width, const int32_t surface_height);

private:
  struct terrain_consts {
    static constexpr const uint32_t VERTICES_X = 512;
    static constexpr const uint32_t VERTICES_Z = 512;
    static constexpr const float    TILE_WIDTH = 256.0f;
    static constexpr const float    TILE_DEPTH = 256.0f;
    static constexpr const int32_t  NUM_TILES  = 3;
  };

  xray::rendering::scoped_buffer    _per_instance_heightmap;
  xray::rendering::scoped_buffer    _per_instance_transforms;
  xray::rendering::basic_mesh       _mesh;
  xray::rendering::vertex_program   _vs;
  xray::rendering::fragment_program _fs;
  xray::rendering::program_pipeline _pipeline;
  xray::rendering::scoped_texture   _colormap;
  xray::rendering::scoped_sampler   _sampler;

  xray::scene::camera                             _camera;
  xray::scene::camera_controller_spherical_coords _camcontrol{
    &_camera, "config/misc/terrain/cam_controller_spherical.conf"};

  struct terrain_opts {
    int32_t width{513};
    int32_t height{513};
    float   scaling{16.0f};
    bool    wireframe{true};
  } _terrain_opts;

private:
  XRAY_NO_COPY(terrain_demo);
};

} // namespace app
