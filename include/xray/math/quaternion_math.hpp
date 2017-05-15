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

#pragma once

#include <cmath>

#include "xray/xray.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template <typename real_type>
bool operator==(const quaternion<real_type>& lhs,
                const quaternion<real_type>& rhs) noexcept {
  return is_equal(lhs.w, rhs.w) && is_equal(lhs.x, rhs.x) &&
         is_equal(lhs.y, rhs.y) && is_equal(lhs.z, rhs.z);
}

template <typename real_type>
inline bool operator!=(const quaternion<real_type>& lhs,
                       const quaternion<real_type>& rhs) {
  return !(lhs == rhs);
}

template <typename real_type>
inline quaternion<real_type>
operator+(const quaternion<real_type>& lhs,
          const quaternion<real_type>& rhs) noexcept {
  quaternion<real_type> result{lhs};
  return result += rhs;
}

template <typename real_type>
inline quaternion<real_type>
operator-(const quaternion<real_type>& lhs,
          const quaternion<real_type>& rhs) noexcept {
  quaternion<real_type> result{lhs};
  return result -= rhs;
}

template <typename real_type>
inline quaternion<real_type>
operator-(const quaternion<real_type>& quat) noexcept {
  return {-quat.w, -quat.x, -quat.y, -quat.z};
}

template <typename real_type>
quaternion<real_type> operator*(const quaternion<real_type>& lhs,
                                const quaternion<real_type>& rhs) noexcept {
  quaternion<real_type> result;

  result.w = lhs.w * rhs.w - (lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z);
  result.x = lhs.w * rhs.x + rhs.w * lhs.x + lhs.y * rhs.z - lhs.z * rhs.y;
  result.y = lhs.w * rhs.y + rhs.w * lhs.y + lhs.z * rhs.x - lhs.x * rhs.z;
  result.z = lhs.w * rhs.z + rhs.w * lhs.z + lhs.x * rhs.y - lhs.y * rhs.x;

  return result;
}

template <typename real_type>
inline quaternion<real_type> operator*(const quaternion<real_type>& lhs,
                                       const real_type scalar) noexcept {
  quaternion<real_type> result{lhs};
  return lhs *= scalar;
}

template <typename real_type>
inline quaternion<real_type>
operator*(const real_type scalar, const quaternion<real_type>& rhs) noexcept {
  return rhs * scalar;
}

template <typename real_type>
inline quaternion<real_type> operator/(const quaternion<real_type>& lhs,
                                       const real_type scalar) noexcept {
  quaternion<real_type> result{lhs};
  return result /= scalar;
}

template <typename real_type>
inline quaternion<real_type>
conjugate(const quaternion<real_type>& q) noexcept {
  return {q.w, -q.x, -q.y, -q.z};
}

template <typename real_type>
inline real_type length_squared(const quaternion<real_type>& q) noexcept {
  return q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z;
}

template <typename real_type>
inline real_type length(const quaternion<real_type>& q) noexcept {
  return std::sqrt(length_squared(q));
}

template <typename real_type>
quaternion<real_type> normalize(const quaternion<real_type>& q) noexcept {
  const real_type len_sq = length_squared(q);

  if (is_zero(len_sq)) {
    return quaternion<real_type>::stdc::identity;
  }

  const real_type scale_factor = real_type{} / len_sq;
  return {q.w * scale_factor,
          q.x * scale_factor,
          q.y * scale_factor,
          q.z * scale_factor};
}

template <typename real_type>
quaternion<real_type> invert(const quaternion<real_type>& q) noexcept {
  const real_type len_sq = length_squared(q);

  if (is_zero(len_sq)) {
    return quaternion<real_type>::stdc::identity;
  }

  const real_type scale_factor = real_type{1} / len_sq;
  return {q.w * scale_factor,
          -q.x * scale_factor,
          -q.y * scale_factor,
          -q.z * scale_factor};
}

template <typename real_type>
inline real_type dot_product(const quaternion<real_type>& lhs,
                             const quaternion<real_type>& rhs) {
  return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

template <typename real_type>
scalar3<real_type> rotate_vector(const quaternion<real_type>& q,
                                 const scalar3<real_type>&    v) noexcept {

  assert(is_unit_length(q) && "Quaternion must be unit length!");

  const real_type dotp = real_type{2} * (q.x * v.x + q.y * v.y + q.z * v.z);

  const real_type cross_mul = real_type{2} * q.w;
  const real_type vmul      = cross_mul * q.w - real_type{1};
  const real_type x_val     = v.x;
  const real_type y_val     = v.y;
  const real_type z_val     = v.z;

  return {vmul * x_val + dotp * q.x + cross_mul * (q.y * z_val - q.z * y_val),
          vmul * y_val + dotp * q.y + cross_mul * (q.z * x_val - q.x * z_val),
          vmul * z_val + dotp * q.z + cross_mul * (q.x * y_val - q.y * x_val)};
}

template <typename real_type>
scalar4x4<real_type> rotation_matrix(const quaternion<real_type>& q) noexcept {
  real_type s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

  s = real_type{2} / length_squared(q);

  xs = s * q.x;
  ys = s * q.y;
  zs = s * q.z;

  wx = q.w * xs;
  wy = q.w * ys;
  wz = q.w * zs;

  xx = q.x * xs;
  xy = q.x * ys;
  xz = q.x * zs;

  yy = q.y * ys;
  yz = q.y * zs;

  zz = q.z * zs;

  return {//
          // 1st row
          real_type(1) - (yy + zz),
          xy - wz,
          xz + wy,
          real_type{},
          //
          // 2nd row
          xy + wz,
          real_type(1) - (xx + zz),
          yz - wx,
          real_type{},
          //
          // 3rd row
          xz - wy,
          yz + wx,
          real_type(1) - (xx + yy),
          real_type{},
          //
          // 4th row
          real_type{},
          real_type{},
          real_type{},
          real_type{1}};
}

template <typename real_type>
inline bool is_zero_length(const quaternion<real_type>& q) noexcept {
  return is_zero(length_squared(q));
}

template <typename real_type>
inline bool is_unit_length(const quaternion<real_type>& q) noexcept {
  return is_equal(real_type{1}, length_squared(q));
}

template <typename real_type>
inline bool is_identity(const quaternion<real_type>& q) noexcept {
  return is_equal(real_type(1), q.w) &&
         is_zero(q.x * q.x + q.y * q.y + q.z * q.z);
}

/// @}

} // namespace math
} // namespace xray
