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

#include "xray/math/scalar2.hpp"
#include "xray/xray.hpp"

#include <tl/optional.hpp>

namespace xray {

namespace ui {
struct window_event;
} // namespace ui

namespace scene {

class camera;

/// \addtogroup __GroupXrayMath
/// @{

class camera_controller_spherical_coords
{
  public:
    camera_controller_spherical_coords(const char* config_file = nullptr);

    void input_event(const ui::window_event& input_evt);

    void update_camera(camera& targetCam);

  private:
    struct controller_params_t
    {
        float radius_{ 15.0f };
        float theta_{ -0.5f };
        float phi_{ 0.5f };
        float phi_max_{ 175.0f };
        float phi_min_{ 5.0f };
        float radius_max_{ 50.0f };
        float radius_min_{ 1.0f };
        float speed_theta_{ 0.1f };
        float speed_phi_{ 0.1f };
        float speed_move_{ 0.2f };
    };

    void mouse_moved(const float x_pos, const float y_pos) noexcept;

  private:
    controller_params_t _params{};
    tl::optional<xray::math::vec2f> _last_mouse_pos{};
};

/// @}

} // namespace scene
} // namespace xray
