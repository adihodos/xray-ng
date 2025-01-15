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

#include "fwd_app.hpp"
#include "xray/base/delegate.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/ui/events.hpp"
#include "xray/xray.hpp"
#include "xray/base/basic_timer.hpp"

#include <bitset>
#include <cstdint>
#include <tuple>

namespace xray {
namespace scene {
class camera_controller;
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

namespace app {

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
};

class DemoBase
{
  public:
    DemoBase(const init_context_t& init_ctx);

    virtual ~DemoBase();

    virtual void event_handler(const xray::ui::window_event& evt) = 0;
    virtual void loop_event(const RenderEvent&) = 0;

  protected:
    cpp::delegate<void()> _quit_receiver;
    xray::ui::user_interface* _ui;
    std::bitset<256> _keyboard{};
    xray::base::timer_stdp _timer{};
};

} // namespace app
