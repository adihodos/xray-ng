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
/// \file    aabb3_math.hpp

#include "xray/xray.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"

namespace xray {
namespace math {

/// \addtogroup __GroupXrayMath_Geometry
/// @{

/// \brief  Creates an axis aligned bounding box enclosing all the vertices
///         in the [points, points + num_points) range.
/// \param  points Pointer to an array of vertices.
/// \param  num_points Number of vertices.
/// \param  stride Size of a vertex.
template <typename real_type>
axis_aligned_bounding_box3<real_type>
bounding_box3_axis_aligned(const scalar3<real_type>* points,
                           const size_t              num_points,
                           const size_t              stride) noexcept {

  using aabb_type = axis_aligned_bounding_box3<real_type>;
  aabb_type output_aabb{aabb_type::stdc::identity};

  for (size_t idx = 0; idx < num_points; ++idx) {
    auto input_vertex = reinterpret_cast<const scalar3<real_type>*>(
        reinterpret_cast<const uint8_t*>(points) + idx * stride);

    output_aabb.min = math::min(output_aabb.min, *input_vertex);
    output_aabb.max = math::max(output_aabb.max, *input_vertex);
  }

  return output_aabb;
}

/// \brief  Merges two axis aligned bounding boxes into a single box that
///         encloses both input aabbs.
template <typename real_type>
axis_aligned_bounding_box3<real_type>
merge(const axis_aligned_bounding_box3<real_type>& lhs,
      const axis_aligned_bounding_box3<real_type>& rhs) noexcept {
  return {math::min(lhs.min.x, rhs.min.x), math::min(lhs.min.y, rhs.min.y),
          math::min(lhs.min.z, rhs.min.z), math::max(lhs.max.x, rhs.max.x),
          math::max(lhs.max.y, rhs.max.y), math::max(lhs.max.z, rhs.max.z)};
}

/// \brief  Returns a new bounding box obtained by transforming the
///         input bounding box with the specified matrix.
template <typename real_type>
axis_aligned_bounding_box3<real_type>
transform(const scalar4x4<real_type>&                  mtx,
          const axis_aligned_bounding_box3<real_type>& aabb) noexcept {
  const auto xmin = mtx[1] * aabb.min.x;
  const auto xmax = mtx[1] * aabb.max.x;

  const auto ymin = mtx[2] * aabb.min.y;
  const auto ymax = mtx[2] * aabb.max.y;

  const auto zmin = mtx[3] * aabb.min.z;
  const auto zmax = mtx[3] * aabb.max.z;

  const auto min_pt = math::min(xmin, xmax) + math::min(ymin, ymax) +
                      math::min(zmin, zmax) + mtx[4];

  const auto max_pt = math::max(xmin, xmax) + math::max(ymin, ymax) +
                      math::max(zmin, zmax) + mtx[4];

  return {min_pt, max_pt};
}

/// \brief  Test if point is inside the aabb.
template <typename real_type>
inline bool contains_point(const axis_aligned_bounding_box3<real_type>& aabb,
                           const real_type px, const real_type py,
                           const real_type pz) noexcept {
  if (px < aabb.min.x || px > aabb.max.x)
    return false;

  if (py < aabb.min.y || py > aabb.max.y)
    return false;

  if (pz < aabb.min.z || pz > aabb.max.z)
    return false;

  return true;
}

/// \brief  Test if point is inside the aabb
template <typename real_type>
inline bool contains_point(const axis_aligned_bounding_box3<real_type>& aabb,
                           const scalar3<real_type>& point) noexcept {
  return contains_point(aabb, point.x, point.y, point.z);
}

/// @}

} // namespace xray
} // namespace math