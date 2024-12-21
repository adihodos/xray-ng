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

/// \file scalar2_math.hpp Operations on scalar2 types.

#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/xray.hpp"
#include <cmath>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template<typename T>
inline scalar2<T>&
scalar2<T>::operator+=(const scalar2<T>& rhs) noexcept
{
    x += rhs.x;
    y += rhs.y;
    return *this;
}

template<typename T>
inline scalar2<T>&
scalar2<T>::operator-=(const class_type& rhs) noexcept
{
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

template<typename T>
inline scalar2<T>&
scalar2<T>::operator*=(const T scalar) noexcept
{
    x *= scalar;
    y *= scalar;
    return *this;
}

template<typename T>
inline scalar2<T>&
scalar2<T>::operator/=(const T scalar) noexcept
{
    x /= scalar;
    y /= scalar;
    return *this;
}

template<typename T>
inline bool
is_zero_length(const scalar2<T>& v) noexcept
{
    return is_zero(length_squared(v));
}

template<typename T>
inline bool
is_unit_length(const scalar2<T>& v) noexcept
{
    return is_equal(length_squared(v), T(1));
}

template<typename T>
inline bool
operator==(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return is_equal(a.x, b.x) && is_equal(a.y, b.y);
}

template<typename T>
inline bool
operator!=(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return !(a == b);
}

template<typename T>
inline auto
operator-(const scalar2<T>& a) noexcept
{
    return scalar2<T>{ -a.x, -a.y };
}

template<typename T>
inline auto
operator*(const scalar2<T>& a, const T b) noexcept
{
    return scalar2<T>{ a.x * b, a.y * b };
}

template<typename T>
inline auto
operator*(const T a, const scalar2<T>& b) noexcept
{
    return b * a;
}

template<typename T>
inline auto
operator/(const scalar2<T>& a, const T b) noexcept
{
    return scalar2<T>{ a.x / b, a.y / b };
}

template<typename T>
inline auto
operator+(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return scalar2<T>{ a.x + b.x, a.y + b.y };
}

template<typename T>
inline auto
operator-(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return scalar2<T>{ a.x - b.x, a.y - b.y };
}

/// \brief Returns the square of the length of the input vector.
template<typename T>
inline auto
length_squared(const scalar2<T>& a) noexcept
{
    return a.x * a.x + a.y * a.y;
}

/// \brief Returns the length of the vector.
template<typename T>
inline auto
length(const scalar2<T>& a) noexcept
{
    return std::sqrt(length_squared(a));
}

/// \brief Returns the dot product of two vectors.
template<typename T>
inline auto
dot(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return a.x * b.x + a.y * b.y;
}

template<typename T>
inline auto
dot_dot(const scalar2<T>& a) noexcept
{
    return length_squared(a);
}

/// \brief Returns the unit length vector for the input vector.
template<typename T>
scalar2<T>
normalize(const scalar2<T>& v) noexcept
{
    const auto len_sq = length_squared(v);
    if (is_zero(len_sq))
        return scalar2<T>::stdc::zero;

    const auto len = std::sqrt(len_sq);
    return { v.x / len, v.y / len };
}

/// \brief Returns the projection of a vector on another vector.
template<typename T>
inline scalar2<T>
project(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return (dot(a, b) * b) / (dot_dot(b));
}

/// \brief Returns a vector that is the projection of the first vector
/// on another vector that is unit length.
template<typename T>
inline scalar2<T>
project_unit(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return dot(a, b) * b;
}

/// \brief Test if two vectors are orthogonal.
template<typename T>
inline bool
are_orthogonal(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return is_zero(dot(a, b));
}

/// \brief Returns the angle between two vectors.
template<typename T>
inline T
angle_of(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return std::acos(dot(a, b) / (length(a) * length(b)));
}

/// \brief Returns a vector that is orthogonal to the input vector.
template<typename T>
inline scalar2<T>
perpendicular(const scalar2<T>& v) noexcept
{
    return { -v.y, v.x };
}

/// \brief Returns the dot product between the perpendicular of the
/// first input vector and the second input vector.
template<typename T>
inline T
perp_product(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return -a.y * b.x + a.x * b.y;
}

/// \brief Returns true if the vectors are parallel.
template<typename T>
inline bool
are_parallel(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return is_zero_length(perp_product(a, b));
}

/// \brief Test if three points are collinear.
template<typename T>
inline bool
are_collinear(const scalar2<T>& a, const scalar2<T>& b, const scalar2<T>& c) noexcept
{
    return are_parallel(b - a, c - a);
}

/// \brief Returns the square of the distance between two points.
template<typename T>
inline T
squared_distance(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return length_squared(b - a);
}

/// \brief Returns the distance between two points.
template<typename T>
inline T
distance(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return length(b - a);
}

/// \brief Convert from polar coordinates to Cartesian coordinates.
/// \param theta Angle with X axis (ccw).
/// \param radius Distance from origin.
template<typename T>
inline scalar2<T>
point_from_polar_coords(const T theta, const T radius) noexcept
{
    const auto ct = std::cos(theta);
    const auto st = std::sin(theta);

    return { radius * ct, radius * st };
}

/// \brief Convert from Cartesian coordinates to polar coordinates.
/// \return Point in polar coordinates. Radius is stored in member \b x,
/// angle relative to x axis is stored in member \b y.
template<typename T>
scalar2<T>
point_to_polar_coords(const scalar2<T>& pt) noexcept
{
    return { lengt(pt), std::atan2(pt.y, pt.x) };
}

/// \brief Returns a vector whose components have the maximum values of the
/// components of the two input vectors.
template<typename T>
scalar2<T>
max(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return { max(a.x, b.x), max(a.y, b.y) };
}

/// \brief Returns a vector whose components have the minimum values of the
/// components of the two input vectors.
template<typename T>
scalar2<T>
min(const scalar2<T>& a, const scalar2<T>& b) noexcept
{
    return { min(a.x, b.x), min(a.y, b.y) };
}

/// @}

} // namespace math
} // namespace xray
