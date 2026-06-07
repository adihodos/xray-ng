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

#include "xray/math/scalar3.hpp"
#include "xray/physics/constants.hpp"
#include "xray/xray.hpp"
#include <cstdint>

namespace xray {

namespace math {

template<typename T>
class plane;

} // namespace math

namespace physics {

/// \addtogroup __GroupXrayPhysics
/// @{

class particle
{
  public:
    float mass{ 1.0f };
    float speed{};
    float radius{};
    xray::math::vec3f position{ xray::math::vec3f::stdc::zero };
    xray::math::vec3f velocity{ xray::math::vec3f::stdc::zero };
    xray::math::vec3f forces{ xray::math::vec3f::stdc::zero };
    xray::math::vec3f gravity{ 0.0f, mass* gravity_acceleration<float>, 0.0f };
    xray::math::vec3f previous_pos{ xray::math::vec3f::stdc::zero };
    xray::math::vec3f impact_forces;
    bool collided{ false };

    void set_mass(const float m) noexcept
    {
        mass = m;
        gravity.y = mass * gravity_acceleration<float>;
    }

    void compute_loads(const float drag_coefficient, const xray::math::vec3f wind_dir, const float wind_speed);

    void update_body_euler(const float dt);
};

/// @}

} // namespace physics
} // namespace xray
