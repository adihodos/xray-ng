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
#include "xray/xray_types.hpp"
#include <cstdint>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template <typename T>
struct scalar3 {

  union {

    struct {
      T x;
      T y;
      T z;
    };

    struct {
      T u;
      T v;
      T w;
    };

    struct {
      T s;
      T t;
      T p;
    };

    T components[3];
  };

  using class_type = scalar3<T>;

  scalar3() noexcept = default;

  constexpr scalar3(const T xval, const T yval, const T zval) noexcept
      : x{xval}, y{yval}, z{zval} {}

  class_type& operator+=(const class_type& rhs) noexcept {
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
  }

  class_type& operator-=(const class_type& rhs) noexcept {
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
  }

  class_type& operator*=(const T scalar) noexcept {
    x *= scalar;
    y *= scalar;
    z *= scalar;
    return *this;
  }

  class_type& operator/=(const T scalar) noexcept {
    x /= scalar;
    y /= scalar;
    z /= scalar;
    return *this;
  }

  /// \name Standard constants for R3 vectors.
  /// @{

public:
  struct stdc;

  /// @}
};

template <typename T>
struct scalar3<T>::stdc {
  static constexpr scalar3<T> unit_x{T(1.0), T(0.0), T(0.0)};
  static constexpr scalar3<T> unit_y{T(0.0), T(1.0), T(0.0)};
  static constexpr scalar3<T> unit_z{T(0.0), T(0.0), T(1.0)};
  static constexpr scalar3<T> zero{T(0.0), T(0.0), T(0.0)};
  static constexpr scalar3<T> one{T(1.0), T(1.0), T(1.0)};
};

template <typename T>
constexpr scalar3<T> scalar3<T>::stdc::unit_x;

template <typename T>
constexpr scalar3<T> scalar3<T>::stdc::unit_y;

template <typename T>
constexpr scalar3<T> scalar3<T>::stdc::unit_z;

template <typename T>
constexpr scalar3<T> scalar3<T>::stdc::zero;

template <typename T>
constexpr scalar3<T> scalar3<T>::stdc::one;

using float3     = scalar3<scalar_lowp>;
using double3    = scalar3<scalar_mediump>;
using scalar3u8  = scalar3<uint8_t>;
using scalar3u32 = scalar3<uint32_t>;

/// @}

} // namespace math
} // namespace xray
