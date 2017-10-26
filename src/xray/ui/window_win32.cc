#include "opengl/opengl.hpp"
#include "opengl/wglext.h"

#include "xray/base/array_dimension.hpp"
#include "xray/base/containers/fixed_vector.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/window_win32.hpp"
#include <windowsx.h>

using namespace xray::base;
using namespace xray::ui;
using namespace xray::ui::detail;
using namespace std;

static constexpr auto kWindowClassName = "__@@##!!__WindowsOpenGL__!!##@@__";

static constexpr const xray::ui::key_sym::e WIN32_KEYS_MAPPING_TABLE[] = {
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::backspace,    key_sym::e::tab,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::clear,        key_sym::e::enter,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::pause,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::escape,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::space,        key_sym::e::page_up,
  key_sym::e::page_down,    key_sym::e::end,
  key_sym::e::home,         key_sym::e::left,
  key_sym::e::up,           key_sym::e::right,
  key_sym::e::down,         key_sym::e::select,
  key_sym::e::print_screen, key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::insert,
  key_sym::e::del,          key_sym::e::unknown,
  key_sym::e::key_0,        key_sym::e::key_1,
  key_sym::e::key_2,        key_sym::e::key_3,
  key_sym::e::key_4,        key_sym::e::key_5,
  key_sym::e::key_6,        key_sym::e::key_7,
  key_sym::e::key_8,        key_sym::e::key_9,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::key_a,
  key_sym::e::key_b,        key_sym::e::key_c,
  key_sym::e::key_d,        key_sym::e::key_e,
  key_sym::e::key_f,        key_sym::e::key_g,
  key_sym::e::key_h,        key_sym::e::key_i,
  key_sym::e::key_j,        key_sym::e::key_k,
  key_sym::e::key_l,        key_sym::e::key_m,
  key_sym::e::key_n,        key_sym::e::key_o,
  key_sym::e::key_p,        key_sym::e::key_q,
  key_sym::e::key_r,        key_sym::e::key_s,
  key_sym::e::key_t,        key_sym::e::key_u,
  key_sym::e::key_v,        key_sym::e::key_w,
  key_sym::e::key_x,        key_sym::e::key_y,
  key_sym::e::key_z,        key_sym::e::left_win,
  key_sym::e::right_win,    key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::kp0,          key_sym::e::kp1,
  key_sym::e::kp2,          key_sym::e::kp3,
  key_sym::e::kp4,          key_sym::e::kp5,
  key_sym::e::kp6,          key_sym::e::kp7,
  key_sym::e::kp8,          key_sym::e::kp9,
  key_sym::e::kp_multiply,  key_sym::e::kp_add,
  key_sym::e::unknown,      key_sym::e::kp_minus,
  key_sym::e::unknown,      key_sym::e::kp_divide,
  key_sym::e::f1,           key_sym::e::f2,
  key_sym::e::f3,           key_sym::e::f4,
  key_sym::e::f5,           key_sym::e::f6,
  key_sym::e::f7,           key_sym::e::f8,
  key_sym::e::f9,           key_sym::e::f10,
  key_sym::e::f11,          key_sym::e::f12,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::scrol_lock,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::left_shift,   key_sym::e::right_shift,
  key_sym::e::left_control, key_sym::e::right_control,
  key_sym::e::left_menu,    key_sym::e::right_menu,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
  key_sym::e::unknown,      key_sym::e::unknown,
};

static key_sym::e map_key(const int32_t key_code) {
  if (key_code == VK_MENU) {
    return key_sym::e::unknown;
  }

  if (key_code < XR_I32_COUNTOF(WIN32_KEYS_MAPPING_TABLE)) {
    return WIN32_KEYS_MAPPING_TABLE[key_code];
  }

  XR_LOG_INFO("Unmaped key code {}", key_code);
  return key_sym::e::unknown;
}

static POINT get_primary_monitor_resolution() {
  auto primaryMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
  if (!primaryMonitor)
    return {1024, 1024};

  MONITORINFO monInfo;
  monInfo.cbSize = sizeof(monInfo);
  if (!GetMonitorInfo(primaryMonitor, &monInfo)) {
    return {1024, 1024};
  }

  return {std::abs(monInfo.rcWork.right - monInfo.rcWork.left),
          std::abs(monInfo.rcWork.bottom - monInfo.rcWork.top)};
}

///
/// Pointers to WGL extensions
struct wgl {
  static PFNWGLCHOOSEPIXELFORMATARBPROC    ChoosePixelFormatARB;
  static PFNWGLCREATECONTEXTATTRIBSARBPROC CreateContextAttribsARB;
  static PFNWGLSWAPINTERVALEXTPROC         SwapIntervalEXT;
};

PFNWGLCHOOSEPIXELFORMATARBPROC    wgl::ChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wgl::CreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC         wgl::SwapIntervalEXT;

xray::ui::window::window(const window_params_t& wparam) { initialize(&wparam); }

xray::ui::window::~window() {}

void xray::ui::window::initialize(const window_params_t* wp) {
  if (_pixelformat_id == 0) {
    constexpr const char* const kTempWindowClassName =
      "__##@@_TempOpenGLWindow";

    const WNDCLASSEX wndClass = {
      sizeof(wndClass),
      CS_OWNDC,
      DefWindowProc,
      0,
      0,
      GetModuleHandle(nullptr),
      LoadIcon(nullptr, IDI_APPLICATION),
      LoadCursor(nullptr, IDC_ARROW),
      (HBRUSH) GetStockObject(COLOR_WINDOW + 1),
      nullptr,
      kTempWindowClassName,
      nullptr,
    };

    //
    // First call, so just create a dummy window and a dummy context to
    // get a suitable pixel format
    RegisterClassEx(&wndClass);
    HWND tempWindow = CreateWindowEx(0L,
                                     kTempWindowClassName,
                                     "TempOpenGL Window",
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     nullptr,
                                     nullptr,
                                     GetModuleHandle(nullptr),
                                     this);

    if (!tempWindow) {
      XR_LOG_CRITICAL("Failed to create temporary window!");
      return;
    }

    {
      scoped_window_dc devContext{GetDC(tempWindow), hdc_deleter{tempWindow}};

      const auto pfd = [wp]() {
        PIXELFORMATDESCRIPTOR pfd = {0};
        pfd.nSize                 = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion              = 1;
        pfd.dwFlags =
          PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType   = PFD_TYPE_RGBA;
        pfd.cColorBits   = (BYTE) wp->color_bits;
        pfd.cDepthBits   = (BYTE) wp->depth_bits;
        pfd.cStencilBits = (BYTE) wp->stencil_bits;
        pfd.iLayerType   = PFD_MAIN_PLANE;

        return pfd;
      }();

      const auto pixelFormat = ChoosePixelFormat(raw_ptr(devContext), &pfd);
      if (pixelFormat == 0) {
        XR_LOG_CRITICAL("Failed to find suitable pixel format!");
        return;
      }

      if (!SetPixelFormat(raw_ptr(devContext), pixelFormat, &pfd)) {
        XR_LOG_CRITICAL("Failed to set new pixel format for temporary window!");
        return;
      }

      scoped_opengl_context tempGLContext{
        wglCreateContext(raw_ptr(devContext))};

      if (!tempGLContext) {
        XR_LOG_CRITICAL("Failed to create initial OpenGL context!");
        return;
      }

      wglMakeCurrent(raw_ptr(devContext), raw_ptr(tempGLContext));

      wgl::ChoosePixelFormatARB =
        (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress(
          "wglChoosePixelFormatARB");

      if (!wgl::ChoosePixelFormatARB) {
        XR_LOG_CRITICAL("wglChoosePixelFormatARB not present!");
        return;
      }

      const int32_t desiredPixelFmtProperties[] = {WGL_DRAW_TO_WINDOW_ARB,
                                                   TRUE,
                                                   /* Full acceleration */
                                                   WGL_ACCELERATION_ARB,
                                                   WGL_FULL_ACCELERATION_ARB,
                                                   /* OpenGL support */
                                                   WGL_SUPPORT_OPENGL_ARB,
                                                   TRUE,
                                                   /* Double buffered */
                                                   WGL_DOUBLE_BUFFER_ARB,
                                                   TRUE,
                                                   WGL_PIXEL_TYPE_ARB,
                                                   WGL_TYPE_RGBA_ARB,
                                                   WGL_COLOR_BITS_ARB,
                                                   wp->color_bits,
                                                   WGL_RED_BITS_ARB,
                                                   8,
                                                   WGL_GREEN_BITS_ARB,
                                                   8,
                                                   WGL_BLUE_BITS_ARB,
                                                   8,
                                                   WGL_ALPHA_BITS_ARB,
                                                   8,
                                                   WGL_DEPTH_BITS_ARB,
                                                   wp->depth_bits,
                                                   WGL_STENCIL_BITS_ARB,
                                                   wp->stencil_bits,
                                                   0};

      uint32_t numFormats{};
      wgl::ChoosePixelFormatARB(raw_ptr(devContext),
                                desiredPixelFmtProperties,
                                nullptr,
                                1,
                                &_pixelformat_id,
                                &numFormats);

      if (numFormats == 0) {
        XR_LOG_CRITICAL("wglChoosePixelFormat found no suitable pixel format!");
        return;
      }
    }

    DestroyWindow(tempWindow);
    //
    //  Found a suitable pixel format, call again
    initialize(wp);
    return;
  }

  assert(_pixelformat_id != 0);
  //
  // Since we have found a suitable pixel format, we will now create
  // the real window, where we will draw stuff.

  const WNDCLASSEX wndClass = {
    sizeof(wndClass),
    CS_OWNDC,
    &window::window_proc_stub,
    0,
    0,
    GetModuleHandle(nullptr),
    LoadIcon(nullptr, IDI_APPLICATION),
    LoadCursor(nullptr, IDC_ARROW),
    (HBRUSH) GetStockObject(COLOR_WINDOW + 1),
    nullptr,
    kWindowClassName,
    nullptr,
  };

  if (!RegisterClassEx(&wndClass)) {
    XR_LOG_CRITICAL("Failed to register real window class!");
    return;
  }

  constexpr auto kWindowStyle = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
  const auto     monSize      = get_primary_monitor_resolution();
  RECT           windowGeom{0, 0, monSize.x, monSize.y};
  AdjustWindowRectEx(&windowGeom, kWindowStyle, false, 0L);

  _window = CreateWindowEx(0,
                           kWindowClassName,
                           wp->title,
                           kWindowStyle,
                           windowGeom.left,
                           windowGeom.top,
                           windowGeom.right - windowGeom.left,
                           windowGeom.bottom - windowGeom.top,
                           nullptr,
                           nullptr,
                           GetModuleHandle(nullptr),
                           this);
  if (!_window) {
    XR_LOG_CRITICAL("Failed to recreate window !");
    return;
  }

  _window_dc = scoped_window_dc{GetDC(_window), hdc_deleter{_window}};

  PIXELFORMATDESCRIPTOR selectedPixelFormat;
  if (!DescribePixelFormat(raw_ptr(_window_dc),
                           _pixelformat_id,
                           (UINT) sizeof(selectedPixelFormat),
                           &selectedPixelFormat)) {
    XR_LOG_CRITICAL("Failed to get description of pixel format {}",
                    _pixelformat_id);
    return;
  }

  //
  //  set the new pixel format
  if (!SetPixelFormat(
        raw_ptr(_window_dc), _pixelformat_id, &selectedPixelFormat)) {
    XR_LOG_CRITICAL("Failed to set chosen pixel format!");
    return;
  }

  //
  //  Create another temp context
  {
    scoped_opengl_context tempGLContext{wglCreateContext(raw_ptr(_window_dc))};
    if (!tempGLContext) {
      XR_LOG_CRITICAL("Failed to create temp OpenGL context (2)");
      return;
    }

    wglMakeCurrent(raw_ptr(_window_dc), raw_ptr(tempGLContext));

    //
    // load wglCreateContextAttribsARB
    wgl::CreateContextAttribsARB =
      (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress(
        "wglCreateContextAttribsARB");

    if (!wgl::CreateContextAttribsARB) {
      XR_LOG_CRITICAL("wglCreateContextAttribsARB not found!");
      return;
    }

    fixed_vector<int32_t, 16u> contextAttribs;

    contextAttribs.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
    contextAttribs.push_back(wp->ver_major);

    contextAttribs.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
    contextAttribs.push_back(wp->ver_minor);

    contextAttribs.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
    contextAttribs.push_back(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);

    if (wp->debug_output_level != 0) {
      contextAttribs.push_back(WGL_CONTEXT_FLAGS_ARB);
      contextAttribs.push_back(WGL_CONTEXT_DEBUG_BIT_ARB);
    }

    if (wp->sample_count != 0) {
    }

    //
    // terminate attribute list
    contextAttribs.push_back(0);

    _glcontext = scoped_opengl_context{
      wgl::CreateContextAttribsARB(
        raw_ptr(_window_dc), nullptr, contextAttribs.data()),
      hglrc_deleter{raw_ptr(_window_dc)}};

    if (!_glcontext) {
      XR_LOG_CRITICAL("Failed to create REAL OPENGL context!");
      return;
    }
  }

  //
  //  Make new context current
  if (!wglMakeCurrent(raw_ptr(_window_dc), raw_ptr(_glcontext))) {
    XR_LOG_CRITICAL("Failed to set context as current OpenGL context !");
    return;
  }

  //
  // Load OpenGL functions
  if (!gl::sys::LoadFunctions()) {
    XR_LOG_CRITICAL("Failed to load OpenGL functions");
    _glcontext = {};
    return;
  }

  ShowWindow(_window, SW_SHOWNORMAL);
  UpdateWindow(_window);

  {
    RECT rc;
    GetClientRect(_window, &rc);
    _wnd_width  = rc.right - rc.left;
    _wnd_height = rc.bottom - rc.top;
  }
}

LRESULT WINAPI xray::ui::window::window_proc_stub(HWND   wnd,
                                                  UINT   msg,
                                                  WPARAM wparam,
                                                  LPARAM lparam) {

  if (msg == WM_CREATE) {
    const auto ptr = (const CREATESTRUCT*) lparam;
    SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR) ptr->lpCreateParams);
    return true;
  }

  auto obj = (window*) GetWindowLongPtr(wnd, GWLP_USERDATA);
  return obj ? obj->window_proc(msg, wparam, lparam)
             : DefWindowProc(wnd, msg, wparam, lparam);
}

LRESULT
xray::ui::window::window_proc(UINT message, WPARAM wparam, LPARAM lparam) {
  maybe<LRESULT> msg_result{0L};

  switch (message) {
  case WM_CLOSE:
    DestroyWindow(_window);
    break;

  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
    event_mouse_button(message, wparam, lparam);
    break;

  case WM_KEYDOWN:
  case WM_KEYUP:
    event_key(message, wparam, lparam);
    break;

  case WM_MOUSEWHEEL:
    event_mouse_wheel(wparam, lparam);
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_MOUSEMOVE:
    event_motion_notify(wparam, lparam);
    break;

  case WM_SIZE:
    event_configure(wparam, lparam);
    break;

  default:
    msg_result = maybe<LRESULT>{nothing{}};
    break;
  }

  if (msg_result) {
    return msg_result.value();
  }

  return DefWindowProc(_window, message, wparam, lparam);
}

void xray::ui::window::disable_cursor() noexcept {}
void xray::ui::window::enable_cursor() noexcept {}

void xray::ui::window::message_loop() {
  pod_zero<MSG> wnd_msg;

  for (; _quit_flag == false;) {

    events.poll_start({});

    while (PeekMessage(&wnd_msg, nullptr, 0, 0, PM_NOREMOVE) && !_quit_flag) {
      const auto res = GetMessage(&wnd_msg, nullptr, 0, 0);
      if (res <= 0) {
        _quit_flag = true;
        break;
      }

      TranslateMessage(&wnd_msg);
      DispatchMessage(&wnd_msg);
    }

    events.poll_end({});

    //
    // user loop event
    events.loop({_wnd_width, _wnd_height, this});
    SwapBuffers(raw_ptr(_window_dc));
  }
}

void xray::ui::window::event_mouse_button(const uint32_t type,
                                          const WPARAM   wp,
                                          const LPARAM   lp) {

  mouse_button_event mbe;

  mbe.type = (type == WM_LBUTTONDOWN || type == WM_RBUTTONDOWN)
               ? event_action_type::press
               : event_action_type::release;

  mbe.wnd       = this;
  mbe.pointer_x = GET_X_LPARAM(lp);
  mbe.pointer_y = GET_Y_LPARAM(lp);
  mbe.button    = (type == WM_LBUTTONDOWN || type == WM_LBUTTONUP)
                 ? mouse_button::button1
                 : mouse_button::button3;
  mbe.button1 = (wp & MK_LBUTTON) != 0;
  mbe.button2 = (wp & MK_MBUTTON) != 0;
  mbe.button3 = (wp & MK_RBUTTON) != 0;
  mbe.button4 = (wp & MK_XBUTTON1) != 0;
  mbe.button5 = (wp & MK_XBUTTON2) != 0;
  mbe.shift   = (wp & MK_SHIFT) != 0;
  mbe.control = (wp & MK_CONTROL) != 0;

  window_event we;
  we.type         = event_type::mouse_button;
  we.event.button = mbe;

  events.window(we);
}

void xray::ui::window::event_mouse_wheel(const WPARAM wparam,
                                         const LPARAM lparam) {
  XR_LOG_INFO("Wheel event!");

  mouse_wheel_event mwe;
  mwe.delta     = GET_WHEEL_DELTA_WPARAM(wparam) < 0 ? +1 : -1;
  mwe.wnd       = this;
  mwe.pointer_x = GET_X_LPARAM(lparam);
  mwe.pointer_y = GET_Y_LPARAM(lparam);
  mwe.button1   = (wparam & MK_LBUTTON) != 0;
  mwe.button2   = (wparam & MK_MBUTTON) != 0;
  mwe.button3   = (wparam & MK_RBUTTON) != 0;
  mwe.button4   = (wparam & MK_XBUTTON1) != 0;
  mwe.button5   = (wparam & MK_XBUTTON2) != 0;
  mwe.shift     = (wparam & MK_SHIFT) != 0;
  mwe.control   = (wparam & MK_CONTROL) != 0;

  window_event we;
  we.type        = event_type::mouse_wheel;
  we.event.wheel = mwe;

  events.window(we);
}

void xray::ui::window::event_key(const uint32_t type,
                                 const WPARAM   wp,
                                 const LPARAM   lp) {
  key_event ke;
  ke.wnd  = this;
  ke.type = (type == WM_KEYDOWN ? event_action_type::press
                                : event_action_type::release);
  memset(ke.name, 0, sizeof(ke.name));

  const auto cursor_pos = [w = _window]() {
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(w, &pt);
    return pt;
  }();

  ke.keycode   = map_key(wp);
  ke.pointer_x = cursor_pos.x;
  ke.pointer_y = cursor_pos.y;
  ke.button1   = (GetAsyncKeyState(VK_LBUTTON) & (1 << 15)) != 0;
  ke.button2   = (GetAsyncKeyState(VK_MBUTTON) & (1 << 15)) != 0;
  ke.button3   = (GetAsyncKeyState(VK_RBUTTON) & (1 << 15)) != 0;
  ke.button4   = (GetAsyncKeyState(VK_XBUTTON1) & (1 << 15)) != 0;
  ke.button5   = (GetAsyncKeyState(VK_XBUTTON2) & (1 << 15)) != 0;
  ke.shift     = (GetAsyncKeyState(VK_SHIFT) & (1 << 15)) != 0;
  ke.control   = (GetAsyncKeyState(VK_CONTROL) & (1 << 15)) != 0;

  GetKeyNameText(lp, ke.name, XR_I32_COUNTOF(ke.name));

  window_event we;
  we.type      = event_type::key;
  we.event.key = ke;

  events.window(we);
}

void xray::ui::window::event_motion_notify(const WPARAM wparam,
                                           const LPARAM lparam) {
  mouse_motion_event mme;
  mme.wnd       = this;
  mme.pointer_x = GET_X_LPARAM(lparam);
  mme.pointer_y = GET_Y_LPARAM(lparam);
  mme.button1   = (wparam & MK_LBUTTON) != 0;
  mme.button2   = (wparam & MK_MBUTTON) != 0;
  mme.button3   = (wparam & MK_RBUTTON) != 0;
  mme.button4   = (wparam & MK_XBUTTON1) != 0;
  mme.button5   = (wparam & MK_XBUTTON2) != 0;
  mme.shift     = (wparam & MK_SHIFT) != 0;
  mme.control   = (wparam & MK_CONTROL) != 0;

  window_event we;
  we.type         = event_type::mouse_motion;
  we.event.motion = mme;

  events.window(we);
}

void xray::ui::window::event_configure(const WPARAM wparam,
                                       const LPARAM lparam) {
  if ((wparam != SIZE_MAXIMIZED) || (wparam != SIZE_RESTORED)) {
    return;
  }

  _wnd_width  = GET_X_LPARAM(lparam);
  _wnd_height = GET_Y_LPARAM(lparam);

  window_configure_event cfg_evt;
  cfg_evt.width  = _wnd_width;
  cfg_evt.height = _wnd_height;
  cfg_evt.wnd    = this;

  window_event we;
  we.type            = event_type::configure;
  we.event.configure = cfg_evt;

  events.window(we);
}
