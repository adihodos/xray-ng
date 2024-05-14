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

#include "xray/math/handedness.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/xray.hpp"
#include <cassert>
#include <cmath>

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath
/// @{

template<typename T>
scalar4x4<T>
perspective(const T rmin, const T rmax, const T umin, const T umax, const T dmin, const T dmax)
{

    return MatrixWithInvertedMatrixPair<T>{

        .transform =
            scalar4x4<T>{ // 1st row
                          T{ 2 } * dmin / (rmax - rmin),
                          T{},
                          -(rmax + rmin) / (rmax - rmin),
                          T{},
                          // 2nd row
                          T{},
                          T{ 2 } * dmin / (umax - umin),
                          -(umax + umin) / (umax - umin),
                          T{},
                          // 3rd row
                          T{},
                          T{},
                          dmax / (dmax - dmin),
                          (-dmax * dmin) / (dmax - dmin),
                          // 4th row
                          T{},
                          T{},
                          T{ 1 },
                          T{ 0 } },

        .inverted =
            scalar4x4<T>{ // 1st row
                          (rmax - rmin) / T{ 2 } * dmin,
                          T{},
                          T{},
                          (rmax + rmin) / (T{ 2 } * dmin),
                          // 2nd row
                          T{},
                          (umax - umin) / (T{ 2 } * dmin),
                          T{},
                          (umax + umin) / (T{ 2 } * dmin),
                          // 3rd row
                          T{},
                          T{},
                          T{},
                          T{ 1 },
                          // 4th row
                          T{ 0 },
                          T{ 0 },
                          (dmin - dmax) / (dmax * dmin),
                          T{ 1 } / dmin }
    };
}

template<typename T>
MatrixWithInvertedMatrixPair<T>
perspective_symmetric(const T aspect_ratio, const T fov, const T near_plane, const T far_plane) noexcept
{
    const auto d = T(1.0) / std::tan(fov * T(0.5));
    const auto dmin{ near_plane };
    const auto dmax{ far_plane };

    return MatrixWithInvertedMatrixPair<T>{

        .transform =
            scalar4x4<T>{ // 1st row
                          d / aspect_ratio,
                          T{},
                          T{},
                          T{},
                          // 2nd row
                          T{},
                          d,
                          T{},
                          T{},
                          // 3rd row
                          T{},
                          T{},
                          dmax / (dmax - dmin),
                          (-dmax * dmin) / (dmax - dmin),
                          // 4th row
                          T{},
                          T{},
                          T{ 1 },
                          T{ 0 } },

        .inverted =
            scalar4x4<T>{ // 1st row
                          aspect_ratio / d,
                          T{},
                          T{},
                          T{},
                          // 2nd row
                          T{},
                          T{ 1 } / d,
                          T{},
                          T{},
                          // 3rd row
                          T{},
                          T{},
                          T{},
                          T{ 1 },
                          // 4th row
                          T{},
                          T{},
                          (dmin - dmax) / (dmax * dmin),
                          T{ 1 } / dmin }
    };
}

template<typename T>
inline MatrixWithInvertedMatrixPair<T>
perspective_symmetric(const T width, const T height, const T fov, const T near_plane, const T far_plane) noexcept
{
    return perspective_symmetric(width / height, fov, near_plane, far_plane);
}

template<typename T>
MatrixWithInvertedMatrixPair<T>
view_matrix(const scalar3<T> right, const scalar3<T> up, const scalar3<T> dir, const scalar3<T> origin) noexcept
{

    assert(are_orthogonal(right, up) && "Right and up vector must be orthogonal !");
    assert(are_orthogonal(right, dir) && "Right and direction vector must be orthogonal !");
    assert(are_orthogonal(dir, up) && "Direction and up vector must be orthogonal !");

    return MatrixWithInvertedMatrixPair<T> {

		.transform = { // 1st row
             right.x,
             right.y,
             right.z,
             -dot(right, origin),
             // 2nd row
             up.x,
             up.y,
             up.z,
             -dot(up, origin),
             // 3rd row
             dir.x,
             dir.y,
             dir.z,
             -dot(dir, origin),
             // 4th row
             T{},
             T{},
             T{},
             T{ 1 }
		},
		
		.inverted = {
			// 1st row
			right.x, up.x, dir.x, origin.x,
			// 2nd row
			right.y, up.y, dir.y, origin.y,
			// 3rd row
			right.z, up.z, dir.z, origin.z,
			// 4th row
			T{}, T{}, T{}, T{1}
		}
	};
}

template<typename T>
MatrixWithInvertedMatrixPair<T>
look_at(const scalar3<T>& eye_pos, const scalar3<T>& target, const scalar3<T>& world_up) noexcept
{
    const auto direction = normalize(target - eye_pos);

    assert(!is_zero_length(direction) && "Direction vector cannot be null!");
    assert(!are_parallel(direction, world_up) && "Direction vector and world up vector cannot be parallel!");

    const auto right = normalize(cross(world_up, direction));
    const auto up = cross(direction, right);

    return view_matrix(right, up, direction, eye_pos);
}

template<typename T>
scalar4x4<T>
orthographic(const T left, const T right, const T top, const T bottom, const T dmin, const T dmax) noexcept
{
    // TODO: convert to MatrixWithInvertedMatrixPair
    // clang-format off
	return scalar4x4<T> {
		T{2} / (right - left), T{}, T{}, -(right + left) / (right - left),
		T{}, T{2} / (top - bottom), T{}, -(top + bottom) / (top - bottom),
		T{}, T{}, T{1} / (dmax - dmin), -dmin / (dmax - dmin),
		T{}, T{}, T{}, T{1}
	};
    // clang-format on
}

template<typename T>
scalar4x4<T>
orthographic_symmetric(const T width, const T height, const T dmin, const T dmax) noexcept
{
    // TODO: convert to MatrixWithInvertedMatrixPair
    // clang-format off
	return scalar4x4<T> {
		T{1} / (width * T{0.5f}), T{}, T{}, T{},
		T{}, T{1} / (height * 0.5f), T{}, T{},
		T{}, T{}, T{1} / (dmax - dmin), -dmin / (dmax - dmin),
		T{}, T{}, T{}, T{1}
	};
    // clang-format on
}

/// @}

} // namespace math
} // namespace xray
