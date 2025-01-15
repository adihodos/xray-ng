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

#pragma once

#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"

namespace xray {

namespace ui {
struct window_event;
} // namespace ui

namespace scene {

class camera;

/// Adapted from: https://github.com/Twinklebear/arcball/blob/master/src/lib.rs
struct ArcballCamera
{
    xray::math::mat4f translation{ xray::math::mat4f::stdc::identity };
    xray::math::mat4f center_translation{ xray::math::mat4f::stdc::identity };
    xray::math::quatf rotation{ xray::math::quatf::stdc::identity };
    xray::math::mat4f view_transform{ xray::math::mat4f::stdc::identity };
    xray::math::mat4f inverse_view_transform{ xray::math::mat4f::stdc::identity };
    xray::math::vec2f inv_screen{ 1.0f, 1.0f };
    xray::math::vec2f prev_mouse{ 0.0f, 0.0f };
    float zoom_speed{ 1.0f };
    bool is_rotating{ false };
    bool is_first_rotation{ true };
    bool is_panning{ false };
    bool is_first_panning{ true };

    static constexpr const float INITIAL_FOV = 30.0f;
    static constexpr const float TRANSLATION_FACTOR = 1.0f;

    ArcballCamera() = default;
    ArcballCamera(xray::math::vec3f center, const float zoom_speed, xray::math::vec2i32 screen_size) noexcept;

    void update() noexcept
    {
        this->view_transform = translation * math::rotation_matrix(rotation) * center_translation;
        this->inverse_view_transform = math::invert(view_transform);
    }

    void update_screen(const int32_t width, const int32_t height) noexcept
    {
        inv_screen = math::vec2f{ 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height) };
    }

    void rotate(const math::vec2f mouse_pos) noexcept;
    void end_rotate() noexcept { is_rotating = false; }

    math::quatf screen_to_arcball(const math::vec2f p) noexcept
    {
        const float distance = math::dot(p, p);

        if (distance <= 1.0f) {
            return math::normalize(math::quatf{ 0.0f, p.x, p.y, std::sqrt(1.0f - distance) });
        } else {
            const math::vec2f unit_p = math::normalize(p);
            return math::normalize(math::quatf{ 0.0f, unit_p.x, unit_p.y, 0.0f });
        }
    }

    void zoom(const float amount, const float) noexcept
    {
        const math::vec3f motion{ 0.0f, 0.0f, amount };
        translation = math::R4::translate(motion * zoom_speed) * translation;
        update();
    }

    void set_zoom_speed(const float s) noexcept { zoom_speed = s; }
    void pan(const math::vec2f mouse_cur) noexcept;
    void input_event(const ui::window_event& e) noexcept;
    void update_camera(camera& cam) const noexcept;
};

}
}
