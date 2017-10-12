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

/// \file instanced_drawing_demo.hpp

#pragma once

#include "xray/xray.hpp"
#include "demo_base.hpp"
#include "init_context.hpp"
#include "light_source.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/scene/fps_camera_controller.hpp"
#include <cstdint>
#include <vector>

namespace app {

class geometric_shapes_demo : public demo_base {
public:
  geometric_shapes_demo(const init_context_t* init_ctx);

  ~geometric_shapes_demo();

  virtual void draw(const xray::rendering::draw_context_t&) override;
  virtual void update(const float delta_ms) override;
  virtual void event_handler(const xray::ui::window_event& evt) override;
  virtual void compose_ui() override;

private:
  void init();

private:
  struct instance_info {
    static constexpr auto rotation_speed = 0.01f;

    float                      roll{};
    float                      pitch{};
    float                      yaw{};
    float                      scale{};
    float                      speed{};
    xray::math::vec3f          position;
    xray::rendering::rgb_color color;
    uint32_t                   texture_id{};
  };

  struct gpu_instance_info {
    xray::math::mat4f          world_view_proj;
    xray::math::mat4f          view;
    xray::math::mat4f          normals;
    xray::rendering::rgb_color color;
    uint32_t                   tex_id;
    uint32_t                   _pad[3];
  };

  struct object_instances {
    static constexpr uint32_t      instance_count{64u};
    std::vector<instance_info>     instances;
    xray::rendering::scoped_buffer buffer_transforms;
    xray::rendering::scoped_buffer buffer_texture_ids;
  } _obj_instances;

  struct render_state {
    uint32_t                             index_count{};
    uint32_t                             index_count_sphere{};
    xray::rendering::scoped_buffer       vertices_sphere;
    xray::rendering::scoped_buffer       indices_sphere;
    xray::rendering::scoped_buffer       vertices;
    xray::rendering::scoped_buffer       indices;
    xray::rendering::scoped_buffer       indirect_draw_cmd_buffer;
    xray::rendering::scoped_buffer       instances_ssbo;
    xray::rendering::scoped_buffer       draw_ids;
    xray::rendering::scoped_vertex_array vertexarray;
    xray::rendering::vertex_program      vs;
    xray::rendering::vertex_program      vs_instancing;
    xray::rendering::geometry_program    gs;
    xray::rendering::fragment_program    fs;
    xray::rendering::program_pipeline    pipeline;
    xray::rendering::scoped_texture      textures;
    xray::rendering::scoped_sampler      sampler;
  } _render;

  struct {
    xray::scene::camera camera;
    //    xray::scene::fps_camera_controller cam_control{&camera};
    xray::scene::camera_controller_spherical_coords cam_control{
      &camera, "config/cam_controller_spherical.conf"};
    light_source lights[4];
  } _scene;

private:
  XRAY_NO_COPY(geometric_shapes_demo);
};

} // namespace app
