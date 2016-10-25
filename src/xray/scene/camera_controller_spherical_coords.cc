#include "xray/base/config_settings.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include <cassert>

using namespace xray::base;
using namespace xray::math;
using namespace xray::ui;

xray::scene::camera_controller_spherical_coords::
  camera_controller_spherical_coords(
    camera* cam, const char* config_file_path /* = nullptr */) noexcept
  : cam_{cam} {
  if (!config_file_path)
    return;

  config_file cam_conf{config_file_path};
  if (!cam_conf)
    return;

  // struct controller_params_t {
  //   float radius_{5.0f};
  //   float theta_{0.0f};
  //   float phi_{0.5f};
  //   float phi_max_{175.0f};
  //   float phi_min_{5.0f};
  //   float radius_max_{50.0f};
  //   float radius_min_{1.0f};
  //   float speed_theta_{0.1f};
  //   float speed_phi_{0.1f};
  //   float speed_move{0.2f};

  //   controller_params_t() noexcept = default;
  // };

  {
    float spd_theta{};
    if (cam_conf.lookup_value("camera.speed_theta", spd_theta))
      params_.speed_theta_ = spd_theta;
  }
  {
    float spd_phi{};
    if (cam_conf.lookup_value("camera.speed_phi", spd_phi))
      params_.speed_phi_ = spd_phi;
  }
  {
    float spd_move{};
    if (cam_conf.lookup_value("camera.speed_move", spd_move))
      params_.speed_move_ = spd_move;
  }
  {
    float radius{};
    if (cam_conf.lookup_value("camera.radius", radius))
      params_.radius_ = radius;
  }
  {
    float radius_max{};
    if (cam_conf.lookup_value("camera.max_radius", radius_max))
      params_.radius_max_ = radius_max;
  }
  {
    float radius_min{};
    if (cam_conf.lookup_value("camera.min_radius", radius_min))
      params_.radius_min_ = radius_min;
  }
  {
    float theta{};
    if (cam_conf.lookup_value("camera.theta", theta))
      params_.theta_ = radians(theta);
  }
  {
    float phi{};
    if (cam_conf.lookup_value("camera.phi", phi))
      params_.phi_ = radians(phi);
  }
  {
    float phi_max{};
    if (cam_conf.lookup_value("camera.angle_phi_max", phi_max))
      params_.phi_max_ = radians(phi_max);
  }
  {
    float phi_min{};
    if (cam_conf.lookup_value("camera.angle_phi_min", phi_min))
      params_.phi_min_ = radians(phi_min);
  }
}

void xray::scene::camera_controller_spherical_coords::input_event(
  const ui::window_event& input_evt) noexcept {
  using namespace xray::ui;

  assert(ui::is_input_event(input_evt));

  if ((input_evt.type == event_type::mouse_motion) &&
      (input_evt.event.motion.button1)) {
    mouse_moved(static_cast<float>(input_evt.event.motion.pointer_x),
                static_cast<float>(input_evt.event.motion.pointer_y));
    return;
  }

  if (input_evt.type == event_type::mouse_wheel) {
    const auto delta = input_evt.event.wheel.delta;
    params_.radius_  = clamp(params_.radius_ + delta * params_.speed_move_,
                            params_.radius_min_,
                            params_.radius_max_);
    return;
  }
}

void xray::scene::camera_controller_spherical_coords::mouse_moved(
  const float x_pos, const float y_pos) noexcept {
  const auto delta_x = x_pos - last_mouse_pos_.x;
  const auto delta_y = y_pos - last_mouse_pos_.y;

  last_mouse_pos_ = {x_pos, y_pos};
  // XR_LOG_INFO(" x {}, y {}", x_pos, y_pos);

  params_.theta_ += delta_x * params_.speed_theta_;

  if (params_.theta_ > two_pi<float>)
    params_.theta_ -= two_pi<float>;

  if (params_.theta_ < -two_pi<float>)
    params_.theta_ += two_pi<float>;

  // XR_LOG_INFO("Theta {}, phi {}", params_.theta_, params_.phi_);

  params_.phi_ += delta_y * params_.speed_phi_;
  params_.phi_ = clamp(params_.phi_, params_.phi_min_, params_.phi_max_);
}

void xray::scene::camera_controller_spherical_coords::update() noexcept {
  assert(cam_ != nullptr);
  const auto cam_pos =
    point_from_spherical_coords(params_.radius_, params_.theta_, params_.phi_);

  cam_->look_at(cam_pos, vec3f::stdc::zero, vec3f::stdc::unit_y);
}
