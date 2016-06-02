#include "basic_window.hpp"
#include "renderer_dx10.hpp"
#include "simple_object.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/shims/string.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar3x3_string_cast.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/scalar4x4_string_cast.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/ui/window_context.hpp"
#include <cstdint>

using namespace xray;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

namespace app {

class basic_scene {
public:
  basic_scene(const uint32_t wnd_width, const uint32_t wnd_height,
              xray::rendering::renderer* r_ptr);

  void window_resized(const int32_t new_height,
                      const int32_t new_width) noexcept;

  void key_event(const int32_t key_code, const int32_t action,
                 const int32_t modifiers) noexcept;

  void mouse_moved(const float delta_x, const float delta_y) noexcept;

  void scroll_event(const float x_off, const float y_off) noexcept;

  void tick_event(const float delta);

  void draw(const xray::ui::window_context& wnd_ctx);

  bool valid() const noexcept { return initialized_; }

  explicit operator bool() const noexcept { return valid(); }

private:
  static constexpr const char* controller_cfg_file_path =
      "config/cam_controller_spherical.conf";

private:
  bool                                            initialized_{false};
  xray::rendering::draw_context_t                 draw_ctx_;
  xray::scene::camera                             cam_;
  xray::scene::camera_controller_spherical_coords cam_control_{
      &cam_, controller_cfg_file_path};
  xray::rendering::renderer* renderer_;
  simple_object              obj_;

private:
  XRAY_NO_COPY(basic_scene);
};

basic_scene::basic_scene(const uint32_t wnd_width, const uint32_t wnd_height,
                         xray::rendering::renderer* r_ptr)
    : renderer_{r_ptr}, obj_{r_ptr} {

  if (!obj_)
    return;

  assert(renderer_ != nullptr);
  cam_control_.update();

  cam_.set_projection(projection::perspective_symmetric(
      static_cast<float>(wnd_width), static_cast<float>(wnd_height),
      radians(70.0f), 0.3f, 100.0f));

  OUTPUT_DBG_MSG("Window [%u x %u]", wnd_width, wnd_height);

  draw_ctx_.window_width  = wnd_width;
  draw_ctx_.window_height = wnd_height;

  initialized_ = true;
}

void basic_scene::window_resized(const int32_t new_height,
                                 const int32_t new_width) noexcept {
  cam_.set_projection(projection::perspective_symmetric(
      static_cast<float>(new_width), static_cast<float>(new_height),
      radians(70.0f), 0.3f, 100.0f));
}

void basic_scene::key_event(const int32_t key_code, const int32_t action,
                            const int32_t modifiers) noexcept {
  cam_control_.key_event(key_code, action, modifiers);
}

void basic_scene::mouse_moved(const float delta_x,
                              const float delta_y) noexcept {
  cam_control_.mouse_moved(delta_x, delta_y);
}

void basic_scene::scroll_event(const float x_off, const float y_off) noexcept {
  cam_control_.scroll_event(x_off, y_off);
}

void basic_scene::tick_event(const float delta) {
  cam_control_.update();

  draw_ctx_.view_matrix       = cam_.view();
  draw_ctx_.projection_matrix = cam_.projection();
  draw_ctx_.proj_view_matrix  = cam_.projection_view();

  obj_.update(delta);
}

void basic_scene::draw(const xray::ui::window_context& wnd_ctx) {
  renderer_->clear_backbuffer(0.0f, 0.0f, 0.0f);

  obj_.draw(draw_ctx_);

  renderer_->swap_buffers();
}

} // namespace app

template <typename enter_fn, typename exit_fn>
struct enter_exit_action {
public:
  enter_exit_action(enter_fn fun_enter, exit_fn fun_exit) noexcept
      : fn_exit_{std::move(fun_exit)} {
    fun_enter();
  }

  enter_exit_action(enter_exit_action&& eea) noexcept
      : fn_exit_{std::move(eea.fn_exit_)} {
    eea.exec_ = false;
  }

  ~enter_exit_action() {
    if (exec_)
      fn_exit_();
  }

private:
  exit_fn fn_exit_;
  bool    exec_{true};
};

template <typename enter_fn, typename exit_fn>
inline enter_exit_action<enter_fn, exit_fn>
make_scoped_enter_exit(enter_fn func_enter, exit_fn func_exit) noexcept {
  return enter_exit_action<enter_fn, exit_fn>{
      std::forward<enter_fn>(func_enter), std::forward<exit_fn>(func_exit)};
}

int main(int, char**) {
  using namespace xray::ui;
  using namespace xray::base;

  basic_window app_wnd{};

  if (!app_wnd)
    return EXIT_FAILURE;

  const auto r_params = render_params_t{app_wnd.width(), app_wnd.height(),
                                        app_wnd.handle(), true};

  renderer rd{r_params};
  if (!rd)
    return EXIT_FAILURE;

  app::basic_scene scene{app_wnd.width(), app_wnd.height(), &rd};
  if (!scene)
    return EXIT_FAILURE;

  // return EXIT_SUCCESS;

  // auto hide_restore_cursor =
  //     make_scoped_enter_exit([&app_wnd]() { app_wnd.disable_cursor(); },
  //                            [&app_wnd]() { app_wnd.enable_cursor(); });

  // struct {
  //     key_delegate              keys;
  //     mouse_button_delegate     mouse_button;
  //     mouse_scroll_delegate     mouse_scroll;
  //     mouse_enter_exit_delegate mouse_enter_exit;
  //     mouse_move_delegate       mouse_move;
  //     window_size_delegate      window_resize;
  //     draw_delegate_type        draw;
  //     tick_delegate_type        tick;
  //     debug_delegate_type       debug;
  //   } event_delegates;

  app_wnd.events.keys = make_delegate(scene, &app::basic_scene::key_event);
  app_wnd.events.mouse_scroll =
      make_delegate(scene, &app::basic_scene::scroll_event);
  app_wnd.events.mouse_move =
      make_delegate(scene, &app::basic_scene::mouse_moved);
  app_wnd.events.window_resize =
      make_delegate(scene, &app::basic_scene::window_resized);
  app_wnd.events.tick = make_delegate(scene, &app::basic_scene::tick_event);
  app_wnd.events.draw = make_delegate(scene, &app::basic_scene::draw);

  app_wnd.pump_messages();

  return EXIT_SUCCESS;
}
