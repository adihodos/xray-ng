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
#include "xray/math/scalar4x4.hpp"
#include "xray/xray.hpp"
#include "xray/xray_types.hpp"

namespace xray {
namespace scene {

/// \addtogroup __GroupXrayMath
/// @{

class camera
{
  public:
    camera() noexcept = default;

    const math::vec3f& origin() const noexcept { return origin_; }

    const math::vec3f& direction() const noexcept { return direction_; }

    const math::vec3f& right() const noexcept { return right_; }

    const math::vec3f& up() const noexcept { return up_; }

    void set_view_matrix(const math::MatrixWithInvertedMatrixPair4f& view_transform_with_inverse) noexcept
    {
        set_view_matrix(view_transform_with_inverse.transform, view_transform_with_inverse.inverted);
    }
    void set_view_matrix(const math::mat4f& view, const math::mat4f& inverse_of_view) noexcept;

    void set_projection(const math::MatrixWithInvertedMatrixPair4f& projection_with_inverse) noexcept
    {
        set_projection_matrix(projection_with_inverse.transform, projection_with_inverse.inverted);
    }
    void set_projection_matrix(const math::mat4f& projection, const math::mat4f& inverse_of_projection) noexcept
    {
        projection_.transform = projection;
        projection_.inverted = inverse_of_projection;
    }

    const math::mat4f& view() const noexcept;
    const math::mat4f& projection() const noexcept;
    const math::mat4f& projection_view() const noexcept;

    const math::MatrixWithInvertedMatrixPair4f& view_bijection() const noexcept;
    const math::MatrixWithInvertedMatrixPair4f& projection_bijection() const noexcept;
    const math::MatrixWithInvertedMatrixPair4f& projection_view_bijection() const noexcept;

    void look_at(const math::vec3f& eye_pos, const math::vec3f& target, const math::vec3f& world_up) noexcept;

    // void set_fov(const float fov_rads) noexcept;
    // void set_near_far(const float nearplane, const float farplane);

  private:
    void invalidate() noexcept { updated_ = false; }

    void update() const noexcept;

  private:
    math::vec3f right_{ math::vec3f::stdc::zero };
    math::vec3f up_{ math::vec3f::stdc::zero };
    math::vec3f direction_{ math::vec3f::stdc::zero };
    math::vec3f origin_{ math::vec3f::stdc::zero };
    mutable math::MatrixWithInvertedMatrixPair4f view_;
    mutable math::MatrixWithInvertedMatrixPair4f projection_;
    mutable math::MatrixWithInvertedMatrixPair4f projection_view_;
    mutable bool updated_{ false };
};

/// @}

} // namespace scene
} // namespace xray
