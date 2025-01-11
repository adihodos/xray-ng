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

#include "xray/rendering/geometry/heightmap.generator.hpp"

void
xray::rendering::HeightmapGenerator::point_from_square(const int32_t x,
                                                       const int32_t z,
                                                       const int32_t size,
                                                       const float value) noexcept
{
    const auto hs = size / 2;
    const auto a = get_point(x - hs, z - hs);
    const auto b = get_point(x + hs, z - hs);
    const auto c = get_point(x - hs, z + hs);
    const auto d = get_point(x + hs, z + hs);

    set_point(x, z, (a + b + c + d) / 4.0f + value);
}

void
xray::rendering::HeightmapGenerator::point_from_diamond(const int32_t x,
                                                        const int32_t z,
                                                        const int32_t size,
                                                        const float value) noexcept
{
    const auto hs = size / 2;
    const auto a = get_point(x - hs, z);
    const auto b = get_point(x + hs, z);
    const auto c = get_point(x, z - hs);
    const auto d = get_point(x, z + hs);

    set_point(x, z, (a + b + c + d) / 4.0f + value);
}

void
xray::rendering::HeightmapGenerator::diamond_square(const int32_t step_size, const float scale) noexcept
{
    const auto half_step = step_size / 2;

    for (int32_t z = half_step; z < _height + half_step; z += half_step) {
        for (int32_t x = half_step; x < _width + half_step; x += half_step) {
            point_from_square(x, z, step_size, _rng.next_float(0.0f, 1.0f) * scale);
        }
    }

    for (int32_t z = 0; z < _height; z += step_size) {
        for (int32_t x = 0; x < _width; x += step_size) {
            point_from_diamond(x + half_step, z, step_size, _rng.next_float(0.0f, 1.0f) * scale);
            point_from_diamond(x, z + half_step, step_size, _rng.next_float(0.0f, 1.0f) * scale);
        }
    }
}

void
xray::rendering::HeightmapGenerator::seed() noexcept
{
    for (int32_t z = 0; z < _height; ++z) {
        for (int32_t x = 0; x < _width; ++x) {
            point_at(x, z)->y = 0.0f;
                //_rng.next_float(0.0f, 1.0f) * _roughness;
        }
    }
}

void
xray::rendering::HeightmapGenerator::smooth_terrain(const int32_t pass_size) noexcept
{
    auto sample_size = pass_size;
    auto scale_factor = 1.0f;

    while (sample_size > 1) {
        diamond_square(sample_size, scale_factor);
        sample_size /= 2;
        scale_factor /= 2.0f;
    }
}

// class HeightmapGenerator
// {
//   public:
//     HeightmapGenerator() = default;
//     HeightmapGenerator(const int32_t width, const int32_t height) { generate(width, height); }
//
//     void generate(const int32_t width, const int32_t height)
//     {
//         this->seed(width, height);
//         smooth_terrain(32);
//         smooth_terrain(128);
//     }
//
//     std::span<const vec3f> points() const noexcept { return std::span{ _points }; }
//
//   private:
//     xray::math::vec3f make_point(const float x, const float z)
//     {
//         return { x, _rng.next_float(0.0f, 1.0f) > 0.3f ? std::abs(std::sin(x * z) * _roughness) : 0.0f, z };
//     }
//
//     float get_point(const int32_t x, const int32_t z) const
//     {
//         const auto xp = (x + _width) % _width;
//         const auto zp = (z + _height) % _height;
//
//         return _points[zp * _width + xp].y;
//     }
//
//     void set_point(const int32_t x, const int32_t z, const float value)
//     {
//         const auto xp = (x + _width) % _width;
//         const auto zp = (z + _height) % _height;
//
//         _points[zp * _width + xp].y = value;
//     }
//
//     void point_from_square(const int32_t x, const int32_t z, const int32_t size, const float value)
//     {
//         const auto hs = size / 2;
//         const auto a = get_point(x - hs, z - hs);
//         const auto b = get_point(x + hs, z - hs);
//         const auto c = get_point(x - hs, z + hs);
//         const auto d = get_point(x + hs, z + hs);
//
//         set_point(x, z, (a + b + c + d) / 4.0f + value);
//     }
//
//     void point_from_diamond(const int32_t x, const int32_t z, const int32_t size, const float value)
//     {
//         const auto hs = size / 2;
//         const auto a = get_point(x - hs, z);
//         const auto b = get_point(x + hs, z);
//         const auto c = get_point(x, z - hs);
//         const auto d = get_point(x, z + hs);
//
//         set_point(x, z, (a + b + c + d) / 4.0f + value);
//     }
//
//     void diamond_square(const int32_t step_size, const float scale)
//     {
//         const auto half_step = step_size / 2;
//
//         for (int32_t z = half_step; z < _height + half_step; z += half_step) {
//             for (int32_t x = half_step; x < _width + half_step; x += half_step) {
//                 point_from_square(x, z, step_size, _rng.next_float(0.0f, 1.0f) * scale);
//             }
//         }
//
//         for (int32_t z = 0; z < _height; z += step_size) {
//             for (int32_t x = 0; x < _width; x += step_size) {
//                 point_from_diamond(x + half_step, z, step_size, _rng.next_float(0.0f, 1.0f) * scale);
//                 point_from_diamond(x, z + half_step, step_size, _rng.next_float(0.0f, 1.0f) * scale);
//             }
//         }
//     }
//
//     void seed(const int32_t new_width, const int32_t new_height)
//     {
//         _width = new_width;
//         _height = new_height;
//
//         _points.clear();
//         for (int32_t z = 0; z < _height; ++z) {
//             for (int32_t x = 0; x < _width; ++x) {
//                 _points.push_back(
//                     { static_cast<float>(x), _rng.next_float(0.0f, 1.0f) * _roughness, static_cast<float>(z) });
//             }
//         }
//     }
//
//     void smooth_terrain(const int32_t pass_size)
//     {
//         auto sample_size = pass_size;
//         auto scale_factor = 1.0f;
//
//         while (sample_size > 1) {
//             diamond_square(sample_size, scale_factor);
//             sample_size /= 2;
//             scale_factor /= 2.0f;
//         }
//     }
//
//     float _roughness{ 255.0f };
//     int32_t _width;
//     int32_t _height;
//     xray::base::random_number_generator _rng;
//     std::vector<xray::math::vec3f> _points;
// };
