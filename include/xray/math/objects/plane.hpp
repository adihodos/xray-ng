//
// Copyright (c) 2011-2017 Adrian Hodos
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
#include "xray/xray.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

/**
 * \class   plane
 * \brief   A plane in R3. The plane is stored using a unit length vector
 * 			perpendicular to the plane (the plane's normal) and an constant.
 * 			Thus, given a plane with unit length normal N(a, b, c) and a
 * 			constant D, any point P(X, Y, Z) in the plane verifies the equation
 * 			aX + bY + cZ + D = 0.
 */
template<typename T>
class plane
{
  public:
    plane() noexcept = default;

    /**
     * @brief Construct from normal and offset.
     */
    constexpr plane(const scalar3<T>& n, const T off) noexcept
        : normal{ n }
        , offset(off)
    {
    }

    /**
     * \brief   Construct with an origin point and a specified normal vector.
     * \param   normal  The plane's normal. Must be a vector3 object
     * 					of unit length.
     * \param   origin  The origin point of the plane.
     */
    constexpr plane(const scalar3<T>& n, const scalar3<T>& origin) noexcept
        : normal{ normal }
        , offset{ -dot(n, origin) }
    {
    }

    /**
     * \brief   Makes a plane from three non-collinear points.
     * \param   p0  First point.
     * \param   p1  Second point.
     * \param   p2  Third point.
     */
    static plane<T> from_points(const scalar3<T>& p0, const scalar3<T>& p1, const scalar3<T>& p2) noexcept
    {
        return { normalize(cross(p1 - p0, p2 - p0)), p0 };
    }

    /**
     * \brief     Makes a plane that is parallel with two direction vectors and
     *            has the specified point as its origin. The direction vectors
     *            must not be parallel.
     * \param   d0      The first direction vector.
     * \param   d1      The second direction vector.
     * \param   origin  The origin point.
     */
    static plane<T> from_directions_and_point(const scalar3<T>& d0,
                                              const scalar3<T>& d1,
                                              const scalar3<T>& origin) noexcept
    {
        return { normalize(cross(d0, d1)), origin };
    }

    scalar3<T> normal;
    T offset;
};

using planef32 = plane<float>;
using planef64 = plane<double>;

/// \brief Returns the signed distance from a point to a plane.
/// If the result is positive, the point is on the same side as the plane's
/// normal vector. If negative, the point is on the opposite side. When equal to
/// zero, the point is on the plane itself.
template<typename T>
inline T
signed_distance(const scalar3<T>& p, const plane<T>& plane) noexcept
{
    return dot(plane.normal, p) + plane.offset;
}

/// @}

} // namespace math
} // namespace xray
