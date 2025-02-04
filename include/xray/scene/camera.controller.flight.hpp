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

#include "xray/math/orientation.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/math.units.hpp"

namespace xray {
namespace ui {
struct window_event;
} // namespace ui

namespace scene {

struct FlightCameraParams
{
    math::vec3f position_relative_to_object{ 0.0f, 1.0f, -2.0f };
    float follow_bias{ 0.05f };
    float lookahead_factor{ 5.0f };
};

class camera;

struct CameraFrame
{
    math::vec3f right;
    math::vec3f up;
    math::vec3f dir;
};

struct FlightCamera
{
    FlightCameraParams params;
    math::vec3f position;
    math::mat4f view_matrix;
    math::mat4f inverse_of_view_matrix;
    float near;
    float far;
    float aspect;
    math::RadiansF32 fovy;
    math::mat4f projection_matrix;
    math::mat4f inverse_of_projection_matrix;

    FlightCamera(const math::RadiansF32 fovy, const float aspect, const float near, const float far) noexcept
    {
        const auto [projection, inverse_projection] = math::perspective_symmetric(aspect, fovy, near, far);

        this->projection_matrix = projection;
        this->inverse_of_projection_matrix = inverse_projection;
        this->near = near;
        this->far = far;
        this->aspect = aspect;
        this->fovy = fovy;
        this->position = math::vec3f::stdc::zero;
    }

    void update(const math::mat4f& rotation, const math::vec3f& translation) noexcept;

    CameraFrame frame() const noexcept
    {
        using math::X, math::Y, math::Z;

        return CameraFrame{
            .right = view_matrix[0].swizzle<X, Y, Z>(),
            .up = view_matrix[1].swizzle<X, Y, Z>(),
            .dir = view_matrix[2].swizzle<X, Y, Z>(),
        };
    }
};

}
}
