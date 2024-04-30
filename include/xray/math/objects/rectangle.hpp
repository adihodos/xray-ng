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

///
/// \file    rectangle.hpp

#include "xray/math/scalar2.hpp"
#include "xray/xray.hpp"
#include <cassert>
#include <cmath>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

template<typename real_type>
class rectangle
{

    /// \name   Defined types.
    /// @{

  public:
    using point_type = scalar2<real_type>;

    /// @}

    /// \name   Members.
    /// @{

  public:
    union
    {

        struct
        {
            real_type left;
            real_type top;
            real_type right;
            real_type bottom;
        };

        struct
        {
            point_type top_left;
            point_type bottom_right;
        };
    };

    /// @}

    /// \name   Constructors
    /// @{

  public:
    rectangle() noexcept = default;

    /// \brief  Construct with 4 values.
    rectangle(const real_type r_left, const real_type r_top, const real_type r_right, const real_type r_bottom) noexcept
        : left{ r_left }
        , top{ r_top }
        , right{ r_right }
        , bottom{ r_bottom }
    {
        assert(left <= right);
        assert(top <= bottom);
    }

    /// \brief  Construct with two points.
    rectangle(const point_type& pt_top_left, const point_type& pt_bottom_right) noexcept
        : rectangle{ pt_top_left.x, pt_top_left.y, pt_bottom_right.x, pt_bottom_right.y }
    {
    }

    /// @}

    /// \name   Attributes.
    /// @{

  public:
    real_type width() const noexcept { return right - left; }

    real_type height() const noexcept { return bottom - top; }

    real_type area() const noexcept { return width() * height(); }

    /// @}
};

/// @}

using rect2f = rectangle<float>;
using rect2d = rectangle<double>;

} // namespace xray
} // namespace math
