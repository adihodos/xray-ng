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
/// \file    sphere.hpp

#include <type_traits>
#include "xray/math/scalar3.hpp"
#include "xray/xray.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
class sphere
{
  public:
    using point_type = scalar3<real_type>;
    using class_type = sphere<real_type>;

    point_type center;
    real_type radius;

    sphere() noexcept = default;

    ///< \brief Construct from the components of a point and a radius.
    constexpr sphere(const real_type cx, const real_type cy, const real_type cz, const real_type sph_radius) noexcept
        : center{ cx, cy, cz }
        , radius{ sph_radius }
    {
    }

    ///< \brief Construct from a point and a radius.
    constexpr sphere(const point_type& center_pt, const real_type sph_radius) noexcept
        : center{ center_pt }
        , radius{ sph_radius }
    {
    }

    real_type squared_radius() const noexcept { return radius * radius; }

    struct stdc;
};

template<typename real_type>
    requires std::is_arithmetic_v<real_type>
struct sphere<real_type>::stdc
{
    ///<    Sphere centered at the origin, with a radius of 1.
    static constexpr const sphere<real_type> unity{ scalar3<real_type>::stdc::zero, real_type{ 1 } };
    static constexpr const sphere<real_type> null{ scalar3<real_type>::stdc::zero, real_type{ 0 } };
};

/// \name   Helper typedefs
/// @{

using sphere3f = sphere<float>;
using sphere3d = sphere<double>;

/// @}

/// @}

} // namespace xray
} // namespace math
