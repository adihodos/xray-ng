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

#include <cmath>

#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/xray.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template<typename real_type>
inline bool
operator==(const scalar4<real_type>& lhs, const scalar4<real_type>& rhs) noexcept
{
    return is_equal(lhs.x, rhs.x) && is_equal(lhs.y, rhs.y) && is_equal(lhs.z, rhs.z) && is_equal(lhs.w, rhs.w);
}

template<typename real_type>
inline bool
operator!=(const scalar4<real_type>& lhs, const scalar4<real_type>& rhs) noexcept
{
    return !(lhs == rhs);
}

template<typename real_type>
inline scalar4<real_type>
operator-(const scalar4<real_type>& vec) noexcept
{
    return { -vec.x, -vec.y, -vec.z, -vec.w };
}

template<typename real_type>
inline scalar4<real_type>
operator+(const scalar4<real_type>& lhs, const scalar4<real_type>& rhs) noexcept
{
    return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
}

template<typename real_type>
inline scalar4<real_type>
operator-(const scalar4<real_type>& lhs, const scalar4<real_type>& rhs) noexcept
{
    return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
}

template<typename real_type>
inline scalar4<real_type>
operator*(const real_type k, const scalar4<real_type>& vec) noexcept
{
    scalar4<real_type> res{ vec };
    res *= k;
    return res;
}

template<typename real_type>
inline scalar4<real_type>
operator*(const scalar4<real_type>& vec, const real_type k) noexcept
{
    return k * vec;
}

template<typename real_type>
inline scalar4<real_type>
operator/(const scalar4<real_type>& vec, const real_type k) noexcept
{
    assert(!is_zero(k));
    scalar4<real_type> res{ vec };
    res /= k;
    return res;
}

/// \brief Returns a vector whose components have the maximum values of the
/// components of the two input vectors.
template<typename T>
scalar4<T>
max(const scalar4<T>& a, const scalar4<T>& b) noexcept
{
    return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w) };
}

/// \brief Returns a vector whose components have the minimum values of the
/// components of the two input vectors.
template<typename T>
scalar4<T>
min(const scalar4<T>& a, const scalar4<T>& b) noexcept
{
    return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w) };
}

template<typename T>
scalar4<T>
abs(const scalar4<T>& a) noexcept
{
    return { std::abs(a.x), std::abs(a.y), std::abs(a.z), std::abs(a.w) };
}

/// @}

} // namespace math
} // namespace xray
