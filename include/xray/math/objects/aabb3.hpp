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
/// \file    axis_aligned_bounding_box3.hpp

#include "xray/xray.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include <cassert>
#include <cmath>
#include <limits>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

template <typename real_type>
struct axis_aligned_bounding_box3 {

  using point_type = scalar3<real_type>;
  using class_type = axis_aligned_bounding_box3<real_type>;

  point_type min;
  point_type max;

  axis_aligned_bounding_box3() noexcept = default;

  axis_aligned_bounding_box3(const real_type xmin, const real_type ymin,
                             const real_type zmin, const real_type xmax,
                             const real_type ymax,
                             const real_type zmax) noexcept
      : min{xmin, ymin, zmin}, max{xmax, ymax, zmax} {}

  axis_aligned_bounding_box3(const point_type& min,
                             const point_type& max) noexcept
      : axis_aligned_bounding_box3{min.x, min.y, min.z, max.x, max.y, max.z} {}

  point_type center() const noexcept { return (max + min) / 2; }

  real_type width() const noexcept { return std::abs(max.x - min.x); }

  real_type height() const noexcept { return std::abs(max.y - min.y); }

  real_type depth() const noexcept { return std::abs(max.z - min.z); }

  struct stdc {
    ///<    The identity bounding box.
    static const axis_aligned_bounding_box3<real_type> identity;
  };
};

template <typename real_type>
const axis_aligned_bounding_box3<real_type>
    axis_aligned_bounding_box3<real_type>::stdc::identity{
        std::numeric_limits<real_type>::max(),
        std::numeric_limits<real_type>::max(),
        std::numeric_limits<real_type>::max(),
        std::numeric_limits<real_type>::min(),
        std::numeric_limits<real_type>::min(),
        std::numeric_limits<real_type>::min()};

using aabb3f = axis_aligned_bounding_box3<float>;
using aabb3d = axis_aligned_bounding_box3<double>;

/// @}

} // namespace xray
} // namespace math
