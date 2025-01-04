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

/// \file   scalar4.hpp

#include <cstdint>
#include <type_traits>

#include "xray/math/scalar3.hpp"
#include "xray/math/swizzle.hpp"
#include "xray/base/algorithms/copy_pod_range.hpp"
#include "xray/math/math_std.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

/// \class   scalar4
/// \brief   A four component tuple in the form (x, y, z, w),
///             that is used to represent an affine vector/affine point/
///             homogeneous point. It is up to the user of the class to make the
///             distinction
///             in code. An affine vector has the w component set to 0. An
///             affine point has the w component set to 1. And finally, a
///             homogeneous
///             point has a w component with a value different than 1.
template<typename T>
class scalar4 : public SwizzleBase<T, 4>
{
    static_assert(std::is_arithmetic_v<T>, "Template parameter needs to be an arythmetic type!");

    /// \name Defined types.
    /// @{

  public:
    using class_type = scalar4<T>;

    /// @}

    /// \name Data members.
    /// @{

  public:
    union
    {

        struct
        {
            T x;
            T y;
            T z;
            T w;
        };

        struct
        {
            T r;
            T g;
            T b;
            T a;
        };

        struct
        {
            T s;
            T t;
            T p;
            T q;
        };

        T components[4]; ///< Array like access to the vector's components.
    };

    /// @}

    /// \name Constructors
    /// @{

  public:
    /// Default constructor. Leaves components uninitialized.
    scalar4() noexcept = default;

    explicit constexpr scalar4(const T val) noexcept
        : scalar4<T>{ val, val, val, val }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    explicit constexpr scalar4(const U val) noexcept
    {
        x = y = z = w = static_cast<T>(val);
    }

    constexpr scalar4(const T x_val, const T y_val, const T z_val, const T w_val = T(1)) noexcept
        : x{ x_val }
        , y{ y_val }
        , z{ z_val }
        , w{ w_val }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    constexpr scalar4(const U x_val, const U y_val, const U z_val, const U w_val = U(1)) noexcept
        : x{ static_cast<T>(x_val) }
        , y{ static_cast<T>(y_val) }
        , z{ static_cast<T>(z_val) }
        , w{ static_cast<T>(w_val) }
    {
    }

    explicit constexpr scalar4(const std::array<T, 4>& arr) noexcept
    {
        memcpy(this->components, arr.data(), sizeof(this->components));
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    explicit scalar4(const std::array<U, 4>& arr) noexcept
    {
        x = static_cast<T>(arr[0]);
        y = static_cast<T>(arr[1]);
        z = static_cast<T>(arr[2]);
        w = static_cast<T>(arr[3]);
    }

    explicit constexpr scalar4(const T (&arr)[4]) noexcept
    {
        x = arr[0];
        y = arr[1];
        z = arr[2];
        w = arr[4];
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    explicit scalar4(const U (&arr)[4]) noexcept
    {
        x = static_cast<T>(arr[0]);
        y = static_cast<T>(arr[1]);
        z = static_cast<T>(arr[2]);
        z = static_cast<T>(arr[4]);
    }

    constexpr scalar4(const Swizzle4<T> s) noexcept
        : scalar4{ s.x, s.y, s.z, s.w }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    constexpr scalar4(const Swizzle4<U> s) noexcept
        : scalar4{ s.x, s.y, s.z, s.w }
    {
    }

    constexpr scalar4(const scalar3<T>& val, const T w_val = T(1)) noexcept
        : scalar4{ val.x, val.y, val.z, w_val }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    constexpr scalar4(const scalar3<U>& val, const U w_val = T(1)) noexcept
        : scalar4{ val.x, val.y, val.z, w_val }
    {
    }

    explicit scalar4(const T* inputs) noexcept
        : scalar4{ inputs[0], inputs[1], inputs[2], inputs[3] }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    explicit scalar4(const U* s) noexcept
        : scalar4{ s[0], s[1], s[2], s[3] }
    {
    }

    /// @}

    /// \name Assign operators.
    /// @{

  public:
    /// \brief   Addition assignment operator.
    /// \remarks This operation applies to affine vectors. It can be applied
    ///             to affine points, if the following holds :
    ///             Let P, P0, .. Pn be affine points in R4. Then
    ///             P can be written as P = P0 * a0 + P1 * a1 + ... + Pn * an,
    ///             only if (a0 + a1 + ... + an) = 1. So if you know that the
    ///             right
    ///             hand operand is one of the members in the set
    ///             { Pi * ai | 0 <= i <= n and sum(i = 0..n) ai = 1 }
    ///             then it can be applied to scalar4 objects representing affine
    ///             points.
    class_type& operator+=(const scalar4<T>& rhs) noexcept
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    /// \brief   Subtraction assignment operator.
    /// \remarks This operation is valid for vectors and affine points.
    ///             Substracting two affine points result in an affine vector.
    class_type& operator-=(const scalar4<T>& rhs) noexcept
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    class_type& operator*=(const T k) noexcept
    {
        x *= k;
        y *= k;
        z *= k;
        w *= k;
        return *this;
    }

    class_type& operator/=(const T k) noexcept
    {
        x /= k;
        y /= k;
        z /= k;
        w /= k;
        return *this;
    }

    template<typename... Ts>
    constexpr auto swizzle() const noexcept
    {
        return SwizzleBase<T, 4>::swizzle(this->components, Ts{}...);
    }

    /// \name Standard constants for R4 vectors.
    /// @{
  public:
    struct stdc;

    /// @}
};

template<typename T>
struct scalar4<T>::stdc
{
    static constexpr const scalar4<T> unit_x{ T(1.0), T(0.0), T(0.0), T(0.0) };
    static constexpr const scalar4<T> unit_y{ T(0.0), T(1.0), T(0.0), T(0.0) };
    static constexpr const scalar4<T> unit_z{ T(0.0), T(0.0), T(1.0), T(0.0) };
    static constexpr const scalar4<T> unit_w{ T(0.0), T(0.0), T(0.0), T(1.0) };
    static constexpr const scalar4<T> zero{ T(0.0), T(0.0), T(0.0), T(0.0) };
    static constexpr const scalar4<T> one{ T(1.0), T(1.0), T(1.0), T(1.0) };
};

template<typename T>
constexpr const scalar4<T> scalar4<T>::stdc::unit_x;

template<typename T>
constexpr const scalar4<T> scalar4<T>::stdc::unit_y;

template<typename T>
constexpr const scalar4<T> scalar4<T>::stdc::unit_z;

template<typename T>
constexpr const scalar4<T> scalar4<T>::stdc::unit_w;

template<typename T>
constexpr const scalar4<T> scalar4<T>::stdc::zero;

template<typename T>
constexpr const scalar4<T> scalar4<T>::stdc::one;

using vec4f = scalar4<float>;
using vec4d = scalar4<double>;
using vec4ui8 = scalar4<uint8_t>;
using vec4i8 = scalar4<int8_t>;

/// @}

} // namespace math
} // namespace xray
