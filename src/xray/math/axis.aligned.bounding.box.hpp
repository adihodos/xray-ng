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

#include <limits>
#include <tl/optional.hpp>

#include "xray/xray.hpp"
#include "xray/base/xray.slice.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/rank.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

struct OriginWithExtentsTag {};

template <typename PointType>
struct BoundingBoxAxisAligned {
	PointType min;
	PointType max;

	using class_type				= BoundingBoxAxisAligned<PointType>;
	using value_type				= typename PointType::value_type;
	static constexpr const size_t R = Rank<PointType>::R;

	BoundingBoxAxisAligned() noexcept = default;

	constexpr BoundingBoxAxisAligned(PointType pmin, PointType pmax) noexcept : min{pmin}, max{pmax} {}

	constexpr BoundingBoxAxisAligned(OriginWithExtentsTag, PointType origin, PointType half_extents) noexcept
		: BoundingBoxAxisAligned{origin - half_extents, origin + half_extents} {}

	constexpr BoundingBoxAxisAligned(const base::xrSlice_t<const PointType> pts) noexcept;

	PointType center() const noexcept { return (max + min) / value_type{2}; }
	value_type width() const noexcept { return std::abs(max.x - min.x); }
	value_type height() const noexcept { return std::abs(max.y - min.y); }
	value_type depth() const noexcept
		requires requires { Rank<PointType>::R == 3; }
	{
		return std::abs(max.z - min.z);
	}

	value_type max_dimension() const noexcept {
		if constexpr (class_type::R == 3) {
			return math::max(width(), math::max(height(), depth()));
		} else {
			return math::max(width(), height());
		}
	}

	PointType extents() const noexcept {
		if constexpr (class_type::R == 3) {
			return PointType{
				width() / value_type{2},
				height() / value_type{2},
				depth() / value_type{2},
			};
		} else {
			return PointType{
				width() / value_type{2},
				height() / value_type{2},
			};
		}
	}

	constexpr BoundingBoxAxisAligned<PointType>& operator|=(PointType p) noexcept {
		this->min = math::min(this->min, p);
		this->max = math::max(this->max, p);
		return *this;
	}

	constexpr BoundingBoxAxisAligned<PointType>& operator|=(const BoundingBoxAxisAligned<PointType>& rhs) noexcept {
		this->min = math::min(this->min, rhs.min);
		this->max = math::max(this->max, rhs.max);
		return *this;
	}

	constexpr bool operator==(const BoundingBoxAxisAligned<PointType>& rhs) const noexcept {
		return min == rhs.min && max == rhs.max;
	}

	constexpr bool operator!=(const BoundingBoxAxisAligned<PointType>& rhs) const noexcept { return !(*this == rhs); }

	constexpr bool contains_point(const PointType p) const noexcept {
		if (p.x < min.x || p.x > max.x) return false;

		if (p.y < min.y || p.y > max.y) return false;

		if constexpr (class_type::R == 3) {
			if (p.z < min.z || p.z > max.z) return false;
		}

		return true;
	}

	constexpr bool contains_box(const BoundingBoxAxisAligned<PointType>& rhs) const noexcept {
		if constexpr (class_type::R == 2) {
			return rhs.min.x >= min.x && rhs.min.x <= max.x && rhs.max.x >= min.x && rhs.max.x <= max.x &&
				   rhs.min.y >= min.y && rhs.min.y <= max.y && rhs.max.y >= min.y && rhs.max.y <= max.y;
		} else {
			return rhs.min.x >= min.x && rhs.min.x <= max.x && rhs.max.x >= min.x && rhs.max.x <= max.x &&
				   rhs.min.y >= min.y && rhs.min.y <= max.y && rhs.max.y >= min.y && rhs.max.y <= max.y &&
				   rhs.min.z >= min.z && rhs.min.z <= max.z && rhs.max.z >= min.z && rhs.max.z <= max.z;
		}
	}

	struct stdc;
};

template <typename PointType>
struct BoundingBoxAxisAligned<PointType>::stdc {
	using value_type = typename PointType::value_type;
	static constexpr const BoundingBoxAxisAligned<PointType> identity{
		PointType{std::numeric_limits<value_type>::max()}, PointType{std::numeric_limits<value_type>::min()}
	};
};

template <typename PointType>
constexpr BoundingBoxAxisAligned<PointType>::BoundingBoxAxisAligned(
	const base::xrSlice_t<const PointType> points_range
) noexcept {
	this->min = PointType{std::numeric_limits<value_type>::max()};
	this->max = PointType{std::numeric_limits<value_type>::min()};

	for (const auto& point : points_range) {
		this->min = math::min(this->min, point);
		this->max = math::max(this->max, point);
	}
}

template <typename PointType>
tl::optional<BoundingBoxAxisAligned<PointType>> operator^(
	const BoundingBoxAxisAligned<PointType>& lhs, const BoundingBoxAxisAligned<PointType>& rhs
) noexcept {
	using value_type = typename BoundingBoxAxisAligned<PointType>::value_type;

	const value_type min_x = math::max(lhs.min.x, rhs.min.x);
	const value_type max_x = math::min(lhs.max.x, rhs.max.x);

	if (min_x >= max_x) {
		return tl::nullopt;
	}

	const value_type min_y = math::max(lhs.min.y, rhs.min.y);
	const value_type max_y = math::min(lhs.max.y, rhs.max.y);

	if (min_y >= max_y) {
		return tl::nullopt;
	}

	return BoundingBoxAxisAligned<PointType>{PointType{min_x, min_y}, PointType{max_x, max_y}};
}

/// @}

}  // namespace math
}  // namespace xray
