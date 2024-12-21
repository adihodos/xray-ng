//
// Copyright (c) Adrian Hodos
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

#include "xray/scene/camera.controller.arcball.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar4_math.hpp"
#include "xray/ui/events.hpp"
#include "xray/scene/camera.hpp"

namespace xray::scene {

ArcballCamera::ArcballCamera(math::vec3f center, const float zoom_speed_, math::vec2i32 screen_size) noexcept
    : translation{ math::R4::translate(math::vec3f{ 0.0f, 0.0f, 1.0f }) }
    , center_translation{ math::R4::translate(-center) }
    , rotation{ math::quatf::stdc::identity }
    , view_transform{ math::mat4f::stdc::identity }
    , inverse_view_transform{ math::mat4f::stdc::identity }
    , inv_screen{ 1.0f / static_cast<float>(screen_size.x), 1.0f / static_cast<float>(screen_size.y) }
    , zoom_speed{ zoom_speed_ }
{
    update();
}

void
ArcballCamera::rotate(const math::vec2f mouse_pos) noexcept
{
    const math::vec2f mouse_cur{ math::clamp(mouse_pos.x * 2.0f * inv_screen.x - 1.0f, -1.0f, 1.0f),
                                 math::clamp(1.0f - 2.0f * mouse_pos.y * inv_screen.y, -1.0f, 1.0f) };

    const math::vec2f mouse_prev{ math::clamp(prev_mouse.x * 2.0f * inv_screen.x - 1.0f, -1.0f, 1.0f),
                                  math::clamp(1.0f - 2.0f * prev_mouse.y * inv_screen.y, -1.0f, 1.0f) };

    const math::quatf mouse_cur_ball = screen_to_arcball(mouse_cur);
    const math::quatf mouse_prev_ball = screen_to_arcball(mouse_prev);
    rotation = mouse_cur_ball * mouse_prev_ball * rotation;
    prev_mouse = mouse_pos;
    update();
}

void
ArcballCamera::pan(const math::vec2f mouse_cur) noexcept
{
    const math::vec2f mouse_delta = mouse_cur - prev_mouse;
    const float zoom_dist = std::abs(translation[3].z);
    const math::vec4f delta{ math::vec4f{ mouse_delta.x * inv_screen.x, -mouse_delta.y * inv_screen.y, 0.0f, 0.0f } *
                             zoom_dist };

    const math::vec4f motion = math::mul_vec(inverse_view_transform, delta);
    center_translation = math::R4::translate(math::vec3f{ motion.x, motion.y, motion.z }) * center_translation;
    prev_mouse = mouse_cur;
    update();
}

void
ArcballCamera::input_event(const ui::window_event& e) noexcept
{
    switch (e.type) {
        case ui::event_type::mouse_button: {
            const ui::mouse_button_event* mbe = &e.event.button;

            if (mbe->type == ui::event_action_type::press) {
                if (mbe->button == ui::mouse_button::button2) {
                    is_rotating = true;
                    is_first_rotation = true;
                } else if (mbe->button == ui::mouse_button::button3) {
                    is_panning = true;
                    is_first_panning = true;
                }
            } else {
                if (mbe->button == ui::mouse_button::button2) {
                    is_rotating = false;
                    is_first_rotation = true;
                } else if (mbe->button == ui::mouse_button::button3) {
                    is_panning = false;
                    is_first_panning = true;
                }
            }

        } break;

        case ui::event_type::mouse_motion: {
            const ui::mouse_motion_event* mme = &e.event.motion;

            if (is_rotating) {
                if (is_first_rotation) {
                    prev_mouse = math::vec2f{ (float)mme->pointer_x, (float)mme->pointer_y };
                    is_first_rotation = false;
                } else {
                    rotate(math::vec2f{ (float)mme->pointer_x, (float)mme->pointer_y });
                }
            }

            if (is_panning) {
                if (is_first_panning) {
                    prev_mouse = math::vec2f{ (float)mme->pointer_x, (float)mme->pointer_y };
                    is_first_panning = false;
                } else {
                    pan(math::vec2f{ (float)mme->pointer_x, (float)mme->pointer_y });
                }
            }
        } break;

        case ui::event_type::configure: {
            const ui::window_configure_event* cfg = &e.event.configure;
            update_screen(cfg->width, cfg->height);
        } break;

        case ui::event_type::mouse_wheel: {
            const ui::mouse_wheel_event* mwe = &e.event.wheel;
            zoom(static_cast<float>(mwe->delta), 0.0f);
        } break;

        default:
            break;
    }
}

void
ArcballCamera::update_camera(camera& cam) const noexcept
{
    cam.set_view_matrix({ this->view_transform, this->inverse_view_transform });
}

}
