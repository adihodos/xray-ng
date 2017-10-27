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

#include "xray/xray.hpp"
#include "debug_record.hpp"
#include "demo_base.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/ui.hpp"
#include "xray/ui/window.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <imgui/imgui.h>
#include <opengl/opengl.hpp>
#include <tbb/tbb.h>
#include <thread>

#include "misc/mesh/mesh_demo.hpp"

using namespace xray;
using namespace xray::base;
using namespace xray::math;
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
  lighting_point,
  procedural_city,
  instanced_drawing,
  geometric_shapes
};

class main_app {
public:
  explicit main_app(xray::ui::window* wnd);

  bool valid() const noexcept { return _initialized; }

  explicit operator bool() const noexcept { return valid(); }

private:
  bool demo_running() const noexcept { return _demo != nullptr; }
  void run_demo(const demo_type type);
  void hookup_event_delegates();
  void demo_quit();

  /// \group Event handlers
  /// @{

  void main_loop(const xray::ui::window_loop_event& loop_evt);

  void window_event_handler(const xray::ui::window_event& wnd_evt);

  void poll_start(const xray::ui::poll_start_event&);

  void poll_end(const xray::ui::poll_end_event&);

  /// @}

  void draw(const xray::ui::window_loop_event& loop_evt);

private:
  xray::ui::window*                     _window;
  xray::base::unique_pointer<demo_base> _demo;
  xray::ui::user_interface              _app_ui;
  xray::rendering::rgb_color            _clear_color{
    xray::rendering::color_palette::material::orange700};

  bool _initialized{false};

  XRAY_NO_COPY(main_app);
};

main_app::main_app(xray::ui::window* wnd) : _window{wnd} {
  hookup_event_delegates();
}

void main_app::demo_quit() {
  assert(demo_running());
  hookup_event_delegates();
}

void main_app::hookup_event_delegates() {
  _window->events.loop       = make_delegate(*this, &main_app::main_loop);
  _window->events.poll_start = make_delegate(*this, &main_app::poll_start);
  _window->events.poll_end   = make_delegate(*this, &main_app::poll_end);
  _window->events.window =
    make_delegate(*this, &main_app::window_event_handler);
}

void main_app::poll_start(const xray::ui::poll_start_event&) {
  _app_ui.input_begin();
}

void main_app::poll_end(const xray::ui::poll_end_event&) {
  _app_ui.input_end();
}

void main_app::window_event_handler(const xray::ui::window_event& wnd_evt) {
  if (is_input_event(wnd_evt)) {
    _app_ui.ui_event(wnd_evt);
  }
}

void main_app::main_loop(const xray::ui::window_loop_event& levt) {
  {
    auto ctx = _app_ui.ctx();

    XRAY_SCOPE_EXIT { nk_end(ctx); };

    const char* demo_list[] = {"None",
                               "Colored Circle",
                               "Julia fractal",
                               "Texture array",
                               "Mesh",
                               "Bufferless drawing",
                               "Directional lighting",
                               "Point lighting",
                               "Procedural city",
                               "Instanced drawing",
                               "Geometric shapes generation"};

    if (nk_begin(ctx,
                 "Select a demo to run",
                 nk_rect(0.0f, 0.0f, 300.0f, 300.0f),
                 NK_WINDOW_BORDER | NK_WINDOW_MINIMIZABLE | NK_WINDOW_SCALABLE |
                   NK_WINDOW_MOVABLE)) {
      nk_layout_row_static(ctx, 25, 200, 1);
      const auto selected_demo = nk_combo(
        ctx, demo_list, XR_I32_COUNTOF(demo_list), 0, 25, nk_vec2(200, 200));

      run_demo(static_cast<demo_type>(selected_demo));
    }
  }

  const xray::math::vec4f viewport{0.0f,
                                   0.0f,
                                   static_cast<float>(levt.wnd_width),
                                   static_cast<float>(levt.wnd_height)};

  gl::ViewportIndexedfv(0, viewport.components);
  gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, _clear_color.components);
  gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0xffffffff);
  _app_ui.render(levt.wnd_width, levt.wnd_height);
}

void main_app::run_demo(const demo_type type) {
  auto make_demo_fn =
    [ this, w = _window ](const demo_type dtype)->unique_pointer<demo_base> {

    const init_context_t init_context{
      (uint32_t) w->width(),
      (uint32_t) w->height(),
      xr_app_config,
      make_delegate(*this, &main_app::demo_quit)};

    switch (dtype) {

      //    case demo_type::colored_circle:
      //      return xray::base::make_unique<colored_circle_demo>();
      //      break;

      //    case demo_type::fractal:
      //      return xray::base::make_unique<fractal_demo>();
      //      break;

      //    case demo_type::texture_array:
      //      return xray::base::make_unique<texture_array_demo>();
      //      break;

    case demo_type::mesh:
      return xray::base::make_unique<mesh_demo>(&init_context);
      break;

      //    case demo_type::bufferless_draw:
      //      return xray::base::make_unique<bufferless_draw_demo>();
      //      break;

      //    case demo_type::lighting_directional:
      //      return xray::base::make_unique<directional_light_demo>();
      //      break;

      //    case demo_type::procedural_city:
      //      return
      //      xray::base::make_unique<procedural_city_demo>(init_context);
      //      break;

      //    case demo_type::instanced_drawing:
      //      return
      //      xray::base::make_unique<instanced_drawing_demo>(&init_context);
      //      break;

      //    case demo_type::geometric_shapes:
      //      return
      //      xray::base::make_unique<geometric_shapes_demo>(&init_context);
      //      break;

      //    case demo_type::lighting_point:
      //      return xray::base::make_unique<point_light_demo>(&init_context);
      //      break;

    default:
      break;
    }

    return nullptr;
  };

  _demo = make_demo_fn(type);
  if (!_demo) {
    return;
  }

  _window->events.loop       = make_delegate(*_demo, &demo_base::loop_event);
  _window->events.poll_start = make_delegate(*_demo, &demo_base::poll_start);
  _window->events.poll_end   = make_delegate(*_demo, &demo_base::poll_end);
  _window->events.window     = make_delegate(*_demo, &demo_base::event_handler);
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

  //  XR_LOGGER_START(argc, argv);
  //  XR_LOGGER_CONFIG_FILE("config/logging.conf");
  XR_LOG_INFO("Starting up ...");

  tbb::task_scheduler_init tbb_initializer{};

  app_config app_cfg{"config/app_config.conf"};
  xr_app_config = &app_cfg;

  XR_LOG_INFO("Configured paths");
  XR_LOG_INFO("Root {}", xr_app_config->root_directory().c_str());
  XR_LOG_INFO("Shaders {}", xr_app_config->shader_config_path("").c_str());
  XR_LOG_INFO("Models {}", xr_app_config->model_path("").c_str());
  XR_LOG_INFO("Textures {}", xr_app_config->texture_path("").c_str());
  XR_LOG_INFO("Fonts {}", xr_app_config->font_path("").c_str());

  const window_params_t wnd_params{"OpenGL Demo", 4, 5, 24, 8, 32, 0, 1, false};

  window main_window{wnd_params};
  if (!main_window) {
    XR_LOG_ERR("Failed to initialize application window!");
    return EXIT_FAILURE;
  }

  gl::DebugMessageCallback(app::debug_proc, nullptr);
  //
  // turn off these so we don't get spammed
  gl::DebugMessageControl(gl::DONT_CARE,
                          gl::DONT_CARE,
                          gl::DEBUG_SEVERITY_NOTIFICATION,
                          0,
                          nullptr,
                          gl::FALSE_);

  app::main_app app{&main_window};
  main_window.message_loop();

  XR_LOG_INFO("Shutting down ...");
  return EXIT_SUCCESS;
}
