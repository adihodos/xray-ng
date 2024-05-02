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

#include "xray/rendering/colors/rgb_color.hpp"

xray::rendering::rgb_color&
xray::rendering::rgb_color::operator+=(const rgb_color& rhs) noexcept
{
    r += rhs.r;
    g += rhs.g;
    b += rhs.b;
    a += rhs.a;
    return *this;
}

xray::rendering::rgb_color&
xray::rendering::rgb_color::operator-=(const rgb_color& rhs) noexcept
{
    r -= rhs.r;
    g -= rhs.g;
    b -= rhs.b;
    a -= rhs.a;
    return *this;
}

xray::rendering::rgb_color&
xray::rendering::rgb_color::operator*=(float const k) noexcept
{
    r *= k;
    g *= k;
    b *= k;
    a *= k;
    return *this;
}

xray::rendering::rgb_color&
xray::rendering::rgb_color::operator*=(const rgb_color& other) noexcept
{
    r *= other.r;
    g *= other.g;
    b *= other.b;
    a *= other.a;
    return *this;
}

xray::rendering::rgb_color&
xray::rendering::rgb_color::operator/=(const float scalar) noexcept
{
    float k = 1.0f / scalar;
    r *= k;
    g *= k;
    b *= k;
    a *= k;
    return *this;
}

xray::rendering::rgb_color
xray::rendering::operator+(const rgb_color& lhs, const rgb_color& rhs) noexcept
{
    rgb_color result(lhs);
    result += rhs;
    return result;
}

xray::rendering::rgb_color
xray::rendering::operator-(const rgb_color& lhs, const rgb_color& rhs) noexcept
{
    rgb_color result(lhs);
    result -= rhs;
    return result;
}

xray::rendering::rgb_color
xray::rendering::operator*(const rgb_color& lhs, const rgb_color& rhs) noexcept
{
    rgb_color result(lhs);
    result *= rhs;
    return result;
}

xray::rendering::rgb_color
xray::rendering::operator*(const rgb_color& lhs, const float k) noexcept
{
    rgb_color result(lhs);
    result *= k;
    return result;
}

xray::rendering::rgb_color
xray::rendering::operator*(const float k, const rgb_color& rhs) noexcept
{
    return rhs * k;
}

xray::rendering::rgb_color
xray::rendering::operator/(const rgb_color& lhs, const float scalar) noexcept
{
    const float inv = 1.0f / scalar;
    return lhs * inv;
}

namespace xray::rendering {
oklab
linear_srgb_to_oklab(const rgb_color& c)
{
    float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
    float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
    float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

    float l_ = cbrtf(l);
    float m_ = cbrtf(m);
    float s_ = cbrtf(s);

    return {
        0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
        1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
        0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_,
    };
}

rgb_color
oklab_to_linear_srgb(const oklab& c)
{
    float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
    float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
    float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;

    float l = l_ * l_ * l_;
    float m = m_ * m_ * m_;
    float s = s_ * s_ * s_;

    return rgb_color{ +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
                      -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
                      -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
                      1.0f };
}

}
