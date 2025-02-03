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

#include "xray/scene/camera.controller.flight.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"

namespace xray::scene {
void
FlightCamera::update(const math::mat4f& rotation, const math::vec3f& translation) noexcept
{
    const math::vec3f ideal_cam_pos = math::mul_point(rotation, params.position_relative_to_object) + translation;

    const math::vec3f cam_velocity = (ideal_cam_pos - position) * params.follow_bias;
    position += cam_velocity;

    using math::X, math::Y, math::Z;
    const math::vec3f up_vec = math::mul_vec(rotation, math::vec3f::stdc::unit_y);
    const math::vec3f look_at =
        math::mul_vec(rotation, math::vec3f::stdc::unit_z) * params.lookahead_factor + translation;

    const auto [view_matrix, inverse_view] = math::look_at(position, look_at, up_vec);
    this->view_matrix = view_matrix;
    this->inverse_view = inverse_view;
}

}
