//
// Copyright (c) 2011-2016 Adrian Hodos
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

/// \file main.cc

// #include "animated_plane.hpp"
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
// #include "cap6/edge_detect/edge_detect_demo.hpp"
// #include "cap6/hdr_tonemapping/hdr_tonemapping_demo.hpp"
// #include "colored_circle.hpp"
#include "cap2/colored_circle/colored_circle_demo.hpp"
#include "debug_record.hpp"
#include "lighting/directional_light_demo.hpp"
#include "misc/bufferless_draw/bufferless-draw-demo.hpp"
#include "misc/fractals/fractal_demo.hpp"
#include "misc/instanced_drawing/instanced_drawing_demo.hpp"
#include "misc/mesh/mesh_demo.hpp"
#include "misc/procedural_city/procedural_city_demo.hpp"
#include "misc/texture_array/texture_array_demo.hpp"
// #include "fractal.hpp"
#include "init_context.hpp"
// #include "lit_torus.hpp"
#include "resize_context.hpp"
// #include "subroutine_test.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/perf_stats_collector.hpp"
#include "xray/base/scoped_guard.hpp"
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
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/scene/point_light.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/window.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <imgui/imgui.h>
#include <opengl/opengl.hpp>
#include <tbb/tbb.h>
#include <thread>

using namespace xray;
using namespace xray::base;
using namespace xray::math;
using namespace xray::scene;
using namespace xray::ui;
using namespace xray::rendering;

xray::base::app_config* xr_app_config{nullptr};

namespace app {

enum class demo_type : int32_t {
  none,
  colored_circle,
  fractal,
  texture_array,
  mesh,
  bufferless_draw,
  lighting_directional,
  procedural_city,
  instanced_drawing
};

class basic_scene {
public:
  basic_scene(xray::ui::window* wnd);
  ~basic_scene() noexcept;

  void main_loop(const xray::ui::window_loop_event& loop_evt);

  void window_event_handler(const xray::ui::window_event& wnd_evt);

  void tick(const float delta);

  void draw(const xray::ui::window_loop_event& loop_evt);

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

  unique_pointer<demo_base> make_demo(const demo_type dtype);

private:
  xray::ui::window*                               _wnd;
  xray::scene::camera                             _cam;
  xray::scene::camera_controller_spherical_coords _cam_control{
    &_cam, controller_cfg_file_path};
  xray::ui::imgui_backend                      _ui;
  xray::base::stats_thread                     _stats_collector;
  xray::base::stats_thread::process_stats_info _proc_stats;
  xray::base::timer_highp                      _timer;
  xray::rendering::rgb_color                   _clear_color{
    xray::rendering::color_palette::material::orange700};

  xray::base::unique_pointer<demo_base> _demo;
  demo_type                             _demotype{demo_type::none};
  bool                                  _initialized{false};

private:
  XRAY_NO_COPY(basic_scene);
};

basic_scene::~basic_scene() noexcept { _stats_collector.signal_stop(); }

basic_scene::basic_scene(xray::ui::window* wnd) : _wnd{wnd} {
  _cam_control.update();
  _cam.set_projection(
    projection::perspective_symmetric(static_cast<float>(_wnd->width()),
                                      static_cast<float>(_wnd->height()),
                                      radians(70.0f),
                                      0.3f,
                                      1000.0f));

  _initialized = true;
}

void basic_scene::tick(const float delta) {
  _cam_control.update();
  _proc_stats = _stats_collector.process_stats();

  _ui.new_frame(_wnd->width(), _wnd->height());
  _ui.tick(delta);

  if (_demo)
    _demo->compose_ui();
  else
    setup_ui();

  if (_demo)
    _demo->update(delta);
}

void basic_scene::draw(const xray::ui::window_loop_event& levt) {
  if (_demo) {
    const draw_context_t draw_ctx{(uint32_t) levt.wnd_width,
                                  (uint32_t) levt.wnd_height,
                                  _cam.view(),
                                  _cam.projection(),
                                  _cam.projection_view(),
                                  &_cam,
                                  nullptr};
    _demo->draw(draw_ctx);
  } else {
    const xray::math::vec4f viewport{0.0f,
                                     0.0f,
                                     static_cast<float>(levt.wnd_width),
                                     static_cast<float>(levt.wnd_height)};
    gl::ViewportIndexedfv(0, viewport.components);
    gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, _clear_color.components);
    gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0xffffffff);
  }

  _ui.draw();
}

void basic_scene::window_event_handler(const xray::ui::window_event& event) {
  using namespace xray::ui;

  if (is_input_event(event)) {
    if (event.type == event_type::key) {
      const auto key_evt = event.event.key;
      if (key_evt.type == event_action_type::press) {
        const auto key_code = key_evt.keycode;

        switch (key_code) {
        case key_sym::e::escape: {
          if (!_demo) {
            _wnd->quit();
          } else {
            _demo     = nullptr;
            _demotype = demo_type::none;
          }
          return;
        } break;

        default:
          break;
        }
      }
    }

    _ui.input_event(event);
    if (_ui.wants_input())
      return;

    _cam_control.input_event(event);
  }

  if (event.type == event_type::configure) {
    const auto cfg_evt = event.event.configure;
    _cam.set_projection(
      projection::perspective_symmetric(static_cast<float>(cfg_evt.width),
                                        static_cast<float>(cfg_evt.height),
                                        radians(70.0f),
                                        0.3f,
                                        1000.0f));
  }

  if (_demo) {
    _demo->event_handler(event);
  }
}

unique_pointer<app::demo_base> basic_scene::make_demo(const demo_type dtype) {

  const init_context_t init_context{
    (uint32_t) _wnd->width(), (uint32_t) _wnd->height(), xr_app_config};

  switch (dtype) {
  case demo_type::colored_circle:
    return xray::base::make_unique<colored_circle_demo>();
    break;

  case demo_type::fractal:
    return xray::base::make_unique<fractal_demo>();
    break;

  case demo_type::texture_array:
    return xray::base::make_unique<texture_array_demo>();
    break;

  case demo_type::mesh:
    return xray::base::make_unique<mesh_demo>();
    break;

  case demo_type::bufferless_draw:
    return xray::base::make_unique<bufferless_draw_demo>();
    break;

  case demo_type::lighting_directional:
    return xray::base::make_unique<directional_light_demo>();
    break;

  case demo_type::procedural_city:
    return xray::base::make_unique<procedural_city_demo>(init_context);
    break;

  case demo_type::instanced_drawing:
    return xray::base::make_unique<instanced_drawing_demo>(&init_context);
    break;

  default:
    break;
  }

  return nullptr;
}

void basic_scene::setup_ui() {

  ImGui::SetNextWindowPosCenter(ImGuiSetCond_Always);
  ImGui::SetNextWindowSize({300, 100}, ImGuiSetCond_Always);
  ImGui::Begin("Select a demo to run");

  XRAY_SCOPE_EXIT { ImGui::End(); };

  const char* demo_list[] = {"None",
                             "Colored Circle",
                             "Julia fractal",
                             "Texture array",
                             "Mesh",
                             "Bufferless drawing",
                             "Directional lighting",
                             "Procedural city",
                             "Instanced drawing"};

  demo_type selected_demo{_demotype};
  if (ImGui::Combo(
        "", (int32_t*) &selected_demo, demo_list, XR_I32_COUNTOF(demo_list))) {
    if (selected_demo != _demotype) {
      auto new_demo = make_demo(selected_demo);

      if (!new_demo || !new_demo->valid())
        return;

      _demo     = std::move(new_demo);
      _demotype = selected_demo;
      XR_LOG_INFO("Selection index {}", (int32_t) selected_demo);
    }
  }
}

void basic_scene::main_loop(const xray::ui::window_loop_event& loop_evt) {
  _timer.update_and_reset();
  tick(_timer.elapsed_millis());
  draw(loop_evt);
}

void debug_proc(GLenum source,
                GLenum type,
                GLuint id,
                GLenum severity,
                GLsizei /*length*/,
                const GLchar* message,
                const void* /*userParam*/) {

  auto msg_source = [source]() {
    switch (source) {
    case gl::DEBUG_SOURCE_API:
      return "API";
      break;

    case gl::DEBUG_SOURCE_APPLICATION:
      return "APPLICATION";
      break;

    case gl::DEBUG_SOURCE_OTHER:
      return "OTHER";
      break;

    case gl::DEBUG_SOURCE_SHADER_COMPILER:
      return "SHADER COMPILER";
      break;

    case gl::DEBUG_SOURCE_THIRD_PARTY:
      return "THIRD PARTY";
      break;

    case gl::DEBUG_SOURCE_WINDOW_SYSTEM:
      return "WINDOW SYSTEM";
      break;

    default:
      return "UNKNOWN";
      break;
    }
  }();

  const auto msg_type = [type]() {
    switch (type) {
    case gl::DEBUG_TYPE_ERROR:
      return "ERROR";
      break;

    case gl::DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "DEPRECATED BEHAVIOUR";
      break;

    case gl::DEBUG_TYPE_MARKER:
      return "MARKER";
      break;

    case gl::DEBUG_TYPE_OTHER:
      return "OTHER";
      break;

    case gl::DEBUG_TYPE_PERFORMANCE:
      return "PERFORMANCE";
      break;

    case gl::DEBUG_TYPE_POP_GROUP:
      return "POP GROUP";
      break;

    case gl::DEBUG_TYPE_PORTABILITY:
      return "PORTABILITY";
      break;

    case gl::DEBUG_TYPE_PUSH_GROUP:
      return "PUSH GROUP";
      break;

    case gl::DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "UNDEFINED BEHAVIOUR";
      break;

    default:
      return "UNKNOWN";
      break;
    }
  }();

  auto msg_severity = [severity]() {
    switch (severity) {
    case gl::DEBUG_SEVERITY_HIGH:
      return "HIGH";
      break;

    case gl::DEBUG_SEVERITY_LOW:
      return "LOW";
      break;

    case gl::DEBUG_SEVERITY_MEDIUM:
      return "MEDIUM";
      break;

    case gl::DEBUG_SEVERITY_NOTIFICATION:
      return "NOTIFICATION";
      break;

    default:
      return "UNKNOWN";
      break;
    }
  }();

  const auto full_msg =
    fmt::format("OpenGL debug message\n[MESSAGE] : {}\n[SOURCE] : {}\n[TYPE] : "
                "{}\n[SEVERITY] "
                ": {}\n[ID] : {}",
                message,
                msg_source,
                msg_type,
                msg_severity,
                id);

  XR_LOG_DEBUG(full_msg);
}

} // namespace app

int main(int argc, char** argv) {
  using namespace xray::ui;
  using namespace xray::base;

  XR_LOGGER_START(argc, argv);
  XR_LOGGER_CONFIG_FILE("config/logging.conf");
  XR_LOG_INFO("Starting up ...");

  tbb::task_scheduler_init tbb_initializer{};

  app_config app_cfg{"config/app_config.conf"};
  xr_app_config = &app_cfg;

  const window_params_t wnd_params{"OpenGL Demo", 4, 5, 24, 8, 32, 0, 1, false};

  window main_window{wnd_params};
  if (!main_window) {
    XR_LOG_ERR("Failed to initialize application window!");
    return EXIT_FAILURE;
  }

  gl::DebugMessageCallback(app::debug_proc, nullptr);

  app::basic_scene scene{&main_window};
  main_window.events.loop = make_delegate(scene, &app::basic_scene::main_loop);
  main_window.events.window =
    make_delegate(scene, &app::basic_scene::window_event_handler);

  main_window.message_loop();

  XR_LOG_INFO("Shutting down ...");
  return EXIT_SUCCESS;
}
