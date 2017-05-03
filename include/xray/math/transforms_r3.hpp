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

#include "xray/xray.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3.hpp"
#include <cmath>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

/// \brief      Matrices for frequently used transformations in R3.
struct R3 {

  /// \brief      Returns a rotation matrix using Euler angles.
  template <typename T>
  static scalar3x3<T>
  rotate_euler_xyz(const T rx, const T ry, const T rz) noexcept;

  /// \brief      Returns a matrix for a rotation around the X axis.
  template <typename T>
  static scalar3x3<T> rotate_x(const T theta) noexcept;

  /// \brief      Returns a matrix for a rotation around the Y axis.
  template <typename T>
  static scalar3x3<T> rotate_y(const T theta) noexcept;

  /// \brief      Returns a matrix for a rotation around the Z axis.
  template <typename T>
  static scalar3x3<T> rotate_z(const T theta) noexcept;

  /// \brief      Returns a matrix for a rotation around the all three axis.
  template <typename T>
  static scalar3x3<T>
  rotate_xyz(const T angle_x, const T angle_y, const T angle_z) noexcept;
  /// \brief      Returns a matrix for a rotation with a specified angle around
  /// an arbitrary axis.
  template <typename T>
  static scalar3x3<T> rotate_axis_angle(const scalar3<T>& axisv,
                                        const T           theta) noexcept;

  /// \brief      Returns the matrix for an axis angle rotation of v_src into
  /// v_dst.
  template <typename T>
  static scalar3x3<T> rotate_from_vectors(const scalar3<T>& v_src,
                                          const scalar3<T>& v_dst) noexcept;

  /// \brief      Returns a matrix for a reflection around a plane's normal.
  template <typename T>
  static scalar3x3<T> reflect_plane(const scalar3<T>& plane_norm) noexcept;

  template <typename T>
  static scalar3x3<T> scale_x(const T s) noexcept;

  template <typename T>
  static scalar3x3<T> scale_y(const T s) noexcept;

  template <typename T>
  static scalar3x3<T> scale_z(const T s) noexcept;

  template <typename T>
  static scalar3x3<T> scale_xyz(const T sx, const T sy, const T sz) noexcept;

  template <typename T>
  static scalar3x3<T> scale(const T s) noexcept {
    return scale_xyz(s, s, s);
  }
};

template <typename T>
scalar3x3<T> R3::rotate_euler_xyz(const T rx, const T ry, const T rz) noexcept {
  const auto sx = std::sin(rx);
  const auto cx = std::cos(rx);
  const auto sy = std::sin(ry);
  const auto cy = std::cos(ry);
  const auto sz = std::sin(rz);
  const auto cz = std::cos(rz);

  // clang-format off

  return {
    cy * cz, -cy * sz, sy,
    sx * sy * cz + cx * sz, -sx * sy * sz + cx * cz, -sx * cy,
    -cx * sy * cz + sx * sz, cx * sy * sz + sx * cz, cx * cy};

  // clang-format on
}

template <typename T>
scalar3x3<T> R3::rotate_x(const T theta) noexcept {
  const auto sin_theta = std::sin(theta);
  const auto cos_theta = std::cos(theta);

  // clang-format off

  return {T(1),       T(0),         T(0),
          T(0),       cos_theta,    -sin_theta,
          T(0),       sin_theta,    cos_theta};

  // clang-format on
}

template <typename T>
scalar3x3<T> R3::rotate_y(const T theta) noexcept {
  const auto sin_theta = std::sin(theta);
  const auto cos_theta = std::cos(theta);

  // clang-format off

  return {
        cos_theta,  T{0}, sin_theta,
        T{0},       T{1}, T{0},
        -sin_theta, T{0}, cos_theta
    };

  // clang-format on
}

template <typename T>
scalar3x3<T> R3::rotate_z(const T theta) noexcept {
  const T sin_theta = std::sin(theta);
  const T cos_theta = std::cos(theta);

  // clang-format off

  return {cos_theta,    -sin_theta,   T{0},
          sin_theta,    cos_theta,    T{0},
          T{0},         T{0},         T{1}};

  // clang-format on
}

template <typename T>
scalar3x3<T>
R3::rotate_xyz(const T angle_x, const T angle_y, const T angle_z) noexcept {
  const auto cosx = std::cos(angle_x);
  const auto sinx = std::sin(angle_x);

  const auto cosy = std::cos(angle_y);
  const auto siny = std::sin(angle_y);

  const auto cosz = std::cos(angle_z);
  const auto sinz = std::sin(angle_z);

  // clang-format off

    return {cosy * cosz,
            cosz * sinx * siny - cosx * sinz,
            sinx * sinz + cosx * cosz * siny,

            cosy * sinz,
            sinx * siny * sinz + cosx * cosz,
            cosx * siny * sinz - cosz * sinx,

            -siny,
            cosy * sinx,
            cosx * cosy};

  // clang-format on
}

template <typename T>
scalar3x3<T> R3::rotate_axis_angle(const scalar3<T>& axisv,
                                   const T           theta) noexcept {

  const auto sin_theta = std::sin(theta);
  const auto cos_theta = std::cos(theta);
  const auto tval      = T{1} - cos_theta;

  return {tval * axisv.x * axisv.x + cos_theta,
          tval * axisv.x * axisv.y - sin_theta * axisv.z,
          tval * axisv.x * axisv.z + sin_theta * axisv.y,

          tval * axisv.x * axisv.y + sin_theta * axisv.z,
          tval * axisv.y * axisv.y + cos_theta,
          tval * axisv.y * axisv.z - sin_theta * axisv.x,

          tval * axisv.x * axisv.z - sin_theta * axisv.y,
          tval * axisv.y * axisv.z + sin_theta * axisv.x,
          tval * axisv.z * axisv.z + cos_theta};
}

template <typename T>
scalar3x3<T> R3::rotate_from_vectors(const scalar3<T>& v_src,
                                     const scalar3<T>& v_dst) noexcept {
  //
  // Compute the angle between the two vectors.
  const auto rotation_axis = cross(v_src, v_dst);

  if (is_zero_length(rotation_axis)) {
    //
    // Vectors are parallel, so return a matrix for a rotation around
    // first vector with 0 radians.
    return R3::rotate_axis_angle(normalize(v_src), T{});
  }

  //
  // Compute the cross product and normalize to get the rotation axis.
  return R3::rotate_axis_angle(normalize(rotation_axis),
                               angle_of(v_src, v_dst));
}

template <typename T>
scalar3x3<T> R3::reflect_plane(const scalar3<T>& plane_norm) noexcept {
  return {T{1} - T{2} * plane_norm.x * plane_norm.x,
          T{-2} * plane_norm.x * plane_norm.y,
          T{-2} * plane_norm.x * plane_norm.z,

          T{-2} * plane_norm.y * plane_norm.x,
          T{-2} - T{2} * plane_norm.y * plane_norm.y,
          T{-2} * plane_norm.y * plane_norm.z,

          T{-2} * plane_norm.z * plane_norm.x,
          T{-2} * plane_norm.z * plane_norm.y,
          T{1} - T{2} * plane_norm.z * plane_norm.z};
}

template <typename T>
scalar3x3<T> R3::scale_x(const T s) noexcept {
  return {// x
          s,
          T{},
          T{},
          // y
          T{},
          T{1},
          T{},
          // z
          T{},
          T{},
          T{1}};
}

template <typename T>
scalar3x3<T> R3::scale_y(const T s) noexcept {
  return {// x
          T{1},
          T{},
          T{},
          // y
          T{},
          s,
          T{},
          // z
          T{},
          T{},
          T{1}};
}

template <typename T>
scalar3x3<T> R3::scale_z(const T s) noexcept {
  return {// x
          T{1},
          T{},
          T{},
          // y
          T{},
          T{1},
          T{},
          // z
          T{},
          T{},
          s};
}

template <typename T>
scalar3x3<T> R3::scale_xyz(const T sx, const T sy, const T sz) noexcept {
  return {// x
          sx,
          T{},
          T{},
          // y
          T{},
          sy,
          T{},
          // z
          T{},
          T{},
          sz};
}

/// @}

} // namespace math
} // namespace xray
