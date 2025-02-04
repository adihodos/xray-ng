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

#pragma once

#include "xray/xray.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/math.units.hpp"

#include <cstdint>
#include <bitset>

namespace xray {

namespace ui {
struct window_event;
} // namespace ui

namespace scene {

class camera;

/// \addtogroup __GroupXrayMath
/// @{

struct camera_lens_parameters
{
    math::RadiansF32 fov{ math::radians(65.0f) };
    float nearplane{ 0.1f };
    float farplane{ 500.0f };
    float aspect_ratio{ 4.0f / 3.0f };
};

class fps_camera_controller
{
  public:
    static constexpr float MOVE_SPEED = 1.0f;
    static constexpr float ROTATE_SPEED = 0.1f;
    static constexpr float LENS_SPEED = 0.05f;

    fps_camera_controller(const char* config_file = nullptr);

    void move_up() noexcept;
    void move_down() noexcept;

    void move_forward() noexcept;
    void move_backward() noexcept;

    void move_left() noexcept;
    void move_right() noexcept;

    void yaw_left() noexcept;
    void yaw_right() noexcept;

    void pitch_up() noexcept;
    void pitch_down() noexcept;

    void roll_left() noexcept;
    void roll_right() noexcept;

    void zoom_in() noexcept;
    void zoom_out() noexcept;

    void set_lens_parameters(const camera_lens_parameters& lparam) noexcept
    {
        _lensparams = lparam;
        _syncstatus.lens = 0;
    }

    void reset_orientation() noexcept;
    void input_event(const ui::window_event& input_evt);
	void process_input(const std::bitset<256>& keyboard);
    void update_camera(camera& cam);

  private:
    void pitch(const float delta) noexcept;
    void yaw(const float delta) noexcept;
    void roll(const float delta) noexcept;

    xray::math::vec3f _right{ xray::math::vec3f::stdc::unit_x };
    xray::math::vec3f _up{ xray::math::vec3f::stdc::unit_y };
    xray::math::vec3f _dir{ xray::math::vec3f::stdc::unit_z };
    xray::math::vec3f _position{ xray::math::vec3f::stdc::zero };
    camera_lens_parameters _lensparams{};

    struct
    {
        float move_speed{ 1.0f };
        float roll_speed{ 0.2f };
        float yaw_speed{ 0.05f };
        float pitch_speed{ 0.05f };
        float lens_speed{ 0.05f };
    } _viewparams{};

    union
    {
        struct
        {
            uint8_t view : 1;
            uint8_t lens : 1;
        };

        uint8_t state{};
    } mutable _syncstatus;
};

/// @}

} // namespace scene
} // namespace xray
