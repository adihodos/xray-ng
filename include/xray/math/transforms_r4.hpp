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

#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/xray.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

struct R4
{

    /// \brief      Constructs an affine matrix for a translation in R3.
    template<typename T>
    static scalar4x4<T> translate(const T x, const T y, const T z) noexcept;

    /// \brief      Constructs an affine matrix for a translation in R3.
    template<typename T>
    static scalar4x4<T> translate(const scalar3<T>& vec) noexcept;
};

template<typename T>
scalar4x4<T>
R4::translate(const T x, const T y, const T z) noexcept
{
    // clang-format off

  return {
    T(1), T{},  T{},  x,
    T{},  T(1), T{},  y,
    T{},  T{},  T(1), z,
    T{},  T{},  T{},  T(1)
  };

  // clang-format off
}

/// \brief      Constructs an affine matrix for a translation in R3.
template <typename T>
scalar4x4<T> R4::translate(const scalar3<T>& vec) noexcept {
    return translate(vec.x, vec.y, vec.z);
}

/// @}

} // namespace math
} // namespace xray
