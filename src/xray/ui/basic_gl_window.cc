#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/ui/basic_gl_window.hpp"
#include "xray/ui/key_symbols.hpp"
#include "xray/ui/window_context.hpp"
#include <algorithm>
#include <cassert>
#include <opengl/opengl.hpp>
#include <stlsoft/memory/auto_buffer.hpp>

// void xray::ui::detail::default_opengl_debug_sink::setup(
//     const api_debug_output debug_options) {

//   gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE,
//                           gl::DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
//                           debug_options & api_debug_output::notification);

//   gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE,
//   gl::DEBUG_SEVERITY_LOW,
//                           0, nullptr,
//                           debug_options & api_debug_output::low_severity);

//   gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE,
//                           gl::DEBUG_SEVERITY_MEDIUM, 0, nullptr,
//                           debug_options & api_debug_output::medium_severity);

//   gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE,
//   gl::DEBUG_SEVERITY_HIGH,
//                           0, nullptr,
//                           debug_options & api_debug_output::high_severity);
// }

// const char* dbg_event_get_source(const int32_t src_id) noexcept {
//   switch (src_id) {
//   case gl::DEBUG_SOURCE_API:
//     return "OpenGL API call";
//     break;

//   case gl::DEBUG_SOURCE_WINDOW_SYSTEM:
//     return "Window system API call";
//     break;

//   case gl::DEBUG_SOURCE_THIRD_PARTY:
//     return "Third party OpenGL app";
//     break;

//   case gl::DEBUG_SOURCE_APPLICATION:
//     return "Current application";
//     break;

//   case gl::DEBUG_SOURCE_OTHER:
//   default:
//     return "Other source";
//     break;
//   }
// }

// const char* dbg_event_get_type(const int32_t evt_type) noexcept {
//   switch (evt_type) {
//   case gl::DEBUG_TYPE_ERROR:
//     return "OpenGL API error";
//     break;

//   case gl::DEBUG_TYPE_DEPRECATED_BEHAVIOR:
//     return "Deprecated behaviour";
//     break;

//   case gl::DEBUG_TYPE_UNDEFINED_BEHAVIOR:
//     return "Undefined behaviour";
//     break;

//   case gl::DEBUG_TYPE_PORTABILITY:
//     return "Functionaly is not portable";
//     break;

//   case gl::DEBUG_TYPE_PERFORMANCE:
//     return "Possible performance issues";
//     break;

//   case gl::DEBUG_TYPE_MARKER:
//     return "Annotation";
//     break;

//   case gl::DEBUG_TYPE_PUSH_GROUP:
//   case gl::DEBUG_TYPE_POP_GROUP:
//     return "Push/Pop debug group";
//     break;

//   case gl::DEBUG_TYPE_OTHER:
//   default:
//     return "Other";
//     break;
//   }
// }

// const char* dbg_event_get_severity(const int32_t sev) noexcept {
//   switch (sev) {
//   case gl::DEBUG_SEVERITY_HIGH:
//     return "High severity";
//     break;

//   case gl::DEBUG_SEVERITY_MEDIUM:
//     return "Medium severity";
//     break;

//   case gl::DEBUG_SEVERITY_LOW:
//     return "Low severity";
//     break;

//   case gl::DEBUG_SEVERITY_NOTIFICATION:
//   default:
//     return "Notification";
//     break;
//   }
// }

// void xray::ui::detail::default_opengl_debug_sink::sink_dbg_event(
//     const int32_t msg_src, const int32_t msg_type, const uint32_t msg_id,
//     const int32_t msg_sev, const size_t /* msg_len */, const char* msg_txt) {

//   const auto evt_src  = dbg_event_get_source(msg_src);
//   const auto evt_type = dbg_event_get_type(msg_type);

//   switch (msg_sev) {
//   case gl::DEBUG_SEVERITY_HIGH:
//   case gl::DEBUG_SEVERITY_MEDIUM:
//     XR_LOG_CRITICAL("OpenGL event [{}], id [{}], source [{}], \ninfo : [{}]",
//                     evt_type, msg_id, evt_src, msg_txt);
//     break;

//   case gl::DEBUG_SEVERITY_LOW:
//     XR_LOG_WARN("OpenGL event [{}], id [{}], source [{}], \ninfo : [{}]",
//                 evt_type, msg_id, evt_src, msg_txt);
//     break;

//   case gl::DEBUG_SEVERITY_NOTIFICATION:
//   default:
//     XR_LOG_INFO("OpenGL event [{}], id [{}], source [{}], \ninfo : [{}]",
//                 evt_type, msg_id, evt_src, msg_txt);
//     break;
//   }
// }

// static void dump_gl_info() {

//   struct gl_query_data {
//     const char*  info_string;
//     const GLuint query_id;
//   };

//   constexpr gl_query_data query_data[] = {
//       {"Renderer -> ", gl::RENDERER},
//       {"Vendor -> ", gl::VENDOR},
//       {"Version string -> ", gl::VERSION},
//       {"GLSL version string -> ", gl::SHADING_LANGUAGE_VERSION}};

//   XR_LOG_INFO("OpenGL information :");

//   using namespace std;

//   for_each(begin(query_data), end(query_data), [](const auto& qd) {
//     const auto query_res = gl::GetString(qd.query_id);
//     if (!query_res)
//       return;

//     XR_LOG_INFO("{} {}", qd.info_string, query_res);
//   });

//   {
//     GLint v_major{};
//     GLint v_minor{};
//     gl::GetIntegerv(gl::MAJOR_VERSION, &v_major);
//     gl::GetIntegerv(gl::MINOR_VERSION, &v_minor);

//     XR_LOG_INFO("Version string (integer) -> {}.{}", v_major, v_minor);
//   }
// }

// static void dump_extensions() noexcept {
//   GLint extension_count{};
//   gl::GetIntegerv(gl::NUM_EXTENSIONS, &extension_count);

//   XR_LOG_INFO("Supported extension count : {}", extension_count);
//   XR_LOG_INFO("Supported extension list : ");

//   for (int32_t ext_idx = 0; ext_idx < extension_count; ++ext_idx) {
//     const auto ext_name = gl::GetStringi(gl::EXTENSIONS, ext_idx);
//     if (ext_name) {
//       XR_LOG_INFO("\n{}", ext_name);
//     }
//   }
// }

// xray::ui::basic_window::basic_window(const render_params_t& r_params,
//                                      const char* title /* = nullptr */) {

//   if (!glfw_init_)
//     return;

//   auto primary_monitor = glfwGetPrimaryMonitor();
//   if (!primary_monitor) {
//     OUTPUT_DBG_MSG("Failed to retrieve primary monitor !");
//     return;
//   }

//   const auto vid_mode = glfwGetVideoMode(primary_monitor);

//   glfwWindowHint(GLFW_RED_BITS, vid_mode->redBits);
//   glfwWindowHint(GLFW_GREEN_BITS, vid_mode->greenBits);
//   glfwWindowHint(GLFW_BLUE_BITS, vid_mode->blueBits);
//   glfwWindowHint(GLFW_REFRESH_RATE, vid_mode->refreshRate);
//   glfwWindowHint(GLFW_DEPTH_BITS, 24);
//   glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
//   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, r_params.api_ver_major);
//   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, r_params.api_ver_minor);
//   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
//   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

//   if (r_params.debug_info != api_debug_output::none)
//     glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

//   auto window_handle = glfwCreateWindow(vid_mode->width, vid_mode->height,
//                                         title ? title : "Simple GL window",
//                                         primary_monitor, nullptr);

//   if (!window_handle) {
//     OUTPUT_DBG_MSG("Failed to create window !");
//     return;
//   }

//   glfwMakeContextCurrent(window_handle);
//   glfwSwapInterval(1);

//   glfwSetWindowUserPointer(window_handle, this);
//   glfwSetKeyCallback(window_handle, &basic_window::key_callback_stub);
//   glfwSetCursorEnterCallback(window_handle,
//                              &basic_window::mouse_enter_exit_stub);
//   glfwSetMouseButtonCallback(window_handle,
//   &basic_window::mouse_button_stub);
//   glfwSetScrollCallback(window_handle, &basic_window::mouse_scroll_stub);
//   glfwSetCursorPosCallback(window_handle, &basic_window::mouse_move_stub);
//   glfwSetFramebufferSizeCallback(window_handle,
//   &basic_window::window_resized);

//   //
//   //  Load OpenGL functions.
//   {
//     const auto load_result = gl::sys::LoadFunctions();
//     if (!load_result) {
//       OUTPUT_DBG_MSG("Failed to load OpenGL :(");
//       return;
//     }
//   }

//   //
//   // Hookup debug
//   {
//     gl_debug_sink_.setup(r_params.debug_info);
//     if (r_params.debug_info != api_debug_output::none)
//       gl::DebugMessageCallback(&basic_window::debug_callback_stub, this);

//     events.debug = base::make_delegate(
//         gl_debug_sink_, &detail::default_opengl_debug_sink::sink_dbg_event);
//   }

//   {
//     wnd_handle_ = window_handle;

//     if (r_params.show_api_info & api_info::version)
//       dump_gl_info();

//     if (r_params.show_api_info & api_info::extension_list)
//       dump_extensions();

//     timer_.start();
//   }
// }

// xray::ui::basic_window::~basic_window() {
//   if (handle())
//     glfwDestroyWindow(handle());
// }

// void xray::ui::basic_window::disable_cursor() noexcept {
//   assert(valid());
//   glfwSetInputMode(handle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
// }

// void xray::ui::basic_window::enable_cursor() noexcept {
//   assert(valid());
//   glfwSetInputMode(handle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
// }

// void xray::ui::basic_window::key_callback_stub(GLFWwindow*   wnd,
//                                                const int32_t key_code,
//                                                const int32_t /* scan_code */,
//                                                const int32_t action,
//                                                const int32_t mods) {
//   XR_UNUSED_ARG(mods);

//   auto obj_ptr = static_cast<basic_window*>(glfwGetWindowUserPointer(wnd));
//   if (!obj_ptr)
//     return;

//   if (obj_ptr->events.input) {
//     input_event_t key_event;
//     key_event.type            = input_event_type::key;
//     key_event.key_ev.down     = (action == GLFW_PRESS);
//     key_event.key_ev.key_code = os_key_scan_to_app_key(key_code);
//     obj_ptr->events.input(key_event);
//   }
// }

// uint32_t xray::ui::basic_window::width() const noexcept {
//   assert(valid());

//   int32_t win_width{};
//   int32_t win_height{};
//   glfwGetFramebufferSize(handle(), &win_width, &win_height);

//   return static_cast<uint32_t>(win_width);
// }

// uint32_t xray::ui::basic_window::height() const noexcept {
//   assert(valid());

//   int32_t win_width{};
//   int32_t win_height{};
//   glfwGetFramebufferSize(handle(), &win_width, &win_height);

//   return static_cast<uint32_t>(win_height);
// }

// void xray::ui::basic_window::pump_messages() {

//   while (!glfwWindowShouldClose(handle())) {
//     //
//     // update
//     timer_.update_and_reset();
//     const auto delta_time = timer_.elapsed_millis();

//     if (events.tick)
//       events.tick(delta_time);

//     if (events.draw) {
//       const window_context win_ctx{width(), height(), handle()};
//       events.draw(win_ctx);
//     }

//     glfwSwapBuffers(handle());
//     glfwPollEvents();
//   }
// }

// void xray::ui::basic_window::debug_callback_stub(GLenum source, GLenum type,
//                                                  GLuint id, GLenum severity,
//                                                  GLsizei       length,
//                                                  const GLchar* message,
//                                                  const void*   param) {
//   auto obj_ptr =
//       const_cast<basic_window*>(static_cast<const basic_window*>(param));

//   if (obj_ptr && obj_ptr->events.debug)
//     obj_ptr->events.debug(source, type, id, severity, length, message);
// }

// void xray::ui::basic_window::mouse_button_stub(GLFWwindow* wnd, int32_t
// button,
//                                                int32_t action,
//                                                int32_t modifiers) {
//   XR_UNUSED_ARG(modifiers);
//   auto wnd_ptr = static_cast<basic_window*>(glfwGetWindowUserPointer(wnd));
//   assert(wnd_ptr != nullptr);

//   if (wnd_ptr->events.input) {
//     const auto btn_code = [button]() {
//       switch (button) {
//       case GLFW_MOUSE_BUTTON_LEFT:
//         return mouse_button::left;
//         break;

//       case GLFW_MOUSE_BUTTON_RIGHT:
//         return mouse_button::right;
//         break;

//       default:
//         return mouse_button::xbutton1;
//         break;
//       }
//     }();

//     double cx{};
//     double cy{};
//     glfwGetCursorPos(wnd, &cx, &cy);

//     input_event_t mouse_evt;
//     mouse_evt.type                   = input_event_type::mouse_button;
//     mouse_evt.mouse_button_ev.btn_id = btn_code;
//     mouse_evt.mouse_button_ev.down   = (action == GLFW_PRESS);
//     mouse_evt.mouse_button_ev.x_pos  = static_cast<int32_t>(cx);
//     mouse_evt.mouse_button_ev.y_pos  = static_cast<int32_t>(cy);

//     wnd_ptr->events.input(mouse_evt);
//   }
// }

// void xray::ui::basic_window::mouse_scroll_stub(GLFWwindow* wnd, double
// x_offset,
//                                                double y_offset) {
//   (void) x_offset;
//   auto wnd_ptr = static_cast<basic_window*>(glfwGetWindowUserPointer(wnd));
//   assert(wnd_ptr != nullptr);

//   if (wnd_ptr->events.input) {
//     double cx{};
//     double cy{};
//     glfwGetCursorPos(wnd, &cx, &cy);

//     input_event_t ev;
//     ev.type                 = input_event_type::mouse_wheel;
//     ev.mouse_wheel_ev.delta = static_cast<int32_t>(y_offset);
//     ev.mouse_wheel_ev.x_pos = static_cast<int32_t>(cx);
//     ev.mouse_wheel_ev.y_pos = static_cast<int32_t>(cy);

//     wnd_ptr->events.input(ev);
//   }
// }

// void xray::ui::basic_window::mouse_enter_exit_stub(GLFWwindow* wnd,
//                                                    int32_t     entered) {
//   auto wnd_ptr = static_cast<basic_window*>(glfwGetWindowUserPointer(wnd));
//   assert(wnd_ptr != nullptr);

//   if (wnd_ptr->events.input) {
//     input_event_t ev;
//     ev.type                      = input_event_type::mouse_enter_exit;
//     ev.mouse_enter_leave.entered = (entered == GLFW_TRUE);
//     wnd_ptr->events.input(ev);
//   }
// }

// void xray::ui::basic_window::mouse_move_stub(GLFWwindow* wnd, double x_pos,
//                                              double y_pos) {
//   auto wnd_ptr = static_cast<basic_window*>(glfwGetWindowUserPointer(wnd));
//   assert(wnd_ptr != nullptr);

//   if (wnd_ptr->events.input) {
//     input_event_t ev;
//     ev.type                      = input_event_type::mouse_movement;
//     ev.mouse_move_ev.x_pos       = static_cast<int32_t>(x_pos);
//     ev.mouse_move_ev.y_pos       = static_cast<int32_t>(y_pos);
//     ev.mouse_move_ev.params.ctrl = static_cast<uint32_t>(
//         glfwGetKey(wnd_ptr->handle(), GLFW_KEY_LEFT_CONTROL));
//     ev.mouse_move_ev.params.shift = static_cast<uint32_t>(
//         glfwGetKey(wnd_ptr->handle(), GLFW_KEY_LEFT_SHIFT));
//     ev.mouse_move_ev.params.lbutton = static_cast<uint32_t>(
//         glfwGetMouseButton(wnd_ptr->handle(), GLFW_MOUSE_BUTTON_LEFT));
//     ev.mouse_move_ev.params.rbutton = static_cast<uint32_t>(
//         glfwGetMouseButton(wnd_ptr->handle(), GLFW_MOUSE_BUTTON_RIGHT));

//     wnd_ptr->events.input(ev);
//   }
// }

// void xray::ui::basic_window::window_resized(GLFWwindow* window, int32_t
// width,
//                                             int32_t height) {
//   auto wnd_ptr =
//   static_cast<basic_window*>(glfwGetWindowUserPointer(window));
//   assert(wnd_ptr != nullptr);

//   if (wnd_ptr->events.window_resize)
//     wnd_ptr->events.window_resize(width, height);
// }
