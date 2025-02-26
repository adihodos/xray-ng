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

#include <initializer_list>
#include <limits>
#include <ranges>

#include "xray/math/math_std.hpp"
#include "xray/math/rank.hpp"
#include "xray/math/concept.point.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

template<typename PointType>
    requires Point<PointType>
struct BoundingBoxAxisAligned
{
    PointType min;
    PointType max;

    using class_type = BoundingBoxAxisAligned<PointType>;
    using value_type = typename PointType::value_type;
    static constexpr const size_t R = Rank<PointType>::R;

    BoundingBoxAxisAligned() noexcept = default;

    constexpr BoundingBoxAxisAligned(PointType pmin, PointType pmax) noexcept
        : min{ pmin }
        , max{ pmax }
    {
    }

    template<std::ranges::forward_range PointsRange>
        requires std::is_same_v<std::ranges::range_value_t<PointsRange>, PointType>
    constexpr BoundingBoxAxisAligned(PointsRange&& pts) noexcept;

    // constexpr BoundingBoxAxisAligned(std::initializer_list<PointType> points_coll) noexcept;

    PointType center() const noexcept { return (max + min) / value_type{ 2 }; }
    value_type width() const noexcept { return std::abs(max.x - min.x); }
    value_type height() const noexcept { return std::abs(max.y - min.y); }
    value_type depth() const noexcept
        requires requires { Rank<PointType>::R == 3; }
    {
        return std::abs(max.z - min.z);
    }

    value_type max_dimension() const noexcept
    {
        if constexpr (class_type::R == 3) {
            return math::max(width(), math::max(height(), depth()));
        } else {
            return math::max(width(), height());
        }
    }

    PointType extents() const noexcept
    {
        if constexpr (class_type::R == 3) {
            return PointType{
                width() / value_type{ 2 },
                height() / value_type{ 2 },
                depth() / value_type{ 2 },
            };
        } else {
            return PointType{
                width() / value_type{ 2 },
                height() / value_type{ 2 },
            };
        }
    }

    constexpr BoundingBoxAxisAligned<PointType>& operator|=(PointType p) noexcept
    {
        this->min = math::min(this->min, p);
        this->max = math::max(this->max, p);
        return *this;
    }

    constexpr BoundingBoxAxisAligned<PointType>& operator|=(const BoundingBoxAxisAligned<PointType>& rhs) noexcept
    {
        this->min = math::min(this->min, rhs.min);
        this->max = math::max(this->max, rhs.max);
        return *this;
    }

    constexpr bool operator==(const BoundingBoxAxisAligned<PointType>& rhs) const noexcept
    {
        return min == rhs.min && max == rhs.max;
    }

    constexpr bool operator!=(const BoundingBoxAxisAligned<PointType>& rhs) const noexcept { return !(*this == rhs); }

    struct stdc;
};

template<typename PointType>
    requires Point<PointType>
struct BoundingBoxAxisAligned<PointType>::stdc
{
    using value_type = typename PointType::value_type;
    static constexpr const BoundingBoxAxisAligned<PointType> identity{
        PointType{ std::numeric_limits<value_type>::max() },
        PointType{ std::numeric_limits<value_type>::min() }
    };
};

template<typename PointType>
    requires Point<PointType>
template<std::ranges::forward_range PointsRange>
    requires std::is_same_v<std::ranges::range_value_t<PointsRange>, PointType>
constexpr BoundingBoxAxisAligned<PointType>::BoundingBoxAxisAligned(PointsRange&& points_range) noexcept
{
    using BBoxType = BoundingBoxAxisAligned<PointType>;

    this->min = PointType{ std::numeric_limits<value_type>::max() };
    this->max = PointType{ std::numeric_limits<value_type>::min() };

    for (auto&& point : points_range) {
        this->min = math::min(this->min, point);
        this->max = math::max(this->max, point);
    }
}

// template<typename PointType>
//     requires Point<PointType>
// constexpr BoundingBoxAxisAligned<PointType>::BoundingBoxAxisAligned(
//     std::initializer_list<PointType> points_coll) noexcept
// {
//     using BBoxType = BoundingBoxAxisAligned<PointType>;
//
//     this->min = PointType{ std::numeric_limits<value_type>::max() };
//     this->max = PointType{ std::numeric_limits<value_type>::min() };
//
//     for (auto&& point : points_coll) {
//         this->min = math::min(this->min, point);
//         this->max = math::max(this->max, point);
//     }
// }

/// @}

} // namespace math
} // namespace xray
