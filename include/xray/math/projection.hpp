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
#include "xray/math/handedness.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include <cassert>
#include <cmath>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template <typename T>
scalar4x4<T> lookat_to_matrix(const scalar3<T>& direction,
                              const scalar3<T>& eye_pos,
                              const scalar3<T>& ref_up) noexcept {
  assert(!is_zero_length(direction) &&
         "View direction vector cannot be zero length !");
  assert(!are_parallel(direction, ref_up) &&
         "Direction vector cannot be parallel to reference up vector!");

  const auto up    = normalize(ref_up - project(ref_up, direction));
  const auto right = cross(up, direction);

  return make_view_transform(right, up, direction, eye_pos);
}

template <typename T>
scalar4x4<T> make_view_transform(const scalar3<T>& right, const scalar3<T>& up,
                                 const scalar3<T>& direction,
                                 const scalar3<T>& eye_pos) noexcept {

  return {right.x,     right.y,     right.z,     -dot(eye_pos, right),
          up.x,        up.y,        up.z,        -dot(eye_pos, up),
          direction.x, direction.y, direction.z, -dot(eye_pos, direction),
          T(0),        T(0),        T(0),        T(1)};
}

template <typename Handedness>
struct view_framing;

template <>
struct view_framing<left_handed> {
  template <typename T>
  static scalar4x4<T> look_at(const scalar3<T>& eye_pos,
                              const scalar3<T>& target,
                              const scalar3<T>& world_up) noexcept {
    const auto direction = normalize(target - eye_pos);
    return lookat_to_matrix(direction, eye_pos, world_up);
  }
};

template <>
struct view_framing<right_handed> {
  template <typename T>
  static scalar4x4<T> look_at(const scalar3<T>& eye_pos,
                              const scalar3<T>& target,
                              const scalar3<T>& world_up) noexcept {
    //const auto direction = normalize(eye_pos - target);
    //return lookat_to_matrix(direction, eye_pos, world_up);

    const auto dir = normalize(target - eye_pos);
    const auto up = normalize(world_up - project(world_up, dir));
    const auto right = cross(dir, up);

    return{

    };
  }
};

template <typename H>
struct projector;

/// \brief  Builds projection matrices assuming a right handed coordinate
///         system.
template <>
struct projector<right_handed> {

  template <typename T>
  static scalar4x4<T> perspective_symmetric(const T width, const T height,
                                            const T fov, const T near_plane,
                                            const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> perspective(const T left, const T right, const T top,
                                  const T bottom, const T near_plane,
                                  const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> ortho_off_center(const T left, const T right, const T top,
                                       const T bottom, const T near_plane,
                                       const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> ortho(const T width, const T height, const T near_plane,
                            const T far_plane) noexcept;
};

template <typename T>
scalar4x4<T>
projector<right_handed>::perspective_symmetric(const T width, const T height,
                                               const T fov, const T near_plane,
                                               const T far_plane) noexcept {
  //
  // aspect ratio
  const auto a = width / height;
  //
  // distance to projection plane
  const auto d = T(1) / std::tan(fov / T(2));

  auto proj = scalar4x4<T>::stdc::null;

  proj.a00 = d / a;

  proj.a11 = d;

  proj.a22 = (near_plane + far_plane) / (near_plane - far_plane);
  proj.a23 = (T(2) * near_plane * far_plane) / (near_plane - far_plane);

  proj.a32 = T(-1);

  return proj;
}

template <typename T>
scalar4x4<T> projector<right_handed>::perspective(const T left, const T right,
                                                  const T top, const T bottom,
                                                  const T near_plane,
                                                  const T far_plane) noexcept {
  const auto x_sum = right + left;
  const auto x_dif = right - left;
  const auto y_sum = top + bottom;
  const auto y_dif = top - bottom;

  auto proj = scalar4x4<T>::stdc::null;

  proj.a00 = T(2) * near_plane / x_dif;
  proj.a02 = -x_sum / x_dif;

  proj.a11 = T(2) * near_plane / y_dif;
  proj.a12 = -y_sum / y_dif;

  proj.a20 = (near_plane + far_plane) / (near_plane - far_plane);
  proj.a21 = T(2) * near_plane * far_plane / (near_plane - far_plane);

  proj.a32 = T(-1);

  return proj;
}

template <typename T>
inline scalar4x4<T> projector<right_handed>::ortho_off_center(
    const T left, const T right, const T top, const T bottom,
    const T near_plane, const T far_plane) noexcept {

  const auto x_diff = right - left;
  const auto x_sum  = right + left;
  const auto y_diff = top - bottom;
  const auto y_sum  = bottom - top;
  const auto z_diff = far_plane - near_plane;
  const auto z_sum  = far_plane + near_plane;

  return {// 1st row
          T(2) / x_diff, T{}, T{}, -(x_sum / x_diff),
          // 2nd row
          T(0), T(2) / y_diff, T(0), -(y_sum / y_diff),
          // 3rd row
          T(0), T(0), T(-2) / z_diff, -(z_sum / z_diff),
          // 4th row
          T(0), T(0), T(0), T(1)};
}

template <typename T>
scalar4x4<T> projector<right_handed>::ortho(const T width, const T height,
                                            const T near_plane,
                                            const T far_plane) noexcept {
  const auto z_diff = far_plane - near_plane;
  const auto z_sum  = far_plane + near_plane;

  return {// 1st row
          T(2) / width, T(0), T(0), T(0),
          // 2nd row
          T(0), T(2) / height, T(0), T(0),
          // 3rd row
          T(0), T(0), T(-2) / z_diff, -(z_sum / z_diff),
          // 4th row
          T(0), T(0), T(0), T(1)};
}

/// \brief  Builds projection matrices assuming a left handed coordinate
///         system.
template <>
struct projector<left_handed> {

  template <typename T>
  static scalar4x4<T> perspective_symmetric(const T width, const T height,
                                            const T fov, const T near_plane,
                                            const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> perspective(const T left, const T right, const T top,
                                  const T bottom, const T near_plane,
                                  const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> orthographic(const T left, const T right, const T top,
                                   const T bottom, const T near_plane,
                                   const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> orthographic_symmetric(const T width, const T height,
                                             const T near_plane,
                                             const T far_plane) noexcept;
};

template <typename T>
scalar4x4<T>
projector<left_handed>::perspective_symmetric(const T width, const T height,
                                              const T fov, const T near_plane,
                                              const T far_plane) noexcept {
  //
  // aspect ratio
  const auto a = width / height;
  //
  // distance to projection plane
  const auto d = T(1) / std::tan(fov / T(2));

  auto proj = scalar4x4<T>::stdc::null;

  proj.a00 = d / a;

  proj.a11 = d;

  proj.a22 = (far_plane) / (far_plane - near_plane);
  proj.a23 = (-near_plane * far_plane) / (far_plane - near_plane);

  proj.a32 = T(1);

  return proj;
}

template <typename T>
scalar4x4<T> projector<left_handed>::perspective(const T left, const T right,
                                                 const T top, const T bottom,
                                                 const T near_plane,
                                                 const T far_plane) noexcept {
  const auto x_sum = right + left;
  const auto x_dif = right - left;
  const auto y_sum = top + bottom;
  const auto y_dif = top - bottom;

  auto proj = scalar4x4<T>::stdc::null;

  proj.a00 = T(2) * near_plane / x_dif;
  proj.a02 = -x_sum / x_dif;

  proj.a11 = T(2) * near_plane / y_dif;
  proj.a12 = -y_sum / y_dif;

  proj.a20 = (far_plane) / (far_plane - near_plane);
  proj.a21 = (-near_plane * far_plane) / (far_plane - near_plane);

  proj.a32 = T(1);

  return proj;
}

template <typename T>
scalar4x4<T> projector<left_handed>::orthographic(const T left, const T right,
                                                  const T top, const T bottom,
                                                  const T near_plane,
                                                  const T far_plane) noexcept {

  const auto x_diff = right - left;
  const auto x_sum  = right + left;
  const auto y_diff = top - bottom;
  const auto y_sum  = top + bottom;
  const auto z_diff = far_plane - near_plane;

  return {// 1st row
          T(2) / x_diff, T{}, T{}, -(x_sum / x_diff),
          // 2nd row
          T{}, T(2) / y_diff, T(0), -(y_sum / y_diff),
          // 3rd row
          T(0), T(0), T(1) / z_diff, -(near_plane / z_diff),
          // 4th row
          T(0), T(0), T(0), T(1)};
}

template <typename T>
scalar4x4<T>
projector<left_handed>::orthographic_symmetric(const T width, const T height,
                                               const T near_plane,
                                               const T far_plane) noexcept {
  const auto z_diff = far_plane - near_plane;

  return {// 1st row
          T(2) / width, T(0), T(0), T(0),
          // 2nd row
          T(0), T(2) / height, T(0), T(0),
          // 3rd row
          T(0), T(0), T(1) / z_diff, -(near_plane / z_diff),
          // 4th row
          T(0), T(0), T(0), T(1)};
}

using view_frame    = view_framing<xray_default_handedness>;
using view_frame_lh = view_framing<left_handed>;
using view_frame_rh = view_framing<right_handed>;

using projection    = projector<xray_default_handedness>;
using projection_lh = projector<left_handed>;
using projection_rh = projector<right_handed>;

/// @}

} // namespace math
} // namespace xray
