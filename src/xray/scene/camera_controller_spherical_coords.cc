#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/scene/camera.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include <cassert>

using namespace xray::base;
using namespace xray::math;
using namespace xray::ui;

xray::scene::camera_controller_spherical_coords::
  camera_controller_spherical_coords(
    camera* cam, const char* config_file_path /* = nullptr */)
  : camera_controller{cam} {
  if (!config_file_path)
    return;

  config_file cam_conf{config_file_path};
  if (!cam_conf)
    return;

  {
    float spd_theta{};
    if (cam_conf.lookup_value("camera.speed_theta", spd_theta))
      _params.speed_theta_ = spd_theta;
  }
  {
    float spd_phi{};
    if (cam_conf.lookup_value("camera.speed_phi", spd_phi))
      _params.speed_phi_ = spd_phi;
  }
  {
    float spd_move{};
    if (cam_conf.lookup_value("camera.speed_move", spd_move))
      _params.speed_move_ = spd_move;
  }
  {
    float radius{};
    if (cam_conf.lookup_value("camera.radius", radius))
      _params.radius_ = radius;
  }
  {
    float radius_max{};
    if (cam_conf.lookup_value("camera.max_radius", radius_max))
      _params.radius_max_ = radius_max;
  }
  {
    float radius_min{};
    if (cam_conf.lookup_value("camera.min_radius", radius_min))
      _params.radius_min_ = radius_min;
  }
  {
    float theta{};
    if (cam_conf.lookup_value("camera.theta", theta))
      _params.theta_ = radians(theta);
  }
  {
    float phi{};
    if (cam_conf.lookup_value("camera.phi", phi))
      _params.phi_ = radians(phi);
  }
  {
    float phi_max{};
    if (cam_conf.lookup_value("camera.angle_phi_max", phi_max))
      _params.phi_max_ = radians(phi_max);
  }
  {
    float phi_min{};
    if (cam_conf.lookup_value("camera.angle_phi_min", phi_min))
      _params.phi_min_ = radians(phi_min);
  }
}

void xray::scene::camera_controller_spherical_coords::input_event(
  const ui::window_event& input_evt) {
  using namespace xray::ui;

  assert(ui::is_input_event(input_evt));

  if ((input_evt.type == event_type::mouse_motion) &&
      (input_evt.event.motion.button1)) {

    //
    // Mark the position of the first button press, to avoid annoying jumps
    // in camera position between consecutive button press events at different
    // positions.
    if (!_last_mouse_pos) {
      _last_mouse_pos =
        vec2f{static_cast<float>(input_evt.event.motion.pointer_x),
              static_cast<float>(input_evt.event.motion.pointer_y)};
    } else {
      mouse_moved(static_cast<float>(input_evt.event.motion.pointer_x),
                  static_cast<float>(input_evt.event.motion.pointer_y));
    }

    return;
  }

  //
  // Reset last mouse position to avoid camera jumping suddenly the next
  // time the mouse button is pressed.
  if (input_evt.type == event_type::mouse_button &&
      input_evt.event.button.type == event_action_type::release) {
    _last_mouse_pos = nothing{};
    return;
  }

  if (input_evt.type == event_type::mouse_wheel) {
    const auto delta = input_evt.event.wheel.delta;
    _params.radius_  = clamp(_params.radius_ + delta * _params.speed_move_,
                            _params.radius_min_,
                            _params.radius_max_);
    return;
  }
}

void xray::scene::camera_controller_spherical_coords::mouse_moved(
  const float x_pos, const float y_pos) noexcept {
  assert(_last_mouse_pos.has_value());
  const auto& lpos = _last_mouse_pos.value();

  const auto delta_x = x_pos - lpos.x;
  const auto delta_y = y_pos - lpos.y;

  _last_mouse_pos = vec2f{x_pos, y_pos};

  _params.theta_ += delta_x * _params.speed_theta_;

  if (_params.theta_ > two_pi<float>)
    _params.theta_ -= two_pi<float>;

  if (_params.theta_ < -two_pi<float>)
    _params.theta_ += two_pi<float>;

  _params.phi_ += delta_y * _params.speed_phi_;
  _params.phi_ = clamp(_params.phi_, _params.phi_min_, _params.phi_max_);
}

void xray::scene::camera_controller_spherical_coords::update() {
  assert(cam_ != nullptr);

  const auto cam_pos =
    point_from_spherical_coords(_params.radius_, _params.theta_, _params.phi_);

  cam_->look_at(cam_pos, vec3f::stdc::zero, vec3f::stdc::unit_y);
}
