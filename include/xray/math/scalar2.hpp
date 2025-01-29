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

#include <cstdint>
#include <type_traits>

#include "xray/math/swizzle.hpp"
#include "xray/xray_types.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

/// \class scalar2
/// \brief  Two component vector/point in R2.
template<typename T>
    requires std::is_arithmetic_v<T>
struct scalar2 : public SwizzleBase<T, 2>
{
    union
    {
        struct
        {
            T x;
            T y;
        };

        struct
        {
            T u;
            T v;
        };

        struct
        {
            T s;
            T t;
        };

        T components[2];
    };

    using class_type = scalar2<T>;

    scalar2() noexcept = default;

    constexpr scalar2(const T xval, const T yval) noexcept
        : x{ xval }
        , y{ yval }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    constexpr scalar2(const U xval, const U yval) noexcept
        : x{ static_cast<T>(xval) }
        , y{ static_cast<T>(yval) }
    {
    }

    constexpr scalar2(const Swizzle2<T> s) noexcept
        : x{ s.x }
        , y{ s.y }
    {
    }

    template<typename U>
    constexpr scalar2(const Swizzle2<U> s) noexcept
        : scalar2{ s.x, s.y }
    {
    }

    explicit constexpr scalar2(const T val) noexcept
        : scalar2<T>{ val, val }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    explicit constexpr scalar2(const U val) noexcept
        : scalar2{ val, val }
    {
    }

    explicit constexpr scalar2(const T (&arr)[2]) noexcept
        : scalar2{ arr[0], arr[1] }
    {
    }

    template<typename U>
        requires std::is_convertible_v<U, T>
    explicit constexpr scalar2(const U (&arr)[2]) noexcept
        : scalar2{ arr[0], arr[1] }
    {
    }

    /// \name Member operators
    /// @{

    inline class_type& operator+=(const class_type& rhs) noexcept;
    inline class_type& operator-=(const class_type& rhs) noexcept;
    inline class_type& operator*=(const T scalar) noexcept;
    inline class_type& operator/=(const T scalar) noexcept;

    /// @}

    template<typename... Ts>
    constexpr auto swizzle() const noexcept
    {
        return SwizzleBase<T, 2>::swizzle(this->components, Ts{}...);
    }

    /// \brief      Standard constants for R2 vectors.
    struct stdc;
};

template<typename T>
    requires std::is_arithmetic_v<T>
struct scalar2<T>::stdc
{
    static constexpr const scalar2<T> unit_x{ T(1.0), T(0.0) };
    static constexpr const scalar2<T> unit_y{ T(0.0), T(1.0) };
    static constexpr const scalar2<T> zero{ T(0.0), T(0.0) };
    static constexpr const scalar2<T> one{ T(1), T(1) };
};

template<typename T>
    requires std::is_arithmetic_v<T>
constexpr const scalar2<T> scalar2<T>::stdc::unit_x;

template<typename T>
    requires std::is_arithmetic_v<T>
constexpr const scalar2<T> scalar2<T>::stdc::unit_y;

template<typename T>
    requires std::is_arithmetic_v<T>
constexpr const scalar2<T> scalar2<T>::stdc::zero;

template<typename T>
    requires std::is_arithmetic_v<T>
constexpr const scalar2<T> scalar2<T>::stdc::one;

using vec2f = scalar2<scalar_lowp>;
using vec2d = scalar2<scalar_mediump>;
using vec2i32 = scalar2<int32_t>;
using vec2ui32 = scalar2<uint32_t>;
using vec2ui64 = scalar2<uint64_t>;

/// @}

} // namespace math
} // namespace xray
