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

#include "xray/base/shims/stl_type_traits_shims.hpp"
#include "xray/math/constants.hpp"
#include "xray/xray.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace xray {
namespace math {
namespace detail {

struct floating_point_tag
{};
struct integral_tag
{};

template<typename T>
inline bool
is_zero_impl(const T fp_val, detail::floating_point_tag) noexcept
{
    return std::abs(fp_val) < epsilon<T>;
}

template<typename T>
inline bool
is_zero_impl(const T int_val, detail::integral_tag) noexcept
{
    return int_val == T{};
}

template<typename T>
inline bool
is_equal_impl(const T a, const T b, detail::floating_point_tag) noexcept
{
    return is_zero_impl(a - b, detail::floating_point_tag{});
}

template<typename T>
inline bool
is_equal_impl(const T a, const T b, detail::integral_tag) noexcept
{
    return a == b;
}

} // namespace detail

/// \addtogroup __GroupXrayMath
/// @{

template<typename T>
inline bool
is_zero(const T arith_val) noexcept
{
    static_assert(base::std_is_arithmetic<T>, "Duh!!");

    return detail::is_zero_impl(
        arith_val,
        base::std_conditional<base::std_is_floating_point<T>, detail::floating_point_tag, detail::integral_tag>{});
}

template<typename T>
inline bool
is_equal(const T a, const T b) noexcept
{
    static_assert(base::std_is_arithmetic<T>, "Duh!!");

    return detail::is_equal_impl(
        a,
        b,
        base::std_conditional<base::std_is_floating_point<T>, detail::floating_point_tag, detail::integral_tag>{});
}

/// @}

} // namespace math
} // namespace xray
