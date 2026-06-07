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

#include <cassert>

#include "xray/base/array_dimension.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template<typename T>
bool
operator==(const scalar4x4<T>& lhs, const scalar4x4<T>& rhs) noexcept
{
    for (size_t i = 0; i < XR_COUNTOF(lhs.components); ++i) {
        if (!is_equal(lhs.components[i], rhs.components[i])) {
            return false;
        }
    }

    return true;
}

template<typename T>
inline bool
operator!=(const scalar4x4<T>& lhs, const scalar4x4<T>& rhs) noexcept
{
    return !(lhs == rhs);
}

template<typename T>
scalar4x4<T>
operator+(const scalar4x4<T>& lhs, const scalar4x4<T>& rhs) noexcept
{
    auto result = lhs;
    result += rhs;
    return result;
}

template<typename T>
scalar4x4<T>
operator-(const scalar4x4<T>& lhs, const scalar4x4<T>& rhs) noexcept
{
    auto result = lhs;
    result -= rhs;
    return result;
}

template<typename T>
scalar4x4<T>
operator-(const scalar4x4<T>& mtx) noexcept
{
    // clang-format off
  return {-mtx.a00, -mtx.a01, -mtx.a02, -mtx.a03,
          -mtx.a10, -mtx.a11, -mtx.a12, -mtx.a13,
          -mtx.a20, -mtx.a21, -mtx.a22, -mtx.a23,
          -mtx.a30, -mtx.a31, -mtx.a32, -mtx.a33};
    // clang-format on
}

template<typename T>
scalar4x4<T>
operator*(const scalar4x4<T>& lhs, const scalar4x4<T>& rhs) noexcept
{
    auto result = scalar4x4<T>::stdc::null;

    for (size_t row = 0; row < 4; ++row) {
        for (size_t col = 0; col < 4; ++col) {
            for (size_t k = 0; k < 4; ++k) {
                result(row, col) += lhs(row, k) * rhs(k, col);
            }
        }
    }

    return result;
}

template<typename T>
scalar4x4<T>
operator*(const T k, const scalar4x4<T>& mtx) noexcept
{
    scalar4x4<T> result{ mtx };
    result *= k;
    return result;
}

template<typename T>
scalar4x4<T>
operator*(const scalar4x4<T>& mtx, const T k) noexcept
{
    return k * mtx;
}

template<typename T>
scalar4x4<T>
operator/(const scalar4x4<T>& mtx, const T k) noexcept
{
    scalar4x4<T> result{ mtx };
    result /= k;
    return result;
}

template<typename T>
T
determinant(const scalar4x4<T>& m) noexcept
{
    //
    // This computation is based on Laplace's theorem
    // which states that the value of a determinant is equal to the product of
    // the minor determinants formed with the elements of p rows/columns and
    // their algebraic complements.
    const T a0 = m.a00 * m.a11 - m.a01 * m.a10;
    const T a1 = m.a00 * m.a12 - m.a02 * m.a10;
    const T a2 = m.a00 * m.a13 - m.a03 * m.a10;
    const T a3 = m.a01 * m.a12 - m.a02 * m.a11;
    const T a4 = m.a01 * m.a13 - m.a03 * m.a11;
    const T a5 = m.a02 * m.a13 - m.a03 * m.a12;
    const T b0 = m.a20 * m.a31 - m.a21 * m.a30;
    const T b1 = m.a20 * m.a32 - m.a22 * m.a30;
    const T b2 = m.a20 * m.a33 - m.a23 * m.a30;
    const T b3 = m.a21 * m.a32 - m.a22 * m.a31;
    const T b4 = m.a21 * m.a33 - m.a23 * m.a31;
    const T b5 = m.a22 * m.a33 - m.a23 * m.a32;
    return a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
}

template<typename T>
inline bool
is_invertible(const scalar4x4<T>& m) noexcept
{
    return !is_zero(determinant(m));
}

template<typename T>
scalar4x4<T>
invert(const scalar4x4<T>& m) noexcept
{
    const T a0 = m.a00 * m.a11 - m.a01 * m.a10;
    const T a1 = m.a00 * m.a12 - m.a02 * m.a10;
    const T a2 = m.a00 * m.a13 - m.a03 * m.a10;
    const T a3 = m.a01 * m.a12 - m.a02 * m.a11;
    const T a4 = m.a01 * m.a13 - m.a03 * m.a11;
    const T a5 = m.a02 * m.a13 - m.a03 * m.a12;
    const T b0 = m.a20 * m.a31 - m.a21 * m.a30;
    const T b1 = m.a20 * m.a32 - m.a22 * m.a30;
    const T b2 = m.a20 * m.a33 - m.a23 * m.a30;
    const T b3 = m.a21 * m.a32 - m.a22 * m.a31;
    const T b4 = m.a21 * m.a33 - m.a23 * m.a31;
    const T b5 = m.a22 * m.a33 - m.a23 * m.a32;

    const T det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
    if (det != T(0)) {
        const T invdet = T(1) / det;

        // clang-format off
        return scalar4x4{
            //
            // 
            (+m.a11 * b5 - m.a12 * b4 + m.a13 * b3) * invdet,
            (-m.a01 * b5 + m.a02 * b4 - m.a03 * b3) * invdet,
            (+m.a31 * a5 - m.a32 * a4 + m.a33 * a3) * invdet,
            (-m.a21 * a5 + m.a22 * a4 - m.a23 * a3) * invdet,

            // 
            //
            (-m.a10 * b5 + m.a12 * b2 - m.a13 * b1) * invdet,
            (+m.a00 * b5 - m.a02 * b2 + m.a03 * b1) * invdet,
            (-m.a30 * a5 + m.a32 * a2 - m.a33 * a1) * invdet,
            (+m.a20 * a5 - m.a22 * a2 + m.a23 * a1) * invdet,

            //
            //
            (+m.a10 * b4 - m.a11 * b2 + m.a13 * b0) * invdet,
            (-m.a00 * b4 + m.a01 * b2 - m.a03 * b0) * invdet,
            (+m.a30 * a4 - m.a31 * a2 + m.a33 * a0) * invdet,
            (-m.a20 * a4 + m.a21 * a2 - m.a23 * a0) * invdet,

            //
            //
            (-m.a10 * b3 + m.a11 * b1 - m.a12 * b0) * invdet,
            (+m.a00 * b3 - m.a01 * b1 + m.a02 * b0) * invdet,
            (-m.a30 * a3 + m.a31 * a1 - m.a32 * a0) * invdet,
            (+m.a20 * a3 - m.a21 * a1 + m.a22 * a0) * invdet
        };
        // clang-format on
    } else {
        return scalar4x4<T>::stdc::null;
    }
}

template<typename T>
scalar4x4<T>
adjoint(const scalar4x4<T>& m) noexcept
{

    const T a0 = m.a00 * m.a11 - m.a1 * m.a10;
    const T a1 = m.a00 * m.a12 - m.a2 * m.a10;
    const T a2 = m.a00 * m.a13 - m.a3 * m.a10;
    const T a3 = m.a01 * m.a12 - m.a2 * m.a11;
    const T a4 = m.a01 * m.a13 - m.a3 * m.a11;
    const T a5 = m.a02 * m.a13 - m.a3 * m.a12;
    const T b0 = m.a20 * m.a31 - m.a1 * m.a30;
    const T b1 = m.a20 * m.a32 - m.a2 * m.a30;
    const T b2 = m.a20 * m.a33 - m.a3 * m.a30;
    const T b3 = m.a21 * m.a32 - m.a2 * m.a31;
    const T b4 = m.a21 * m.a33 - m.a3 * m.a31;
    const T b5 = m.a22 * m.a33 - m.a3 * m.a32;

    // clang-format off
    return scalar4x4<T> {
            +m.a11 * b5 - m.a12 * b4 + m.a13 * b3,
            -m.a01 * b5 + m.a02 * b4 - m.a03 * b3,
            +m.a31 * a5 - m.a32 * a4 + m.a33 * a3,
            -m.a21 * a5 + m.a22 * a4 - m.a23 * a3,
            -m.a10 * b5 + m.a12 * b2 - m.a13 * b1,
            +m.a00 * b5 - m.a02 * b2 + m.a03 * b1,
            -m.a30 * a5 + m.a32 * a2 - m.a33 * a1,
            +m.a20 * a5 - m.a22 * a2 + m.a23 * a1,
            +m.a10 * b4 - m.a11 * b2 + m.a13 * b0,
            -m.a00 * b4 + m.a01 * b2 - m.a03 * b0,
            +m.a30 * a4 - m.a31 * a2 + m.a33 * a0,
            -m.a20 * a4 + m.a21 * a2 - m.a23 * a0,
            -m.a10 * b3 + m.a11 * b1 - m.a12 * b0,
            +m.a00 * b3 - m.a01 * b1 + m.a02 * b0,
            -m.a30 * a3 + m.a31 * a1 - m.a32 * a0,
            +m.a20 * a3 - m.a21 * a1 + m.a22 * a0
    };
    // clang-format on
}

template<typename T>
scalar4x4<T>
transpose(const scalar4x4<T>& m) noexcept
{
    // clang-format off

  return {m.a00, m.a10, m.a20, m.a30,
          m.a01, m.a11, m.a21, m.a31,
          m.a02, m.a12, m.a22, m.a32,
          m.a03, m.a13, m.a23, m.a33};

    // clang-format on
}

template<typename T>
scalar3<T>
mul_vec(const scalar4x4<T>& m, const scalar3<T>& v) noexcept
{
    return { m.a00 * v.x + m.a01 * v.y + m.a02 * v.z,
             m.a10 * v.x + m.a11 * v.y + m.a12 * v.z,
             m.a20 * v.x + m.a21 * v.y + m.a22 * v.z };
}

template<typename T>
scalar3<T>
mul_point(const scalar4x4<T>& m, const scalar3<T>& p) noexcept
{
    return { m.a00 * p.x + m.a01 * p.y + m.a02 * p.z + m.a03,
             m.a10 * p.x + m.a11 * p.y + m.a12 * p.z + m.a13,
             m.a20 * p.x + m.a21 * p.y + m.a22 * p.z + m.a23 };
}

template<typename T>
scalar4<T>
mul_vec(const scalar4x4<T>& m, const scalar4<T>& v) noexcept
{
    return { m.a00 * v.x + m.a01 * v.y + m.a02 * v.z,
             m.a10 * v.x + m.a11 * v.y + m.a12 * v.z,
             m.a20 * v.x + m.a21 * v.y + m.a22 * v.z,
             T(0) };
}

template<typename T>
scalar4<T>
mul_point(const scalar4x4<T>& m, const scalar4<T>& p) noexcept
{
    return { m.a00 * p.x + m.a01 * p.y + m.a02 * p.z + m.a03,
             m.a10 * p.x + m.a11 * p.y + m.a12 * p.z + m.a13,
             m.a20 * p.x + m.a21 * p.y + m.a22 * p.z + m.a23,
             T(1) };
}

/// \brief      Multiply homogeneous point (w != 1).
template<typename T>
scalar4<T>
mul_hpoint(const scalar4x4<T>& m, const scalar4<T>& p) noexcept
{
    return { m.a00 * p.x + m.a01 * p.y + m.a02 * p.z + m.a03 * p.w,
             m.a10 * p.x + m.a11 * p.y + m.a12 * p.z + m.a13 * p.w,
             m.a20 * p.x + m.a21 * p.y + m.a22 * p.z + m.a23 * p.w,
             m.a30 * p.x + m.a31 * p.y + m.a32 * p.z + m.a33 * p.w };
}

/// @}

} // namespace math
} // namespace xray
