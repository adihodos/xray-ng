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

#include "xray/base/algorithms/copy_pod_range.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/vector_type_tag.hpp"
#include "xray/xray.hpp"
#include <cassert>
#include <cstring>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

/// \brief   A 4x4 matrix, stored using in row major format.
/// \remarks When concatenating matrices that represent
///          a sequence of transformations, care must be taken so
///          that the final matrix is built properly. Let M0, M1, ... Mn-1, Mn
///          be matrices representing a sequence of transormations to be applied
///          in this order. Then the proper order of concatenating them into a
///          single matrix is T = Mn * Mn - 1 * ... * M2 * M1 * M0 (right to
///          left).
///          The matrix follows the convention that it multiplies column vectors
///          (the vector is on the right side). Given a matrix \a M
///          and a vector \a V, to transform the vector \a V by the matrix \a M,
///          one would write \a V1 = \a M * \a V.
///          Note that access to individual elements using the function call
///          operator syntax uses 0 based indices.
template<typename T>
class scalar4x4
{
  private:
    size_t index_at(const size_t row, const size_t col) const noexcept
    {
        assert((row >= 0) && (row <= 3) && "Row must be in the [0, 3] range");
        assert((col >= 0) && (col <= 3) && "Row must be in the [0, 3] range");

        return row * 4 + col;
    }

  private:
    void set(const scalar3<T>& x_axis,
             const scalar3<T>& y_axis,
             const scalar3<T>& z_axis,
             const scalar3<T>& origin,
             col_tag) noexcept;

    void set(const scalar3<T>& x_axis,
             const scalar3<T>& y_axis,
             const scalar3<T>& z_axis,
             const scalar3<T>& origin,
             row_tag) noexcept;

  public:
    ///< Fully qualified type of this class.
    using class_type = scalar4x4<T>;

    union
    {

        struct
        {
            T a00, a01, a02, a03;
            T a10, a11, a12, a13;
            T a20, a21, a22, a23;
            T a30, a31, a32, a33;
        };

        T components[16]; ///< Access to elements using an array
    };

    /// Default constructor. Leaves elements uninitialized.
    scalar4x4() noexcept = default;

    constexpr scalar4x4(const T a00_,
                        const T a01_,
                        const T a02_,
                        const T a03_,

                        const T a10_,
                        const T a11_,
                        const T a12_,
                        const T a13_,

                        const T a20_,
                        const T a21_,
                        const T a22_,
                        const T a23_,

                        const T a30_,
                        const T a31_,
                        const T a32_,
                        const T a33_)

        noexcept
        : a00{ a00_ }
        , a01{ a01_ }
        , a02{ a02_ }
        , a03{ a03_ }

        , a10{ a10_ }
        , a11{ a11_ }
        , a12{ a12_ }
        , a13{ a13_ }

        , a20{ a20_ }
        , a21{ a21_ }
        , a22{ a22_ }
        , a23{ a23_ }

        , a30{ a30_ }
        , a31{ a31_ }
        , a32{ a32_ }
        , a33{ a33_ }

    {
    }

    /// Constructs from an array of values.
    scalar4x4(const T* input, const size_t count) noexcept;

    constexpr scalar4x4(const T (&arr)[16]) noexcept;

    /// \brief  Construct from a 3x3 linear transform matrix.
    scalar4x4(const scalar3x3<T>& linear_tf) noexcept;

    ///   Constructs a diagonal matrix.
    constexpr scalar4x4(const T a11, const T a22, const T a33, const T a44 = T{ 1 }) noexcept;

    /// \brief  Construct from three vectors, representing orientation.
    template<typename vector_type_tag>
    scalar4x4(const scalar3<T>& x_axis,
              const scalar3<T>& y_axis,
              const scalar3<T>& z_axis,
              vector_type_tag tp) noexcept;

    /// \brief  Construct from 3 vectors representing orientation and a point
    /// representing the origin.
    template<typename vector_type_tag>
    scalar4x4(const scalar3<T>& x_axis,
              const scalar3<T>& y_axis,
              const scalar3<T>& z_axis,
              const scalar3<T>& origin,
              vector_type_tag tp) noexcept;

    /// \brief Component read-write access, using the [row][column] syntax.
    /// \param row Component's row index (0 based).
    /// \param col Component's column index (0 based).
    T& operator()(const size_t row, const size_t col) noexcept { return components[index_at(row, col)]; }

    /// \brief Component read only access, using the [row][column] syntax.
    /// \param row Component's row index (0 based).
    /// \param col Component's column index (0 based).
    T operator()(const size_t row, const size_t col) const noexcept { return components[index_at(row, col)]; }

  public:
    class_type& operator+=(const class_type& rhs) noexcept;

    class_type& operator-=(const class_type& rhs) noexcept;

    class_type& operator*=(const T k) noexcept;

    class_type& operator/=(const T k) noexcept;

    /// @}

    /// \name Standard R4 matrices.
    /// @{
  public:
    struct stdc;

    /// @}
};

template<typename T>
struct scalar4x4<T>::stdc
{
    /// \brief      Null 4x4 matrix.
    static constexpr const scalar4x4<T> null{ T(0), T(0), T(0), T(0),

                                              T(0), T(0), T(0), T(0),

                                              T(0), T(0), T(0), T(0),

                                              T(0), T(0), T(0), T(0) };

    /// \brief      Identity 4x4 matrix.
    static constexpr const scalar4x4<T> identity{ T(1), T(0), T(0), T(0),

                                                  T(0), T(1), T(0), T(0),

                                                  T(0), T(0), T(1), T(0),

                                                  T(0), T(0), T(0), T(1) };
};

template<typename T>
constexpr const scalar4x4<T> scalar4x4<T>::stdc::null;

template<typename T>
constexpr const scalar4x4<T> scalar4x4<T>::stdc::identity;

using mat4f = scalar4x4<float>;
using mat4d = scalar4x4<double>;

template<typename T>
scalar4x4<T>::scalar4x4(const T* input, const size_t count) noexcept
{
    base::copy_pod_range(input, math::min(XR_COUNTOF(components), count), components);
}

template<typename T>
constexpr scalar4x4<T>::scalar4x4(const T (&arr)[16]) noexcept
    : scalar4x4{ arr[0], arr[1], arr[2],  arr[3],  arr[4],  arr[5],  arr[6],  arr[7],
                 arr[8], arr[9], arr[10], arr[11], arr[12], arr[13], arr[14], arr[15] }
{
}

template<typename T>
scalar4x4<T>::scalar4x4(const scalar3x3<T>& linear_tf) noexcept
{
    for (size_t row = 0; row < 3u; ++row) {
        for (size_t col = 0; col < 3u; ++col) {
            components[index_at(row, col)] = linear_tf(row, col);
        }
    }

    a03 = a13 = a23 = T(0);
    a30 = a31 = a32 = T(0);
    a33 = T(1);
}

template<typename T>
constexpr scalar4x4<T>::scalar4x4(const T a00_, const T a11_, const T a22_, const T a33_) noexcept
    : scalar4x4{ a00_, T(0), T(0), T(0), T(0), a11_, T(0), T(0), T(0), T(0), a22_, T(0), T(0), T(0), T(0), a33_ }
{
}

template<typename T>
template<typename vector_type_tag>
scalar4x4<T>::scalar4x4(const scalar3<T>& x_axis,
                        const scalar3<T>& y_axis,
                        const scalar3<T>& z_axis,
                        vector_type_tag tp) noexcept
{
    set(x_axis, y_axis, z_axis, { T{}, T{}, T{} }, tp);
}

template<typename T>
template<typename vector_type_tag>
scalar4x4<T>::scalar4x4(const scalar3<T>& x_axis,
                        const scalar3<T>& y_axis,
                        const scalar3<T>& z_axis,
                        const scalar3<T>& origin,
                        vector_type_tag tp) noexcept
{
    set(x_axis, y_axis, z_axis, origin, tp);
}

template<typename T>
void
scalar4x4<T>::set(const scalar3<T>& x_axis,
                  const scalar3<T>& y_axis,
                  const scalar3<T>& z_axis,
                  const scalar3<T>& origin,
                  col_tag) noexcept
{
    a00 = x_axis.x;
    a01 = y_axis.x;
    a02 = z_axis.x;
    a03 = origin.x;

    a10 = x_axis.y;
    a11 = y_axis.y;
    a12 = z_axis.y;
    a13 = origin.y;

    a20 = x_axis.z;
    a21 = y_axis.z;
    a22 = z_axis.z;
    a23 = origin.z;

    a30 = a31 = a32 = T{ 0 };
    a33 = T{ 1 };
}

template<typename T>
void
scalar4x4<T>::set(const scalar3<T>& x_axis,
                  const scalar3<T>& y_axis,
                  const scalar3<T>& z_axis,
                  const scalar3<T>& origin,
                  row_tag) noexcept
{
    a00 = x_axis.x;
    a01 = x_axis.y;
    a02 = x_axis.z;
    a03 = origin.x;

    a10 = y_axis.x;
    a11 = y_axis.y;
    a12 = y_axis.z;
    a13 = origin.y;

    a20 = z_axis.x;
    a21 = z_axis.y;
    a22 = z_axis.z;
    a23 = origin.z;

    a30 = a31 = a32 = T{ 0 };
    a33 = T{ 1 };
}

template<typename T>
scalar4x4<T>&
scalar4x4<T>::operator+=(const scalar4x4<T>& rhs) noexcept
{
    for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
        components[i] += rhs.components[i];
    }

    return *this;
}

template<typename T>
scalar4x4<T>&
scalar4x4<T>::operator-=(const scalar4x4<T>& rhs) noexcept
{
    for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
        components[i] -= rhs.components[i];
    }

    return *this;
}

template<typename T>
scalar4x4<T>&
scalar4x4<T>::operator*=(const T k) noexcept
{
    for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
        components[i] *= k;
    }

    return *this;
}

template<typename T>
scalar4x4<T>&
scalar4x4<T>::operator/=(const T k) noexcept
{
    for (size_t i = 0; i < XR_COUNTOF(components); ++i) {
        components[i] /= k;
    }

    return *this;
}

/// @}

} // namespace math
} // namespace xray
