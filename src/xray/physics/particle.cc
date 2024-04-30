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

#include "xray/physics/particle.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3_math.hpp"

using namespace xray::math;
using namespace xray::physics;

void
xray::physics::particle::compute_loads(const float drag_coefficient, const vec3f wind_dir, const float wind_speed)
{
    forces = vec3f::stdc::zero;

    if (collided) {
        forces += impact_forces;
        return;
    }

    forces += gravity;

    const vec3f vdrag{ -normalize(velocity) };
    const auto fdrag = 0.5f * air_density<float> * speed * speed * (pi<float> * radius * radius) * drag_coefficient;
    forces += vdrag * fdrag;

    const auto fwind =
        0.5f * air_density<float> * wind_speed * wind_speed * (pi<float> * radius * radius) * drag_coefficient;

    forces += fwind * wind_dir;
}

void
xray::physics::particle::update_body_euler(const float dt)
{
    const auto a = forces / mass;
    const auto dv = a * dt;
    velocity += dv;

    const auto ds = velocity * dt;
    position += ds;

    speed = length(velocity);
}
