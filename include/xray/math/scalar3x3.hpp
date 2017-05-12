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
#include "xray/base/algorithms/copy_pod_range.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/shims/stl_type_traits_shims.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/vector_type_tag.hpp"
#include "xray/xray_types.hpp"
#include <cassert>
#include <cstddef>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

/// \brief   Represents a 3 by 3 matrix, stored in row major format.
///          It follows the conventions that the matrix goes on the left side
///          when multiplying vectors. Also, when concatenating matrices
///          representing multiple transformations, the order must be from
///          right to left, eg : if we have a rotation R, followed by a scaling
///          S and a reflection RF (R, S, RF) then the matrices need to be
///          concatenated like this : FinalTransform = RF * S * R.
template <typename T>
class scalar3x3 {

  /// \name Data members and defined types.
  /// @{
public:
  using class_type = scalar3x3<T>;

  union {

    struct {
      T a00, a01, a02;
      T a10, a11, a12;
      T a20, a21, a22;
    };

    T components[9];
  };

  /// @}

  /// \name Constructors
  /// @{

public:
  /// \brief Default constructor. Leaves components uninitialized.
  scalar3x3() noexcept = default;

  constexpr scalar3x3(const T a00_, const T a01_, const T a02_, const T a10_,
                      const T a11_, const T a12_, const T a20_, const T a21_,
                      const T a22_) noexcept;

  constexpr scalar3x3(const T (&arr)[9]) noexcept;

  /// \brief Constructs from an array of values.
  scalar3x3(const T* input, const size_t count) noexcept;

  /// \brief Construct a diagonal matrix, setting A(i,j) = 0 for every i <> j.
  constexpr scalar3x3(const T a01_, const T a11_, const T a22_) noexcept;

  /// \brief  Construct from 3 vectors representing orientation.
  template <typename VecType>
  scalar3x3(const scalar3<T>& x_axis, const scalar3<T>& y_axis,
            const scalar3<T>& z_axis, VecType tp) noexcept;

  /// \brief      Construct a matrix representing the tensor product of u and w.
  scalar3x3(const scalar3<T>& u, const scalar3<T>& w) noexcept;

  /// @}

  /// \name   Element access operators.
  /// @{

public:
  T& operator()(const size_t row, const size_t col) noexcept {
    return components[index_at(row, col)];
  }

  T operator()(const size_t row, const size_t col) const noexcept {
    return components[index_at(row, col)];
  }

  /// @}

  /// \name Self assign operators
  /// @{

public:
  class_type& operator+=(const class_type& rhs) noexcept;

  class_type& operator-=(const class_type& rhs) noexcept;

  class_type& operator*=(const T k) noexcept;

  class_type& operator/=(const T k) noexcept;

  /// @}

private:
  void set(const scalar3<T>& x_axis, const scalar3<T>& y_axis,
           const scalar3<T>& z_axis, col_tag) noexcept;

  void set(const scalar3<T>& x_axis, const scalar3<T>& y_axis,
           const scalar3<T>& z_axis, row_tag) noexcept;

  size_t index_at(const size_t row, const size_t col) const noexcept {
    assert((row >= 0) && (row <= 2) && "Row must be in the [0, 2] range");
    assert((col >= 0) && (col <= 2) && "Row must be in the [0, 2] range");

    return row * 3 + col;
  }

  /// \name Standard R3 matrices
  /// @{
public:
  struct stdc;
  /// @}
};

template <typename T>
struct scalar3x3<T>::stdc {
  ///< Null 3x3 matrix.
  static constexpr const scalar3x3<T> null{T(0), T(0), T(0), T(0), T(0),
                                           T(0), T(0), T(0), T(0)};

  ///< Identity 3x3 matrix.
  static constexpr const scalar3x3<T> identity{T(1), T(0), T(0), T(0), T(1),
                                               T(0), T(0), T(0), T(1)};
};

template <typename T>
constexpr const scalar3x3<T> scalar3x3<T>::stdc::null;

template <typename T>
constexpr const scalar3x3<T> scalar3x3<T>::stdc::identity;

using mat3f = scalar3x3<float>;
using mat3d = scalar3x3<double>;

template <typename T>
constexpr scalar3x3<T>::scalar3x3(const T a00_, const T a01_, const T a02_,
                                  const T a10_, const T a11_, const T a12_,
                                  const T a20_, const T a21_,
                                  const T a22_) noexcept
    : a00{a00_}
    , a01{a01_}
    , a02{a02_}
    , a10{a10_}
    , a11{a11_}
    , a12{a12_}
    , a20{a20_}
    , a21{a21_}
    , a22{a22_} {}

template <typename T>
constexpr scalar3x3<T>::scalar3x3(const T a01_, const T a11_,
                                  const T a22_) noexcept
    : scalar3x3{a01_, T(0), T(0), T(0), a11_, T(0), T(0), T(0), a22_} {}

template <typename T>
constexpr scalar3x3<T>::scalar3x3(const T (&arr)[9]) noexcept
    : scalar3x3{arr[0], arr[1], arr[2], arr[3], arr[4],
                arr[5], arr[6], arr[7], arr[8]} {}

template <typename T>
scalar3x3<T>::scalar3x3(const T* input, const size_t count) noexcept {
  base::copy_pod_range(input, math::min(XR_COUNTOF(components), count),
                       components);
}

template <typename T>
template <typename VecType>
scalar3x3<T>::scalar3x3(const scalar3<T>& x_axis, const scalar3<T>& y_axis,
                        const scalar3<T>& z_axis, VecType tp) noexcept {
  set(x_axis, y_axis, z_axis, tp);
}

template <typename T>
scalar3x3<T>::scalar3x3(const scalar3<T>& u, const scalar3<T>& w) noexcept {
  a00 = u.x * w.x;
  a01 = u.x * w.y;
  a02 = u.x * w.z;
  a10 = u.y * w.x;
  a11 = u.y * w.y;
  a12 = u.y * w.z;
  a20 = u.z * w.x;
  a21 = u.z * w.y;
  a22 = u.z * w.z;
}

template <typename T>
void scalar3x3<T>::set(const scalar3<T>& x_axis, const scalar3<T>& y_axis,
                       const scalar3<T>& z_axis, col_tag) noexcept {
  a00 = x_axis.x;
  a01 = y_axis.x;
  a02 = z_axis.x;
  a10 = x_axis.y;
  a11 = y_axis.y;
  a12 = z_axis.y;
  a20 = x_axis.z;
  a21 = y_axis.z;
  a22 = z_axis.z;
}

template <typename T>
void scalar3x3<T>::set(const scalar3<T>& x_axis, const scalar3<T>& y_axis,
                       const scalar3<T>& z_axis, row_tag) noexcept {
  a00 = x_axis.x;
  a01 = x_axis.y;
  a02 = x_axis.z;
  a10 = y_axis.x;
  a11 = y_axis.y;
  a12 = y_axis.z;
  a20 = z_axis.x;
  a21 = z_axis.y;
  a22 = z_axis.z;
}

template <typename T>
scalar3x3<T>& scalar3x3<T>::operator+=(const scalar3x3<T>& rhs) noexcept {
  for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
    components[i] += rhs.components[i];
  }

  return *this;
}

template <typename T>
scalar3x3<T>& scalar3x3<T>::operator-=(const scalar3x3<T>& rhs) noexcept {
  for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
    components[i] -= rhs.components[i];
  }

  return *this;
}

template <typename T>
scalar3x3<T>& scalar3x3<T>::operator*=(const T scalar) noexcept {
  for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
    components[i] *= scalar;
  }

  return *this;
}

template <typename T>
scalar3x3<T>& scalar3x3<T>::operator/=(const T scalar) noexcept {
  for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
    components[i] /= scalar;
  }

  return *this;
}

/// @}

} // namespace math
} // namespace xray
