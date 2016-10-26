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
