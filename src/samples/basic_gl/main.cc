#include "animated_plane.hpp"
// #include "cap3/frag_discard/frag_discard_demo.hpp"
// #include "cap3/soubroutines/soubroutines_demo.hpp"
// #include "cap4/directional_lights/directional_lights_demo.hpp"
// #include "cap4/fog/fog_demo.hpp"
// #include "cap4/multiple_lights/multiple_lights_demo.hpp"
// #include "cap4/per_fragment_lighting/per_fragment_lighting_demo.hpp"
// #include "cap4/spotlight/spotlight_demo.hpp"
// #include "cap4/toon_shading/toon_shading_demo.hpp"
// #include "cap5/discard_alphamap/discard_alphamap_demo.hpp"
// #include "cap5/multiple_textures/multiple_textures_demo.hpp"
// #include "cap5/normal_map/normal_map_demo.hpp"
// #include "cap5/reflection/reflection_demo.hpp"
// #include "cap5/refraction/refraction_demo.hpp"
// #include "cap5/render_texture/render_texture_demo.hpp"
// #include "cap5/textures/textures_demo.hpp"
#include "cap6/edge_detect/edge_detect_demo.hpp"
// #include "cap6/hdr_tonemapping/hdr_tonemapping_demo.hpp"
#include "colored_circle.hpp"
#include "debug_record.hpp"
#include "fractal.hpp"
#include "init_context.hpp"
#include "lit_torus.hpp"
#include "resize_context.hpp"
#include "subroutine_test.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/perf_stats_collector.hpp"
#include "xray/base/shims/string.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar3x3_string_cast.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/scalar4x4_string_cast.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/scene/point_light.hpp"
#include "xray/ui/basic_gl_window.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/window.hpp"
#include "xray/ui/window_context.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <imgui/imgui.h>
#include <thread>

using namespace xray;
using namespace xray::base;
using namespace xray::math;
using namespace xray::scene;
using namespace xray::ui;
using namespace xray::rendering;

xray::base::app_config* xr_app_config{nullptr};

namespace app {

class basic_scene {
public:
  basic_scene(xray::ui::window* wnd);
  ~basic_scene() noexcept;

  void window_resized(const int32_t new_height,
                      const int32_t new_width) noexcept;

  void main_loop(const xray::ui::window_loop_event& loop_evt);
  void window_event_handler(const xray::ui::window_event& wnd_evt);

  void tick_event(const float delta);

  void input_event(const xray::ui::input_event_t& in_evt) noexcept;

  void draw(const xray::ui::window_context& wnd_ctx);

  bool valid() const noexcept { return _initialized; }

  explicit operator bool() const noexcept { return valid(); }

private:
  static constexpr const char* controller_cfg_file_path =
      "config/cam_controller_spherical.conf";

  using compose_ui_event = xray::base::fast_delegate<void(void)>;

  struct {
    compose_ui_event compose_ui;
  } events;

  void setup_ui();

private:
  xray::ui::window* _wnd;
  //  lit_object                                      obj_;
  //  soubroutines_demo                               obj_;
  //  frag_discard_demo                               obj_;
  //  multiple_lights_demo                            obj_;
  //  directional_light_demo                          obj_;
  //  per_fragment_lighting_demo                      obj_;
  //  toon_shading_demo                               obj_;
  //  spotlight_demo                                  obj_;
  //  fog_demo                                        obj_;
  //  textures_demo obj_;
  //  multiple_textures_demo obj_;
  //  discard_alphamap_demo obj_;
  //    normal_map_demo obj_;
  //    reflection_demo                                 obj_;
  //  refraction_demo obj_;
  //  render_texture_demo obj_;

  // hdr_tonemap                                     obj_;
  xray::scene::camera                             _cam;
  xray::scene::camera_controller_spherical_coords _cam_control{
      &_cam, controller_cfg_file_path};
  // xray::ui::imgui_backend                      _ui;
  xray::base::stats_thread                     _stats_collector;
  xray::base::stats_thread::process_stats_info _proc_stats;
  xray::base::unique_pointer<demo_base>        _demo;
  bool                                         _ui_active{false};
  bool                                         _initialized{false};

private:
  XRAY_NO_COPY(basic_scene);
};

basic_scene::~basic_scene() noexcept { _stats_collector.signal_stop(); }

basic_scene::basic_scene(xray::ui::window* wnd) : _wnd{wnd} {
  _cam_control.update();
  // _cam.set_projection(projection::perspective_symmetric(
  //     static_cast<float>(_appwnd->width()),
  //     static_cast<float>(_appwnd->height()), radians(70.0f), 0.3f, 1000.0f));

  // gl::Viewport(0, 0, static_cast<int32_t>(_appwnd->width()),
  //              static_cast<int32_t>(_appwnd->height()));

  // gl::PolygonMode(gl::FRONT_AND_BACK, gl::FILL);
  // gl::Enable(gl::DEPTH_TEST);
  // gl::Enable(gl::CULL_FACE);

  // _demo = make_unique<edge_detect_demo>(
  //     init_context_t{_appwnd->width(), _appwnd->height(), xr_app_config});

  // if (!*_demo) {
  //   XR_LOG_CRITICAL("Failed to created demo!");
  //   XR_NOT_REACHED();
  // }

  events.compose_ui = make_delegate(*_demo, &demo_base::compose_ui);
  _initialized      = true;
}

void basic_scene::window_resized(const int32_t new_width,
                                 const int32_t new_height) noexcept {

  gl::Viewport(0, 0, new_width, new_height);
  _cam.set_projection(projection::perspective_symmetric(
      static_cast<float>(new_width), static_cast<float>(new_height),
      radians(70.0f), 0.3f, 1000.0f));

  _demo->resize_event(resize_context_t{static_cast<uint32_t>(new_width),
                                       static_cast<uint32_t>(new_height)});
}

void basic_scene::tick_event(const float delta) {
  _cam_control.update();

  draw_context_t draw_ctx{0, 0,
                          // _appwnd->width(),
                          //                       _appwnd->height(),
                          _cam.view(), _cam.projection(),
                          _cam.projection_view(), &_cam, nullptr};

  _proc_stats = _stats_collector.process_stats();

  if (_ui_active) {
    // _ui.new_frame(draw_ctx);
    // _ui.tick(delta);

    if (events.compose_ui)
      events.compose_ui();
  }

  _demo->update(delta);
}

void basic_scene::draw(const xray::ui::window_context& /* wnd_ctx */) {
  gl::ClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  gl::ClearDepth(1.0f);
  gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

  // draw_context_t draw_ctx{_appwnd->width(),
  //                         _appwnd->height(),
  //                         _cam.view(),
  //                         _cam.projection(),
  //                         _cam.projection_view(),
  //                         &_cam,
  //                         nullptr};

  // _demo->draw(draw_ctx);
  // if (_ui_active) {
  //   _ui.draw_event(draw_ctx);
  // }
}

// void basic_scene::input_event(
//     const xray::ui::input_event_t& in_event) noexcept {
//   if (in_event.type == input_event_type::key) {
//     const auto key_evt = in_event.key_ev;

//     if (key_evt.down) {
//       const auto key_code = key_evt.key_code;

//       switch (key_code) {
//       case key_symbol::key_o: {
//         _ui_active = !_ui_active;
//         return;
//       } break;

//       case key_symbol::escape: {
//         _appwnd->quit();
//         return;
//       } break;

//       default:
//         break;
//       }
//     }
//   }

//   if (_ui_active) {
//     _ui.input_event(in_event);
//     if (_ui.wants_input())
//       return;
//   }

//   _cam_control.input_event(in_event);
// }

void basic_scene::setup_ui() {
  // ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiSetCond_FirstUseEver);
  // ImGui::SetNextWindowSize(ImVec2(100.0f, 100.0f),
  // ImGuiSetCond_FirstUseEver);
  // ImGui::Begin("Render target clear color :");
  // ImGui::SliderFloat("Red", &_clear_color.r, 0.0f, 1.0f, "%3.3f");
  // ImGui::Separator();
  // ImGui::SliderFloat("Green", &_clear_color.g, 0.0f, 1.0f, "%3.3f");
  // ImGui::Separator();
  // ImGui::SliderFloat("Blue", &_clear_color.b, 0.0f, 1.0f, "%3.3f");
  // ImGui::End();
}

void basic_scene::main_loop(const xray::ui::window_loop_event& loop_evt) {
  std::this_thread::sleep_for(std::chrono::milliseconds{500});
}

void basic_scene::window_event_handler(const xray::ui::window_event& wnd_evt) {
  XR_LOG_INFO(__PRETTY_FUNCTION__);
}
}

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

int main(int argc, char** argv) {
  using namespace xray::ui;
  using namespace xray::base;

  XR_LOGGER_START(argc, argv);
  XR_LOGGER_CONFIG_FILE("config/logging.conf");
  XR_LOG_INFO("Starting up ...");

  app_config app_cfg{"config/app_config.conf"};
  xr_app_config = &app_cfg;

  /*
  struct window_params_t {
    const char* title{"X11 OpenGL window"};
    int32_t     ver_major{4};
    int32_t     ver_minor{0};
    int32_t     depth_bits{24};
    int32_t     stencil_bits{8};
    int32_t     color_bits{32};
    int32_t     sample_count{0};
    uint32_t    debug_output_level{0};

    window_params_t() noexcept = default;
  };
  */
  window_params_t wnd_params = {"OpenGL Demo", 4, 5, 24, 8, 32, 0, 0};
  window          main_window{wnd_params};
  if (!main_window) {
    XR_LOG_ERR("Failed to initialize application window!");
    return EXIT_FAILURE;
  }

  app::basic_scene scene{&main_window};
  main_window.events.loop = make_delegate(scene, &app::basic_scene::main_loop);
  main_window.events.window =
      make_delegate(scene, &app::basic_scene::window_event_handler);

  main_window.message_loop();

  // app::basic_scene scene{&app_wnd};
  // if (!scene) {
  //   XR_LOG_CRITICAL("Failed to create scene !");
  //   return EXIT_FAILURE;
  // }

  // //  auto hide_restore_cursor =
  // //      make_scoped_enter_exit([&app_wnd]() { app_wnd.disable_cursor(); },
  // //                             [&app_wnd]() { app_wnd.enable_cursor(); });

  // //  struct {
  // //    key_delegate              keys;
  // //    mouse_button_delegate     mouse_button;
  // //    mouse_scroll_delegate     mouse_scroll;
  // //    mouse_enter_exit_delegate mouse_enter_exit;
  // //    mouse_move_delegate       mouse_move;
  // //    window_size_delegate      window_resize;
  // //    draw_delegate_type        draw;
  // //    tick_delegate_type        tick;
  // //    debug_delegate_type       debug;
  // //  } event_delegates;

  // app_wnd.events.window_resize =
  //     make_delegate(scene, &app::basic_scene::window_resized);
  // app_wnd.events.tick  = make_delegate(scene, &app::basic_scene::tick_event);
  // app_wnd.events.draw  = make_delegate(scene, &app::basic_scene::draw);
  // app_wnd.events.input = make_delegate(scene,
  // &app::basic_scene::input_event);

  // app_wnd.pump_messages();

  XR_LOG_INFO("Shutting down ...");

  return EXIT_SUCCESS;
}
