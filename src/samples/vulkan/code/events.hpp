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
//

#pragma once

#include <cstdint>
#include "xray/ui/events.hpp"

namespace xray {
namespace base {
class MemoryArena;
class ConfigSystem;
}

namespace scene {
class camera_controller;
class camera;
struct SceneResources;
struct SceneDefinition;
} // namespace scene

namespace ui {
class user_interface;
}

#if defined(XRAY_GRAPHICS_API_VULKAN)
namespace rendering {
struct FrameRenderData;
class VulkanRenderer;
class DebugDrawSystem;
} // namespace rendering
#endif

} // namespace xray

namespace B5 {

struct FrameGlobalData;

struct RenderEvent
{
    xray::ui::window_loop_event loop_event;
    const xray::rendering::FrameRenderData* frame_data;
    xray::rendering::VulkanRenderer* renderer;
    xray::ui::user_interface* ui;
    FrameGlobalData* g_ubo_data;
    xray::rendering::DebugDrawSystem* dbg_draw;
    xray::scene::SceneDefinition* sdef;
    xray::scene::SceneResources* sres;
    float delta;
    xray::base::MemoryArena* arena_perm;
    xray::base::MemoryArena* arena_temp;
};

struct InitContext
{
    int32_t surface_width;
    int32_t surface_height;
    xray::base::MemoryArena* perm;
    xray::base::MemoryArena* temp;
    xray::rendering::VulkanRenderer* renderer;
    xray::base::ConfigSystem* config_sys;
    xray::scene::SceneDefinition* scene_def;
    xray::scene::SceneResources* scene_res;
};

}
