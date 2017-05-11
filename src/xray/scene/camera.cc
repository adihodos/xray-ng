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

#include "xray/scene/camera.hpp"
#include "xray/math/handedness.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar4x4_math.hpp"

void xray::scene::camera::set_view_matrix(const math::mat4f& view) {
  view_ = view;

  // origin_.x = view_.a03;
  // origin_.y = view_.a13;
  // origin_.z = view_.a23;

  right_.x = view_.a00;
  right_.y = view_.a10;
  right_.z = view_.a20;

  up_.x = view_.a01;
  up_.y = view_.a11;
  up_.z = view_.a21;

  direction_.x = view_.a02;
  direction_.y = view_.a12;
  direction_.z = view_.a22;

  invalidate();
}

const xray::math::mat4f& xray::scene::camera::view() const noexcept {
  return view_;
}

const xray::math::mat4f& xray::scene::camera::projection() const noexcept {
  update();
  return projection_;
}

const xray::math::mat4f& xray::scene::camera::projection_view() const noexcept {
  update();
  return projection_view_;
}

void xray::scene::camera::update() const noexcept {
  if (updated_)
    return;

  projection_view_ = projection_ * view_;
  updated_         = true;
}

void xray::scene::camera::set_projection(
  const math::mat4f& projection) noexcept {
  projection_ = projection;
  invalidate();
}

void xray::scene::camera::look_at(const math::vec3f& eye_pos,
                                  const math::vec3f& target,
                                  const math::vec3f& world_up) noexcept {
  origin_ = eye_pos;
  set_view_matrix(math::view_frame::look_at(eye_pos, target, world_up));
}
