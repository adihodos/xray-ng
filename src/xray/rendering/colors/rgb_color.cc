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
