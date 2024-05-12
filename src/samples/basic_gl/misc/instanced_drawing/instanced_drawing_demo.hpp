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

#include <cstdint>
#include <tl/optional.hpp>
#include <vector>

#include "demo_base.hpp"
#include "init_context.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/random.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/scene/fps_camera_controller.hpp"
#include "xray/ui/user_interface.hpp"

namespace app {

class SimpleWorld
{
  public:
    void draw(const xray::scene::camera* cam);
    void update(const float delta_ms);

    static tl::optional<SimpleWorld> create(const init_context_t& init_ctx);

    xray::rendering::basic_mesh _world;
    xray::rendering::vertex_program _vs;
    xray::rendering::fragment_program _fs;
    xray::rendering::program_pipeline _pp;
    xray::rendering::scoped_texture _heightmap;
    xray::rendering::scoped_sampler _sampler;
    xray::math::vec2ui32 _worldsize;
};

class InstancedDrawingDemo : public demo_base
{
  public:
    ~InstancedDrawingDemo();

    virtual void event_handler(const xray::ui::window_event& evt) override;
    virtual void loop_event(const xray::ui::window_loop_event&) override;

    static std::string_view short_desc() noexcept { return "Instanced drawing demo."; }
    static std::string_view detailed_desc() noexcept { return "Instanced drawing demo."; }
    static tl::optional<demo_bundle_t> create(const init_context_t& initContext);

  private:
    void compose_ui(const int32_t surface_width, const int32_t surface_height);

  private:
    struct instance_info
    {
        static constexpr auto rotation_speed = 0.01f;

        float roll{};
        float pitch{};
        float yaw{};
        float scale{};
        float speed{};
        xray::math::vec3f position;
        uint32_t texture_id{};
    };

    struct InstanceXData
    {
        xray::math::mat4f worldViewProjection;
        xray::math::mat4f object2World;
    };

    struct InstancingState
    {
        static constexpr uint32_t instance_count{ 32u };
        std::vector<instance_info> instances;
        xray::rendering::scoped_buffer buffer_transforms;
        std::vector<InstanceXData> scratch_buffer{ 32u };
    } _obj_instances;

    struct RenderState
    {
        xray::rendering::scoped_buffer _vertices;
        xray::rendering::scoped_buffer _indices;
        xray::rendering::scoped_buffer _indirect_draw_cmd_buffer;
        xray::rendering::scoped_vertex_array _vertexarray;
        xray::rendering::vertex_program _vs;
        xray::rendering::fragment_program _fs;
        xray::rendering::scoped_program_pipeline_handle _pipeline;
        xray::rendering::scoped_texture _textures;
        xray::rendering::scoped_sampler _sampler;
        xray::math::vec2i32 _window_size;
        xray::base::timer_highp _timer;
        SimpleWorld _world;
    } _render_state;

    struct Scene
    {
        xray::scene::camera camera;
        xray::scene::fps_camera_controller cam_control{};
    } _scene;

    struct demo_options
    {
        int32_t instance_count{ InstancingState::instance_count };
    } _demo_opts;

  private:
    InstancedDrawingDemo(const init_context_t& init_ctx, RenderState rs, InstancingState ins);
};

} // namespace app
