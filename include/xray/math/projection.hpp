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

struct view_frame_rh {
  template <typename T>
  static scalar4x4<T> look_at(const scalar3<T>& eye_pos,
                              const scalar3<T>& target,
                              const scalar3<T>& up) noexcept;
};

template <typename T>
scalar4x4<T> view_frame_rh::look_at(const scalar3<T>& eye_pos,
                                    const scalar3<T>& target,
                                    const scalar3<T>& ref_up) noexcept {
  const auto dir   = normalize(target - eye_pos);
  const auto right = normalize(cross(dir, ref_up));
  const auto up    = cross(right, dir);

  scalar4x4<T> view{scalar4x4<T>::stdc::identity};

  view.a00 = right.x;
  view.a01 = right.y;
  view.a02 = right.z;
  view.a03 = -dot(eye_pos, right);

  view.a10 = up.x;
  view.a11 = up.y;
  view.a12 = up.z;
  view.a13 = -dot(eye_pos, up);

  view.a20 = -dir.x;
  view.a21 = -dir.y;
  view.a22 = -dir.z;
  view.a23 = -dot(eye_pos, -dir);

  return view;
}

struct view_frame {
  template <typename T>
  static scalar4x4<T> view_matrix(const scalar3<T> right,
                                  const scalar3<T> up,
                                  const scalar3<T> dir,
                                  const scalar3<T> origin) noexcept;

  template <typename T>
  static scalar4x4<T> look_at(const scalar3<T>& eye_pos,
                              const scalar3<T>& target,
                              const scalar3<T>& world_up) noexcept {
    const auto direction = normalize(target - eye_pos);

    assert(!is_zero_length(direction) && "Direction vector cannot be null!");
    assert(!are_parallel(direction, world_up) &&
           "Direction vector and world up vector cannot be parallel!");

    const auto right = normalize(cross(world_up, direction));
    const auto up    = cross(direction, right);

    return view_matrix(right, up, direction, eye_pos);
  }
};

template <typename T>
scalar4x4<T> view_frame::view_matrix(const scalar3<T> right,
                                     const scalar3<T> up,
                                     const scalar3<T> dir,
                                     const scalar3<T> origin) noexcept {

  assert(are_orthogonal(right, up) &&
         "Right and up vector must be orthogonal !");
  assert(are_orthogonal(right, dir) &&
         "Right and direction vector must be orthogonal !");
  assert(are_orthogonal(dir, up) &&
         "Direction and up vector must be orthogonal !");

  return {// 1st row
          right.x,
          right.y,
          right.z,
          -dot(right, origin),
          // 2nd row
          up.x,
          up.y,
          up.z,
          -dot(up, origin),
          // 3rd row
          dir.x,
          dir.y,
          dir.z,
          -dot(dir, origin),
          // 4th row
          T{},
          T{},
          T{},
          T{1}};
}

struct projections_rh {

  template <typename T>
  inline static scalar4x4<T> perspective_symmetric(const T width,
                                                   const T height,
                                                   const T fov,
                                                   const T near_plane,
                                                   const T far_plane) noexcept {
    return perspective_symmetric(width / height, fov, near_plane, far_plane);
  }

  template <typename T>
  static scalar4x4<T> perspective_symmetric(const T aspect_ratio,
                                            const T fov,
                                            const T near_plane,
                                            const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> perspective(const T left,
                                  const T right,
                                  const T top,
                                  const T bottom,
                                  const T near_plane,
                                  const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> orthographic(const T left,
                                   const T right,
                                   const T top,
                                   const T bottom,
                                   const T near_plane,
                                   const T far_plane) noexcept;
};

template <typename T>
scalar4x4<T> projections_rh::perspective_symmetric(const T aspect_ratio,
                                                   const T fov,
                                                   const T near_plane,
                                                   const T far_plane) noexcept {
  const auto d = T(1.0) / std::tan(fov * T(0.5));

  scalar4x4<T> result{scalar4x4<T>::stdc::identity};
  result.a00 = d / aspect_ratio;
  result.a11 = d;

  result.a22 = (near_plane + far_plane) / (near_plane - far_plane);
  result.a23 = (T(2) * near_plane * far_plane) / (near_plane - far_plane);

  result.a32 = T(-1);
  result.a33 = T(0);

  return result;
}

template <typename T>
scalar4x4<T> projections_rh::perspective(const T left,
                                         const T right,
                                         const T top,
                                         const T bottom,
                                         const T near_plane,
                                         const T far_plane) noexcept {

  scalar4x4<T> result{scalar4x4<T>::stdc::identity};

  result.a00 = (T(2) * near_plane) / (right - left);
  result.a02 = (right + left) / (right - left);

  result.a11 = (T(2) * near_plane) / (top - bottom);
  result.a12 = (top + bottom) / (top - bottom);

  result.a22 = (near_plane + far_plane) / (near_plane - far_plane);
  result.a23 = (T(2) * near_plane * far_plane) / (near_plane - far_plane);

  result.a32 = T(-1);
  result.a33 = T(0);

  return result;
}

template <typename T>
scalar4x4<T> projections_rh::orthographic(const T left,
                                          const T right,
                                          const T top,
                                          const T bottom,
                                          const T near_plane,
                                          const T far_plane) noexcept {
  scalar4x4<T> result{scalar4x4<T>::stdc::identity};

  result.a00 = T(2) / (right - left);
  result.a03 = -(right + left) / (right - left);

  result.a11 = T(2) / (top - bottom);
  result.a13 = -(top + bottom) / (top - bottom);

  result.a22 = T(-2) / (far_plane - near_plane);
  result.a23 = -(far_plane + near_plane) / (far_plane - near_plane);

  return result;
}

/// \brief  Builds projection matrices assuming a left handed coordinate
///         system.
struct projections_lh {

  template <typename T>
  inline static scalar4x4<T> perspective_symmetric(const T width,
                                                   const T height,
                                                   const T fov,
                                                   const T near_plane,
                                                   const T far_plane) noexcept {
    return perspective_symmetric(width / height, fov, near_plane, far_plane);
  }

  template <typename T>
  static scalar4x4<T> perspective_symmetric(const T aspect_ratio,
                                            const T fov,
                                            const T near_plane,
                                            const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> perspective(const T left,
                                  const T right,
                                  const T top,
                                  const T bottom,
                                  const T near_plane,
                                  const T far_plane) noexcept;

  template <typename T>
  static scalar4x4<T> orthographic(const T left,
                                   const T right,
                                   const T top,
                                   const T bottom,
                                   const T near_plane,
                                   const T far_plane) noexcept;
};

template <typename T>
scalar4x4<T> projections_lh::perspective_symmetric(const T aspect_ratio,
                                                   const T fov,
                                                   const T near_plane,
                                                   const T far_plane) noexcept {
  //
  // distance to projection plane
  const auto d = T(1) / std::tan(fov / T(2));

  auto proj = scalar4x4<T>::stdc::null;

  proj.a00 = d / aspect_ratio;

  proj.a11 = d;

  proj.a22 = (far_plane) / (far_plane - near_plane);
  proj.a23 = (-near_plane * far_plane) / (far_plane - near_plane);

  proj.a32 = T(1);

  return proj;
}

template <typename T>
scalar4x4<T> projections_lh::perspective(const T left,
                                         const T right,
                                         const T top,
                                         const T bottom,
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
scalar4x4<T> projections_lh::orthographic(const T left,
                                          const T right,
                                          const T top,
                                          const T bottom,
                                          const T near_plane,
                                          const T far_plane) noexcept {

  const auto x_diff = right - left;
  const auto x_sum  = right + left;
  const auto y_diff = top - bottom;
  const auto y_sum  = top + bottom;
  const auto z_diff = far_plane - near_plane;

  return {// 1st row
          T(2) / x_diff,
          T{},
          T{},
          -(x_sum / x_diff),
          // 2nd row
          T{},
          T(2) / y_diff,
          T(0),
          -(y_sum / y_diff),
          // 3rd row
          T(0),
          T(0),
          T(1) / z_diff,
          -(near_plane / z_diff),
          // 4th row
          T(0),
          T(0),
          T(0),
          T(1)};
}

/// @}

} // namespace math
} // namespace xray
