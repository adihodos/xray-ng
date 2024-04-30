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
#include "xray/math/scalar4.hpp"
#include "xray/xray.hpp"

namespace xray {
namespace rendering {

///
/// \brief Represents a 4 component (red, green, blue, alpha)
///       normalized color vector (128 bits).
/// \remarks Color operations (addition, substraction,
///         component-wise multiplication, scalar multiplication) can result
///         in individual components having values out of the [0, 1] range, so
///         some form of normalization should be used, to correct those
///         situations.
struct rgb_color
{
  public:
    union
    {

        struct
        {
            float r; ///< Red component intensity
            float g; ///< Green component intensity
            float b; ///< Blue component intensity
            float a; ///< Alpha component (opacity)
        };

        float components[4];
    };

    /// \name Constructors
    /// @{

  public:
    rgb_color() noexcept = default;

    constexpr rgb_color(const float val) noexcept
        : rgb_color{ val, val, val, 1.0f }
    {
    }

    constexpr rgb_color(const float red, const float green, const float blue, const float alpha = 1.0f) noexcept
        : r{ red }
        , g{ green }
        , b{ blue }
        , a{ alpha }
    {
    }

    constexpr explicit rgb_color(const math::vec3f v, const float a_ = 1.0f)
        : r{ v.x }
        , g{ v.y }
        , b{ v.z }
        , a{ a_ }
    {
    }

    constexpr explicit rgb_color(const math::vec4f v)
        : r{ v.x }
        , g{ v.y }
        , b{ v.z }
        , a{ v.w }

    {
    }

    /// \name   Conversions
    /// @{
    explicit operator math::vec3f() const noexcept { return math::vec3f{ this->r, this->g, this->b }; }

    explicit operator math::vec4f() const noexcept { return math::vec4f{ this->r, this->g, this->b, this->a }; }

  public:
    /// \name   Self assign operators
    /// @{

  public:
    rgb_color& operator+=(const rgb_color& rhs) noexcept;

    rgb_color& operator-=(const rgb_color& rhs) noexcept;

    rgb_color& operator*=(float const k) noexcept;

    /// Performs a componentwise multiplication between the two colors.
    rgb_color& operator*=(const rgb_color& other) noexcept;

    rgb_color& operator/=(const float scalar) noexcept;

    /// @}
};

rgb_color
operator+(const rgb_color& lhs, const rgb_color& rhs) noexcept;

rgb_color
operator-(const rgb_color& lhs, const rgb_color& rhs) noexcept;

rgb_color
operator*(const rgb_color& lhs, const rgb_color& rhs) noexcept;

rgb_color
operator*(const rgb_color& lhs, const float k) noexcept;

rgb_color
operator*(const float k, const rgb_color& rhs) noexcept;

rgb_color
operator/(const rgb_color& lhs, const float scalar) noexcept;

} // namespace rendering
} // namespace xray
