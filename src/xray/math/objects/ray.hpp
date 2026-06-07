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

#include "xray/math/space_traits.hpp"
#include "xray/xray.hpp"
#include <cstdint>
#include <type_traits>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

/// \brief  A ray in 2D/3D space. The equation for a point on the ray is
///         P = O + t * D, where t is in the [0, inf) range.
template<typename T, size_t space_dimension>
class ray
    : public std::enable_if<(space_dimension == 2) || (space_dimension == 3), space_traits<space_dimension, T>>::type
{

  public:
    using space_traits_type = space_traits<space_dimension, T>;

    using point_type = typename space_traits_type::point_type;
    using vector_type = typename space_traits_type::vector_type;

    point_type origin;    ///< Origin point of the ray
    point_type direction; ///< Unit length direction vector.

    ray() noexcept = default;

    /// \brief Construct from and origin point and a unit length direction vector.
    constexpr ray(const point_type& org, const vector_type& dir) noexcept
        : origin{ org }
        , direction{ dir }
    {
    }

    /// \brief  Return a point on the ray, given a value in the [0, inf) range.
    constexpr point_type eval(const T t) const noexcept { return origin + t * direction; }

    struct construct;
};

template<typename T, size_t dim>
struct ray<T, dim>::construct
{
    /// \brief  Construct a ray originating from p0 and passing through p1.
    static ray<T, dim> from_points(const typename ray<T, dim>::point_type& p0,
                                   const typename ray<T, dim>::point_type& p1) noexcept
    {
        return { p0, normalize(p1 - p0) };
    }
};

/// @}

} // namespace math
} // namespace xray
