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
#include "xray/base/array_dimension.hpp"
#include "xray/base/shims/stl_type_traits_shims.hpp"
#include "xray/xray_types.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

/// \class      scalar2x3
/// \brief      A 2x3 matrix representing affine transformations in R2.
///             It uses a vector on the right convention for multiplying
///             points/vectors.
template <typename T>
struct scalar2x3 {

  static_assert(base::std_is_arithmetic<T>,
                "template parameter must be an arithmetic type !");

  union {

    struct {
      T a00, a01, a02;
      T a10, a11, a12;
    };

    T components[6];
  };

  /// \name Construction/destruction
  /// @{

public:
  scalar2x3() noexcept = default;

  constexpr scalar2x3(const T e00, const T e01, const T e02, const T e10,
                      const T e11, const T e12) noexcept;

  /// @}

  /// \name Assignment operators.
  /// @{

public:
  scalar2x3<T>& operator+=(const scalar2x3<T>& rhs) noexcept;

  scalar2x3<T>& operator-=(const scalar2x3<T>& rhs) noexcept;

  scalar2x3<T>& operator*=(const T k) noexcept;

  scalar2x3<T>& operator/=(const T k) noexcept;

  /// @}

  /// \name Standard 2x3 matrices.
  /// @{
public:
  struct stdc;
  /// @}
};

template <typename T>
struct scalar2x3<T>::stdc {
  ///< Null 2x3 matrix.
  static constexpr scalar2x3<T> null{T(0), T(0), T(0), T(0), T(0), T(0)};

  ///< Identity 2x3 matrix.
  static constexpr scalar2x3<T> identity{T(1), T(0), T(0), T(0), T(1), T(0)};
};

template <typename T>
constexpr scalar2x3<T> scalar2x3<T>::stdc::null;

template <typename T>
constexpr scalar2x3<T> scalar2x3<T>::stdc::identity;

using float2x3  = scalar2x3<scalar_lowp>;
using double2x3 = scalar2x3<scalar_mediump>;

template <typename T>
constexpr scalar2x3<T>::scalar2x3(const T e00, const T e01, const T e02,
                                  const T e10, const T e11,
                                  const T e12) noexcept
    : a00{e00}, a01{e01}, a02{e02}, a10{e10}, a11{e11}, a12{e12} {}

template <typename T>
scalar2x3<T>& scalar2x3<T>::operator+=(const scalar2x3<T>& rhs) noexcept {
  for (size_t i = 0; i < XR_COUNTOF__(components); ++i)
    components[i] += rhs.components[i];

  return *this;
}

template <typename T>
scalar2x3<T>& scalar2x3<T>::operator-=(const scalar2x3<T>& rhs) noexcept {
  for (size_t i = 0; i < XR_COUNTOF__(components); ++i)
    components[i] -= rhs.components[i];

  return *this;
}

template <typename T>
scalar2x3<T>& scalar2x3<T>::operator*=(const T k) noexcept {
  for (size_t i = 0; i < XR_COUNTOF__(components); ++i)
    components[i] *= k;

  return *this;
}

template <typename T>
scalar2x3<T>& scalar2x3<T>::operator/=(const T k) noexcept {
  for (size_t i = 0; i < XR_COUNTOF__(components); ++i)
    components[i] /= k;

  return *this;
}

template <typename T>
scalar2x3<T> operator-(const scalar2x3<T>& lhs) noexcept {
  return {-lhs.a00, -lhs.a01, -lhs.a02, -lhs.a10, -lhs.a11, -lhs.a12};
}

template <typename T>
scalar2x3<T> operator+(const scalar2x3<T>& lhs,
                       const scalar2x3<T>& rhs) noexcept {
  auto result = lhs;
  result += rhs;
  return result;
}

template <typename T>
scalar2x3<T> operator-(const scalar2x3<T>& lhs,
                       const scalar2x3<T>& rhs) noexcept {
  auto result = lhs;
  result -= rhs;
  return result;
}

template <typename T>
scalar2x3<T> operator*(const scalar2x3<T>& lhs, const T k) noexcept {
  auto result = lhs;
  result *= k;
  return result;
}

template <typename T>
scalar2x3<T> operator*(const T k, const scalar2x3<T>& rhs) noexcept {
  return rhs * k;
}

template <typename T>
scalar2x3<T> operator/(const scalar2x3<T>& lhs, const T k) noexcept {

  auto result = lhs;
  result /= k;
  return result;
}

template <typename T>
scalar2x3<T> operator*(const scalar2x3<T>& a, const scalar2x3<T>& b) noexcept {
  return {a.a00 * b.a00 + a.a01 * b.a10,         a.a00 * b.b01 + a.a01 * b.a11,
          a.a00 * b.a02 + a.a01 * b.a12 + a.a02,

          a.a10 * b.a00 + a.a11 * b.a10,         a.a10 * b.a01 + a.a11 * b.a11,
          a.a10 * b.a02 + a.a11 * b.a12 + a.a12};
}

///  @}

} // namespace math
} // namespace xray
