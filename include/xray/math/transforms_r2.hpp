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
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2x3.hpp"
#include <cmath>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

/// \brief Construction of matrices for common R2 transforms.
struct R2 {

  /// \brief      Builds a translation matrix
  template <typename T>
  static scalar2x3<T> translate(const T tx, const T ty) noexcept;

  /// \brief      Builds a translation matrix
  template <typename T>
  static scalar2x3<T> translate(const scalar2<T>& v) noexcept;

  /// \brief      Builds a uniform scaling matrix.
  template <typename T>
  static scalar2x3<T> scale(const T factor) noexcept;

  /// \brief      Builds a non uniform scaling matrix.
  template <typename T>
  static scalar2x3<T> scale(const T sx, const T sy) noexcept;

  /// \brief      Builds a matrix for a rotation at the origin.
  template <typename T>
  static scalar2x3<T> rotate(const T angle_rads) noexcept;

  ///
  /// \brief      Builds a matrix for a rotation where the origin of the
  ///             rotation does not correspond with the origin of the
  ///             corrdinate frame.
  template <typename T>
  static scalar2x3<T> rotate_off_center(const T angle_rads, const T tx,
                                        const T ty) noexcept;

  ///
  /// \brief      Builds a matrix for a rotation where the origin of the
  ///             rotation does not correspond with the origin of the
  ///             corrdinate frame.
  template <typename T>
  static scalar2x3<T> rotate_off_center(const T           angle_rads,
                                        const scalar2<T>& center) noexcept;

  /// \brief Shear along x axis.
  template <typename T>
  static scalar2x3<T> shear_x(const T k) noexcept;

  /// \brief Shear along y axis.
  template <typename T>
  static scalar2x3<T> shear_y(const T k) noexcept;

  /// \brief Shear along x and y axis.
  template <typename T>
  static scalar2x3<T> shear_xy(const T kx, const T ky) noexcept;

  //
  //    TO DO : add refect + shear
};

template <typename T>
scalar2x3<T> R2::translate(const T x, const T y) noexcept {
  return {T(1), T(0), x, T(0), T(1), y};
}

template <typename T>
scalar2x3<T> R2::translate(const scalar2<T>& v) noexcept {
  return translate(v.x, v.y);
}

template <typename T>
scalar2x3<T> R2::scale(const T factor) noexcept {
  return {factor, T(0), T(0), T(0), factor, T(0)};
}

template <typename T>
scalar2x3<T> R2::scale(const T sx, const T sy) noexcept {
  return {sx, T(0), T(0), T(0), sy, T(0)};
}

template <typename T>
scalar2x3<T> R2::rotate(const T angle_rads) noexcept {
  const auto ct = std::cos(angle_rads);
  const auto st = std::sin(angle_rads);

  return {ct, -st, T(0), st, ct, T(0)};
}

template <typename T>
scalar2x3<T> R2::rotate_off_center(const T angle_rads, const T tx,
                                   const T ty) noexcept {
  const auto ct = std::cos(angle_rads);
  const auto st = std::cos(angle_rads);

  return {ct, -st, tx * (T(1) - ct) + st * ty,
          st, +ct, ty * (T(1) - ct) - st * tx};
}

template <typename T>
scalar2x3<T> R2::rotate_off_center(const T           angle_rads,
                                   const scalar2<T>& center) noexcept {
  return rotate_off_center(angle_rads, center.x, center.y);
}

template <typename T>
scalar2x3<T> R2::shear_x(const T k) noexcept {
  return {T(1), k, T(0), T(0), T(1), T(0)};
}

template <typename T>
scalar2x3<T> R2::shear_y(const T k) noexcept {
  return {T(1), T(0), T(0), k, T(1), T(0)};
}

template <typename T>
scalar2x3<T> R2::shear_xy(const T kx, const T ky) noexcept {
  return {T(1), T(kx), T(0), ky, T(1), T(0)};
}

/// @}

} // namespace math
} // namespace xray
