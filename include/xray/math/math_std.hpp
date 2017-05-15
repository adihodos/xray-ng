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
#include "xray/math/constants.hpp"
#include <cassert>
#include <cmath>

namespace xray {
namespace math {
/// \addtogroup __GroupXrayMath
/// @{

/// \brief Restrics a value to a certain range.
template <typename T>
inline T clamp(const T val, const T min_val, const T max_val) noexcept {
  return val < min_val ? min_val : (val > max_val ? max_val : val);
}

template <typename T>
inline T min(const T a, const T b) noexcept {
  return a < b ? a : b;
}

template <typename T>
inline T max(const T a, const T b) noexcept {
  return a > b ? a : b;
}

/// \brief Performs a linear interpolation of two values.
/// \param a Start value.
/// \param b Final value.
/// \param u Interpolation factor. Must be in the [0, 1] range;
template <typename T, typename U>
inline T mix(const T& a, const T& b, const U u) noexcept {
  assert((u >= U(0)) && (u <= U(1)) &&
         "Interpolation factor must be in the [0, 1] range!");
  return a * (U(1) - u) + b * u;
}

/// \brief  Returns 1 when x >= a, 0 otherwise.
template <typename T>
constexpr inline T step(const T x, const T a) noexcept {
  return static_cast<T>(x >= a);
}

template <typename T>
constexpr inline T pulse(const T a, const T b, const T x) noexcept {
  return step(a, x) - step(b, x);
}

template <typename T>
inline T smoothstep(const T a, const T b, const T x) noexcept {
  if (x < a)
    return T(0);

  if (x >= b)
    return T(1);

  const auto nx = (x - a) / (b - a);
  return nx * nx * (T(3) - T(2) * nx);
}

inline float bias(const float b, const float x) noexcept {
  return pow(x, log(b) / log(0.5f));
}

inline float gain(const float g, const float x) noexcept {
  if (x < 0.5f)
    return bias(1.0f - g, 2.0f * x) / 2.0f;

  return 1.0f - bias(1.0f - g, 2.0f - 2.0f * x) / 2.0f;
}

/// \brief Returns the sign of x :
/// -1, if x < 0,
/// +1, if x >= 0
template <typename T>
inline constexpr T sgn(const T value) noexcept {
  return (value >= T(0)) - (value < 0);
}

template <typename T>
inline constexpr T radians(const T degrees) noexcept {
  return degrees * pi_over_180<T>;
}

template <typename T>
inline constexpr T degrees(const T radians) noexcept {
  return radians * one_eighty_over_pi<T>;
}

template <typename real_t>
real_t angle_from_xy(const real_t x, const real_t y) noexcept {
  real_t theta{};

  if (x >= real_t(0)) {
    theta = atan(y / x);
    if (theta < real_t(0)) {
      theta += two_pi<real_t>;
    }
  } else {
    theta = std::atan(y / x) + pi<real_t>;
  }

  return theta;
}

template <typename T>
inline T inv_sqrt(const T val) noexcept {
  return T{1} / sqrt(val);
}

/// @}

} // namespace math
} // namespace xray
