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

#include "xray/scene/fps_camera_controller.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/scene/camera.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"

using namespace xray::math;
using namespace xray::ui;

xray::scene::fps_camera_controller::fps_camera_controller(
  xray::scene::camera* cam, const char*)
  : camera_controller{cam} {
  _syncstatus.state = 0;
  update_view_transform();
}

void xray::scene::fps_camera_controller::move_up() noexcept {
  _position += MOVE_SPEED * _up;
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::move_down() noexcept {
  _position -= MOVE_SPEED * _up;
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::move_forward() noexcept {
  _position += MOVE_SPEED * _dir;
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::move_backward() noexcept {
  _position -= MOVE_SPEED * _dir;
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::move_left() noexcept {
  _position -= MOVE_SPEED * _right;
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::move_right() noexcept {
  _position += MOVE_SPEED * _right;
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::yaw_left() noexcept {
  yaw(-_viewparams.yaw_speed);
}

void xray::scene::fps_camera_controller::yaw_right() noexcept {
  yaw(_viewparams.yaw_speed);
}

void xray::scene::fps_camera_controller::pitch_up() noexcept {
  pitch(_viewparams.pitch_speed);
}

void xray::scene::fps_camera_controller::pitch_down() noexcept {
  pitch(-_viewparams.pitch_speed);
}

void xray::scene::fps_camera_controller::roll_left() noexcept {
  roll(_viewparams.roll_speed);
}

void xray::scene::fps_camera_controller::roll_right() noexcept {
  roll(-_viewparams.roll_speed);
}

void xray::scene::fps_camera_controller::zoom_in() noexcept {
  static constexpr auto MIN_FOV = radians(5.0f);
  _lensparams.fov -= LENS_SPEED;
  if (_lensparams.fov < MIN_FOV) {
    _lensparams.fov = MIN_FOV;
  }
  _syncstatus.lens = 0;
}

void xray::scene::fps_camera_controller::zoom_out() noexcept {
  static constexpr auto MAX_FOV = radians(185.0f);
  _lensparams.fov += LENS_SPEED;
  if (_lensparams.fov > MAX_FOV) {
    _lensparams.fov = MAX_FOV;
  }
  _syncstatus.lens = 0;
}

void xray::scene::fps_camera_controller::reset_orientation() noexcept {
  _right           = vec3f::stdc::unit_x;
  _up              = vec3f::stdc::unit_y;
  _dir             = vec3f::stdc::unit_z;
  _position        = vec3f::stdc::zero;
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::input_event(
  const ui::window_event& evt) {
  if ((evt.type == event_type::key) &&
      (evt.event.key.type == event_action_type::press)) {

    switch (evt.event.key.keycode) {
    case key_sym::e::key_q:
      yaw_left();
      break;

    case key_sym::e::key_e:
      yaw_right();
      break;

    case key_sym::e::key_w:
      move_forward();
      break;

    case key_sym::e::key_s:
      move_backward();
      break;

    case key_sym::e::key_d:
      move_right();
      break;

    case key_sym::e::key_a:
      move_left();
      break;

    case key_sym::e::up:
      pitch_down();
      break;

    case key_sym::e::down:
      pitch_up();
      break;

    case key_sym::e::left:
      roll_left();
      break;

    case key_sym::e::right:
      roll_right();
      break;

    case key_sym::e::kp_add:
      zoom_in();
      break;

    case key_sym::e::kp_minus:
      zoom_out();
      break;

    case key_sym::e::backspace:
      reset_orientation();
      break;

    default:
      break;
    }

    return;
  }
}

void xray::scene::fps_camera_controller::update() {
  if (!_syncstatus.view) {
    update_view_transform();
    _syncstatus.view = true;
  }

  if (!_syncstatus.lens) {
    cam_->set_projection(
      projections_rh::perspective_symmetric(_lensparams.aspect_ratio,
                                            _lensparams.fov,
                                            _lensparams.nearplane,
                                            _lensparams.farplane));
    _syncstatus.lens = true;
  }
}

void xray::scene::fps_camera_controller::pitch(const float delta) noexcept {
  const auto qpitch = quaternionf{delta, _right};
  _up               = normalize(qpitch * _up);
  _dir              = normalize(qpitch * _dir);
  _syncstatus.view  = 0;
}

void xray::scene::fps_camera_controller::yaw(const float delta) noexcept {
  const auto qyaw  = quaternionf{delta, _up};
  _right           = normalize(qyaw * _right);
  _dir             = normalize(qyaw * _dir);
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::roll(const float delta) noexcept {
  const auto qroll = quaternionf{delta, _dir};
  _up              = normalize(qroll * _up);
  _right           = normalize(qroll * _right);
  _syncstatus.view = 0;
}

void xray::scene::fps_camera_controller::update_view_transform() {
  _right = normalize(_right);
  _up    = normalize(cross(_dir, _right));
  _dir   = cross(_right, _up);

  cam_->set_view_matrix(view_frame::view_matrix(_right, _up, _dir, _position));
}
