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

#include "xray/math/math_base.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/xray.hpp"
#include <cassert>
#include <cstddef>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

template <typename T>
scalar3x3<T> operator-(const scalar3x3<T> &input_mtx) noexcept {
  return {-input_mtx.a00, -input_mtx.a01, -input_mtx.a02,
          -input_mtx.a10, -input_mtx.a11, -input_mtx.a12,
          -input_mtx.a20, -input_mtx.a21, -input_mtx.a22};
}

template <typename T>
scalar3x3<T> operator+(const scalar3x3<T> &m0,
                       const scalar3x3<T> &m1) noexcept {
  auto result{m0};
  result += m1;
  return result;
}

template <typename T>
scalar3x3<T> operator-(const scalar3x3<T> &m0,
                       const scalar3x3<T> &m1) noexcept {
  auto result = m0;
  result -= m1;
  return result;
}

template <typename T>
scalar3x3<T> operator*(const scalar3x3<T> &m0,
                       const scalar3x3<T> &m1) noexcept {

  auto result = scalar3x3<T>::stdc::null;

  for (size_t row = 0; row < 3; ++row) {
    for (size_t col = 0; col < 3; ++col) {
      for (size_t k = 0; k < 3; ++k) {
        result(row, col) += m0(row, k) * m1(k, col);
      }
    }
  }

  return result;
}

template <typename T>
scalar3x3<T> operator*(const scalar3x3<T> &m, const T k) noexcept {
  auto result{m};
  result *= k;
  return result;
}

template <typename T>
scalar3x3<T> operator*(const T k, const scalar3x3<T> &m) noexcept {
  return m * k;
}

template <typename T>
scalar3x3<T> operator/(const scalar3x3<T> &m, const T k) noexcept {
  auto result{m};
  result /= k;
  return result;
}

/// \brief      Computes the determinant of a matrix.
template <typename T> T determinant(const scalar3x3<T> &m) noexcept {
  //
  //  expand by first row
  const auto det0 = m.a11 * m.a22 - m.a12 * m.a21;
  const auto det1 = m.a10 * m.a22 - m.a12 * m.a20;
  const auto det2 = m.a10 * m.a21 - m.a11 * m.a20;

  return m.a00 * det0 - m.a01 * det1 + m.a02 * det2;
}

/// \brief      Returns the adjoint matrix of the input matrix.
template <typename T> scalar3x3<T> adjoint(const scalar3x3<T> &m) noexcept {
  scalar3x3<T> adj{};

  adj.a00 = m.a11 * m.a22 - m.a21 * m.a12;
  adj.a01 = m.a12 * m.a20 - m.a10 * m.a22;
  adj.a02 = m.a10 * m.a21 - m.a11 * m.a20;

  adj.a10 = m.a02 * m.a21 - m.a01 * m.a22;
  adj.a11 = m.a00 * m.a22 - m.a02 * m.a20;
  adj.a12 = m.a01 * m.a20 - m.a00 * m.a21;

  adj.a20 = m.a01 * m.a12 - m.a02 * m.a11;
  adj.a21 = m.a02 * m.a10 - m.a00 * m.a12;
  adj.a22 = m.a01 * m.a11 - m.a01 * m.a10;

  return adj;
}

/// \brief Compute the inverse of a matrix.
template <typename T> scalar3x3<T> invert(const scalar3x3<T> &m) noexcept {
  const auto det_m = determinant(m);

  if (is_zero(det_m)) {
    assert(false && "Matrix is not invertible!!");
    return scalar3x3<T>::math::null;
  }

  return adjoint(m) / det_m;
}

/// \brief      Applies a transformation to a vector in R3.
template <typename T>
scalar3<T> mul_vec(const scalar3x3<T> &m, const scalar3<T> &v) noexcept {
  return {m.a00 * v.x + m.a01 * v.y + m.a02 * v.z,
          m.a10 * v.x + m.a11 * v.y + m.a12 * v.z,
          m.a20 * v.x + m.a21 * v.y + m.a22 * v.z};
}

/// \brief      Applies a linear transformation a point in R3;
template <typename T>
scalar3<T> mul_point(const scalar3x3<T> &m, const scalar3<T> &p) noexcept {
  return mul_vec(m, p);
}

template <typename T> scalar3x3<T> transpose(const scalar3x3<T> &m) noexcept {
  return {m.a00, m.a10, m.a20, m.a01, m.a11, m.a21, m.a02, m.a12, m.a22};
}

/// @}

} // namespace math
} // namespace xray
