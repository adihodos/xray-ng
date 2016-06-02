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

#include "xray/xray.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/xray_types.hpp"

namespace xray {
namespace scene {

/// \addtogroup __GroupXrayMath
/// @{

class camera {
public:
  camera() noexcept = default;

  const math::float3& origin() const noexcept { return origin_; }

  const math::float3& direction() const noexcept { return direction_; }

  const math::float3& right() const noexcept { return right_; }

  const math::float3& up() const noexcept { return up_; }

  void set_view_matrix(const math::float4x4& view);
  const math::float4x4& view() const noexcept;

  void set_projection(const math::float4x4& projection) noexcept;
  const math::float4x4& projection() const noexcept;

  const math::float4x4& projection_view() const noexcept;

  void look_at(const math::float3& eye_pos, const math::float3& target,
               const math::float3& world_up) noexcept;

private:
  void invalidate() noexcept { updated_ = false; }

  void update() const noexcept;

private:
  math::float3           right_{math::float3::stdc::zero};
  math::float3           up_{math::float3::stdc::zero};
  math::float3           direction_{math::float3::stdc::zero};
  math::float3           origin_{math::float3::stdc::zero};
  mutable math::float4x4 view_{math::float4x4::stdc::identity};
  mutable math::float4x4 projection_{math::float4x4::stdc::identity};
  mutable math::float4x4 projection_view_{math::float4x4::stdc::identity};
  mutable bool           updated_{false};
};

/// @}

} // namespace scene
} // namespace xray
