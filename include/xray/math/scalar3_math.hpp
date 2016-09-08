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

#include <cmath>

#include "xray/xray.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar3.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template <typename T>
inline scalar3<T>& scalar3<T>::operator+=(const scalar3<T>& rhs) noexcept {
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;
  return *this;
}

template <typename T>
inline scalar3<T>& scalar3<T>::operator-=(const scalar3<T>& rhs) noexcept {
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;
  return *this;
}

template <typename T>
inline scalar3<T>& scalar3<T>::operator*=(const T scalar) noexcept {
  x *= scalar;
  y *= scalar;
  z *= scalar;
  return *this;
}

template <typename T>
inline scalar3<T>& scalar3<T>::operator/=(const T scalar) noexcept {
  x /= scalar;
  y /= scalar;
  z /= scalar;
  return *this;
}

template <typename T>
inline bool operator==(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return is_equal(a.x, b.x) && is_equal(a.y, b.y) && is_equal(a.z, b.z);
}

template <typename T>
inline bool operator!=(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return !(a == b);
}

template <typename T>
inline scalar3<T> operator+(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

template <typename T>
inline scalar3<T> operator-(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

template <typename T>
inline scalar3<T> operator-(const scalar3<T>& a) noexcept {
  return {-a.x, -a.y, -a.z};
}

template <typename T>
inline scalar3<T> operator*(const scalar3<T>& v, const T k) noexcept {
  return {v.x * k, v.y * k, v.z * k};
}

template <typename T>
inline scalar3<T> operator*(const T k, const scalar3<T>& v) noexcept {
  return v * k;
}

template <typename T>
inline scalar3<T> operator/(const scalar3<T>& v, const T k) noexcept {
  return {v.x / k, v.y / k, v.z / k};
}

/// \brief Returns the dot product of two vectors.
template <typename T>
inline T dot(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

/// \brief Returns a vector that is orthogonal to the input vectors.
template <typename T>
inline scalar3<T> cross(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

/// \brief Returns the square of the vector's length.
template <typename T>
inline T length_squared(const scalar3<T>& a) noexcept {
  return a.x * a.x + a.y * a.y + a.z * a.z;
}

/// \brief Returns the square of the vector's length.
template <typename T>
inline T length_squared(const T x, const T y, const T z) noexcept {
  return x * x + y * y + z * z;
}

template <typename T>
inline bool is_zero_length(const scalar3<T>& v) noexcept {
  return is_zero(length_squared(v));
}

template <typename T>
inline bool is_unit_length(const scalar3<T>& v) noexcept {
  return is_equal(length_squared(v), T(1));
}

/// \brief Returns the length of the vector.
template <typename T>
inline T length(const scalar3<T>& v) noexcept {
  return std::sqrt(length_squared(v));
}

/// \brief Returns the length of the vector.
template <typename T>
inline T length(const T x, const T y, const T z) noexcept {
  return std::sqrt(length_squared(x, y, z));
}

/// \brief Returns a unit length vector for the input vector.
template <typename T>
scalar3<T> normalize(const scalar3<T>& v) noexcept {
  const auto len_squared = length_squared(v);
  if (is_zero(len_squared))
    return scalar3<T>::stdc::zero;

  const auto len = std::sqrt(len_squared);
  return v / len;
}

/// \brief Returns a vector representing the projection of P on the vector Q;
template <typename T>
inline scalar3<T> project(const scalar3<T>& p, const scalar3<T>& q) noexcept {
  return (dot(p, q) * q) / length_squared(q);
}

/// \brief Returns a vector that is the projection of P on the \c unit vector q.
template <typename T>
inline scalar3<T> project_unit(const scalar3<T>& p,
                               const scalar3<T>& q) noexcept {
  return dot(p, q) * q;
}

/// \brief Tests if two vectors are orthogonal.
template <typename T>
inline bool are_orthogonal(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return is_zero(dot(a, b));
}

/// \brief Returns the angle between two vectors.
template <typename T>
inline T angle_of(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return std::acos(dot(a, b) / (length(a) * length(b)));
}

/// \brief Tests if two vectors are parallel.
template <typename T>
inline bool are_parallel(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return is_zero_length(cross(a, b));
}

/// \brief Tests if three points are collinear.
template <typename T>
inline bool are_collinear(const scalar3<T>& a, const scalar3<T>& b,
                          const scalar3<T>& c) noexcept {
  return are_parallel(b - a, c - a);
}

/// \brief Returns the value of the triple scalar product.
template <typename T>
inline T triple_scalar_product(const scalar3<T>& a, const scalar3<T>& b,
                               const scalar3<T>& c) noexcept {
  return dot(a, cross(b, c));
}

/// \brief Returns the triple vector product of three vectors.
template <typename T>
inline scalar3<T> triple_vector_product(const scalar3<T>& a,
                                        const scalar3<T>& b,
                                        const scalar3<T>& c) noexcept {
  return cross(a, cross(b, c));
}

/// \brief Returns the square of the distance between two points.
template <typename T>
inline T squared_distance(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return length_squared(b - a);
}

/// \brief Returns the square of the distance between two points.
template <typename T>
inline T squared_distance(const T ax, const T ay, const T az, const T bx,
                          const T by, const T bz) noexcept {
  return length_squared(bx - ax, by - ay, bz - az);
}

/// \brief Returns the distance between two points.
template <typename T>
inline T distance(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return length(b - a);
}

/// \brief Returns the square of the distance between two points.
template <typename T>
inline T distance(const T ax, const T ay, const T az, const T bx, const T by,
                  const T bz) noexcept {
  return length(bx - ax, by - ay, bz - az);
}

/// Returns the signed area of the triangle spanned by three points.
template <typename T>
inline T triangle_signed_area(scalar3<T> const& p0, scalar3<T> const& p1,
                              scalar3<T> const& p2) noexcept {
  const T parallelogram_area = p0.x * (p1.y * p2.z - p1.z * p2.y) +
                               p0.y * (p1.x * p2.z - p2.x * p1.z) +
                               p0.z * (p1.x * p2.y - p2.x * p1.y);

  return parallelogram_area / T(2);
}

/// Test if the points spanning a triangle are ordered counter clockwise.
template <typename T>
inline bool ccw_winded_triangle(scalar3<T> const& p0, scalar3<T> const& p1,
                                scalar3<T> const& p2) noexcept {
  return triangle_signed_area(p0, p1, p2) > T{0};
}

/// \brief Returns a vector whose components have the maximum values of the
/// components of the two input vectors.
template <typename T>
scalar3<T> max(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return {max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)};
}

/// \brief Returns a vector whose components have the minimum values of the
/// components of the two input vectors.
template <typename T>
scalar3<T> min(const scalar3<T>& a, const scalar3<T>& b) noexcept {
  return {min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)};
}

/// \brief Convert from spherical coordinates to Cartesian coordinates.
/// \param   r The radius of the sphere.
/// \param   phi Angle with the y axis (in radians).
/// \param   theta Angle with the z axis (in radians).
/// \remarks The formula for conversion is :
///       x = r * sin(phi) * sin(theta);
///       y = r * cos(phi)
///       z = r * sin(phi) * cos(theta)
template <typename T>
scalar3<T> point_from_spherical_coords(const T r, const T theta,
                                       const T phi) noexcept {
  const auto st = std::sin(theta);
  const auto ct = std::cos(theta);
  const auto sp = std::sin(phi);
  const auto cp = std::cos(phi);

  return {r * sp * st, r * cp, r * sp * ct};
}

/// \brief   Convert from Cartesian coordinates to spherical coordinates.
/// \param   pt  A point, in Cartesian coordinates.
/// \return  A scalar3 object with the conversion result. The X member
/// represents the sphere's radius, Y member is angle phi (angle with the y
/// axis), Z member is angle theta (angle with z axis).
/// \remarks The formula for conversion is :
///  r = length(pt)
///  phi = atan2(sqrt(pt.X/// pt.X + pt.Z/// pt.Z), pt.Y)
///  theta = atan2(pt.X, pt.Z)
template <typename T>
scalar3<T> point_to_spherical_coordinates(const scalar3<T>& pt) noexcept {
  const auto delta = length(pt);
  const auto phi   = std::atan2(std::sqrt(pt.x * pt.x + pt.z * pt.z), pt.y);
  const auto theta = atan2(pt.x, pt.z);

  return {delta, phi, theta};
}

/// @}

} // namespace math
} // namespace xray
