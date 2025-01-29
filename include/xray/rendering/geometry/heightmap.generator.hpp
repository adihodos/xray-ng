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

#include <cstdint>
#include <cmath>

#include "xray/base/random.hpp"
#include "xray/math/scalar3.hpp"

namespace xray::rendering {
class HeightmapGenerator
{
  public:
    HeightmapGenerator(void* vertices,
                       const size_t stride,
                       const uint32_t width,
                       const uint32_t height,
                       const float roughness = 255.0f) noexcept
        : _vertexptr{ vertices }
        , _stride{ stride }
        , _width{ width }
        , _height{ height }
        , _roughness{ roughness }
    {
        seed();
        smooth_terrain(32);
        smooth_terrain(128);
    }

  private:
    float make_point(const float x, const float z) noexcept
    {
        return _rng.next_float(0.0f, 1.0f) > 0.3f ? (std::abs(std::sin(x * z) * _roughness)) : 0.0f;
    }

    math::vec3f* point_at(const uint32_t x, const uint32_t y) noexcept
    {
        return (math::vec3f*)(((uint8_t*)_vertexptr) + (y * _width + x) * _stride);
    }

    const math::vec3f* point_at(const uint32_t x, const uint32_t y) const noexcept
    {
        return (math::vec3f*)(((const uint8_t*)_vertexptr) + (y * _width + x) * _stride);
    }

    float get_point(const uint32_t x, const uint32_t z) const noexcept
    {
        const auto xp = (x + _width) % _width;
        const auto zp = (z + _height) % _height;
        return point_at(xp, zp)->y;
    }

    void set_point(const uint32_t x, const uint32_t z, const float value) noexcept
    {
        const auto xp = (x + _width) % _width;
        const auto zp = (z + _height) % _height;

        point_at(xp, zp)->y = value;
    }

    void point_from_square(const uint32_t x, const uint32_t z, const uint32_t size, const float value) noexcept;
    void point_from_diamond(const uint32_t x, const uint32_t z, const uint32_t size, const float value) noexcept;
    void diamond_square(const uint32_t step_size, const float scale) noexcept;
    void seed() noexcept;
    void smooth_terrain(const uint32_t pass_size) noexcept;

    void* _vertexptr;
    size_t _stride;
    uint32_t _width;
    uint32_t _height;
    float _roughness;
    xray::base::random_number_generator _rng;
};
}
