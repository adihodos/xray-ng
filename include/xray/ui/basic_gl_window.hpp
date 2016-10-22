#pragma once

#include "xray/xray.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/ui/event_delegate_types.hpp"
#include <cstddef>
#include <cstdint>
#include <opengl/opengl.hpp>

#include <GLFW/glfw3.h>

namespace xray {
namespace ui {

/// \brief      Controls whether debug messages are emitted by the
///             OpenGL api.
enum class api_debug_output : uint8_t {
  none            = 0,
  high_severity   = 1 << 0,
  medium_severity = 1 << 1,
  low_severity    = 1 << 2,
  notification    = 1 << 3
};

constexpr api_debug_output operator|(const api_debug_output s0,
                                     const api_debug_output s1) {
  return static_cast<api_debug_output>(static_cast<uint8_t>(s0) |
                                       static_cast<uint8_t>(s1));
}

constexpr bool operator&(const api_debug_output s0, const api_debug_output s1) {
  return (static_cast<uint8_t>(s0) & static_cast<uint8_t>(s1)) != 0;
}

namespace detail {

struct glfw_scoped_initializer {
public:
  glfw_scoped_initializer() noexcept : succeeded_{glfwInit() == GLFW_TRUE} {}

  ~glfw_scoped_initializer() noexcept {
    //
    // must be called even if glfwInit() did not succeed.
    glfwTerminate();
  }

  /// \brief Returns true if the GLFW library was successfully initialized.
  explicit operator bool() const noexcept { return succeeded_; }

private:
  XRAY_NO_COPY(glfw_scoped_initializer);

private:
  bool succeeded_;
};

/// \brief      Simple printer for OpenGL debug events.
class default_opengl_debug_sink {
public:
  default_opengl_debug_sink() {}

  void setup(const api_debug_output debug_options);

  void sink_dbg_event(const int32_t msg_src, const int32_t msg_type,
                      const uint32_t msg_id, const int32_t    msg_sev,
                      const size_t /* msg_len */, const char* msg_txt);
};

} // namespace detail

/// \brief      Controls what information to show about the rendering API,
///             at initialization.
enum class api_info : uint8_t {
  none           = 0x0,
  version        = 0x01,
  extension_list = 0x02
};

constexpr api_info operator|(const api_info opt_a, const api_info opt_b) {
  return static_cast<api_info>(static_cast<uint8_t>(opt_a) |
                               static_cast<uint8_t>(opt_b));
}

constexpr bool operator&(const api_info opt_a, const api_info opt_b) {
  return (static_cast<uint8_t>(opt_a) & static_cast<uint8_t>(opt_b)) != 0;
}

/// \brief      Parameters for the renderer.
struct render_params_t {
  render_params_t() = default;

  render_params_t(const api_debug_output dbg_output, const api_info api_info_,
                  const uint32_t api_major, const uint32_t api_minor)
      : debug_info{dbg_output}
      , show_api_info{api_info_}
      , api_ver_major{api_major}
      , api_ver_minor{api_minor} {}

  api_debug_output debug_info{api_debug_output::none};
  api_info         show_api_info{api_info::none};
  uint32_t         api_ver_major{0};
  uint32_t         api_ver_minor{0};
};

class basic_window {
public:
  struct {
    //    key_delegate keys;
    //    mouse_button_delegate mouse_button;
    //    mouse_scroll_delegate mouse_scroll;
    //    mouse_enter_exit_delegate mouse_enter_exit;
    //    mouse_move_delegate mouse_move;
    window_size_delegate window_resize;
    draw_delegate_type   draw;
    tick_delegate_type   tick;
    debug_delegate_type  debug;
    input_event_delegate input;
  } events;

  /// \name Construction and destruction.
  /// @{
public:
  using handle_type = GLFWwindow*;

  basic_window(const render_params_t& r_params, const char* title = nullptr);

  ~basic_window();

  /// @}

  auto handle() const noexcept { return wnd_handle_; }

  void disable_cursor() noexcept;

  void enable_cursor() noexcept;

  /// \name Sanity checks.
  /// @{
public:
  explicit operator bool() const noexcept { return valid(); }

  bool valid() const noexcept { return handle() != nullptr; }

  /// @}

  uint32_t width() const noexcept;

  uint32_t height() const noexcept;

  void pump_messages();

  void quit() noexcept { glfwSetWindowShouldClose(handle(), GLFW_TRUE); }

private:
  static void key_callback_stub(GLFWwindow* wnd, const int32_t key_code,
                                const int32_t scan_code, const int32_t action,
                                const int32_t mods);

  static void mouse_button_stub(GLFWwindow* wnd, int32_t button, int32_t action,
                                int32_t modifiers);

  static void mouse_scroll_stub(GLFWwindow* wnd, double x_offset,
                                double y_offset);

  static void mouse_move_stub(GLFWwindow* wnd, double x_pos, double y_pos);

  static void mouse_enter_exit_stub(GLFWwindow* window, int32_t entered);

  static void window_resized(GLFWwindow* window, int32_t width, int32_t height);

  void key_pressed_event(const int32_t key_code);

  static void debug_callback_stub(GLenum source, GLenum type, GLuint id,
                                  GLenum severity, GLsizei length,
                                  const GLchar* message, const void* param);

  void debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity,
                      GLsizei length, const GLchar* message);

private:
  ///< Initializer for the GLFW libray. Must be the first constructed
  ///< object, before any OpenGL calls are made.
  detail::glfw_scoped_initializer glfw_init_{};

  ///< Default OpenGL debug events printer.
  detail::default_opengl_debug_sink gl_debug_sink_{};

  ///< Handle to the GLFW window object.
  GLFWwindow* wnd_handle_{nullptr};

  ///< Timer for update events sent to delegates.
  base::timer_stdp timer_;

  uint32_t dbg_options_{static_cast<uint32_t>(api_debug_output::none)};

private:
  XRAY_NO_COPY(basic_window);
};

} // namespace ui
} // namespace xray
