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
#include "xray/math/scalar3_math.hpp"
#include "xray/math/space_traits.hpp"
#include "xray/xray.hpp"
#include <cstdint>
#include <type_traits>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template<typename T>
class oriented_bounding_box
{
  public:
    using point_type = scalar3<T>;
    using vector_type = scalar3<T>;

    ///<  Origin (centre) point of the OOBB.
    point_type origin;

    union
    {
        struct
        {
            point_type right;
            point_type up;
            point_type dir;
        };

        point_type components[3];
    } axis; ///< A set of orthonormal vectors representing the axis of the oobb.

    union
    {
        struct
        {
            T right;
            T up;
            T dir;
        };
        T components[3];
    } extents; ///< Positive and negative extents along each axis.

    oriented_bounding_box() noexcept = default;

    oriented_bounding_box(const point_type& org,
                          const vector_type& right,
                          const vector_type& up,
                          const vector_type& dir,
                          const T extent_right,
                          const T extent_up,
                          const T extent_dir) noexcept
        : origin{ org }
    {
        axis.right = right;
        axis.up = up;
        axis.dir = dir;
        extents.right = extent_right;
        extents.up = extent_up;
        extents.dir = extent_dir;
    }

    const point_type min() const noexcept
    {
        return origin - axis.right * extents.right - axis.up * extents.up - axis.dir * extents.dir;
    }

    const point_type max() const noexcept
    {
        return origin + axis.right * extents.right + axis.up * extents.up + axis.dir * extents.dir;
    }
};

/// @}

} // namespace math
} // namespace xray
