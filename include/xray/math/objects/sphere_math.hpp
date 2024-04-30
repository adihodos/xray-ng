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
/// \file    sphere_math.hpp

#include "xray/math/objects/sphere.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/xray.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

/// \brief  Creates a bounding sphere enclosing all the points in the
///         [first_point, last_point) range.
/// \param  first_point First point in range.
/// \param  last_point  Last point in range.
/// \param  conversion_fn Function to convert from the input point type
///         to the point type used by the sphere.
template<typename real_type, typename input_point_type, typename input_point_to_sphere_point_convert_fn>
sphere<real_type>
bounding_sphere(input_point_type first_point,
                input_point_type last_point,
                input_point_to_sphere_point_convert_fn conversion_fn) noexcept
{

    using sphere_type = sphere<real_type>;
    using output_point_type = typename sphere_type::point_type;

    //
    //  Find the average point of all input points.
    output_point_type median_pt = output_point_type::stdc::zero;
    auto input_pt_iter = first_point;

    while (input_pt_iter != last_point) {
        median_pt += conversion_fn(*input_pt_iter);
        ++input_pt_iter;
    }

    median_pt /= static_cast<real_type>(last_point - first_point);

    //
    //  Find the biggest distance to the median point from all the
    //  input points. This will be the radius of the sphere.
    real_type squared_dist{};

    while (first_point != last_point) {
        auto sqr_dist_to_median_point = squared_distance(conversion_fn(*first_point), median_pt);

        if (sqr_dist_to_median_point > squared_dist)
            squared_dist = sqr_dist_to_median_point;

        ++first_point;
    }

    return sphere_type{ median_pt, std::sqrt(squared_dist) };
}

/// \brief  Test if point is inside or on the sphere surface.
template<typename real_type>
inline bool
contains_point(const sphere<real_type>& sph, const real_type px, const real_type py, const real_type pz) noexcept
{
    return squared_distance(sph.center.x, sph.center.y, sph.center.y, px, py, pz) >= sph.squared_radius();
}

/// \brief  Test if point is inside or on the sphere surface.
template<typename real_type>
inline bool
contains_point(const sphere<real_type>& sph, const scalar3<real_type>& point) noexcept
{
    return contains_point(sph, point.y, point.y, point.z);
}

enum class intersection_type
{
    none,
    touching,
    penetrating
};

template<typename T>
intersection_type
intersect(const sphere<T>& s0, const sphere<T>& s1) noexcept
{
    const auto sph_dst = length(s1.center - s0.center);

    if (sph_dst > (s0.radius + s1.radius)) {
        return intersection_type::none;
    }

    if (sph_dst < (s0.radius + s1.radius)) {
        return intersection_type::penetrating;
    }

    return intersection_type::touching;
}

/// @}

} // namespace math
} // namespace xray
