#include "xray/ui/app_window.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/ui/input_event.hpp"
#include "xray/ui/key_symbols.hpp"
#include <algorithm>
#include <dwmapi.h>
#include <dxgi.h>
#include <memory>
#include <stlsoft/memory/auto_buffer.hpp>
#include <string>
#include <tchar.h>
#include <tuple>
#include <windowsx.h>

using namespace xray::base;

static const TCHAR* const APP_WINDOW_CLASS_NAME =
    _T("__##@@%%axaxaxa_!!!_Win32XrayWndClass_##@@__");

static constexpr auto swp_flags =
    SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED;

/// \brief  Returns a handle to the primary monitor.
/// \see    http://blogs.msdn.com/b/oldnewthing/archive/2007/08/09/4300545.aspx
static HMONITOR get_primary_monitor() {
  const auto point_zero = POINT{0, 0};
  return MonitorFromPoint(point_zero, MONITOR_DEFAULTTOPRIMARY);
}

xray::ui::application_window::application_window(
    const window_params& init_params) {

  _wininfo.instance             = GetModuleHandle(nullptr);
  _wininfo.width                = init_params.width;
  _wininfo.height               = init_params.height;
  _wininfo.should_be_fullscreen = init_params.fullscreen;

  //
  //  Register window class.
  {
    WNDCLASSEX window_class;
    memset(&window_class, 0, sizeof(window_class));

    window_class.cbSize        = sizeof(window_class);
    window_class.style         = 0;
    window_class.lpfnWndProc   = &application_window::wnd_proc_stub;
    window_class.hInstance     = _wininfo.instance;
    window_class.hbrBackground = nullptr;
    window_class.lpszClassName = APP_WINDOW_CLASS_NAME;
    window_class.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor       = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassEx(&window_class)) {
      XR_LOG_CRITICAL("Failed to register window class !");
      return;
    }
  }

  {
    DWORD   win_style = 0;
    int32_t org_x{};
    int32_t org_y{};
    int32_t width{};
    int32_t height{};

    win_style |= WS_OVERLAPPEDWINDOW;

    RECT wnd_rect;
    wnd_rect.left   = 0;
    wnd_rect.right  = static_cast<int32_t>(init_params.width);
    wnd_rect.top    = 0;
    wnd_rect.bottom = static_cast<int32_t>(init_params.height);

    AdjustWindowRect(&wnd_rect, win_style, false);

    //
    //  Always use the primary monitor to start up.
    const auto  primary_monitor = get_primary_monitor();
    MONITORINFO mon_info;
    mon_info.cbSize = sizeof(mon_info);
    GetMonitorInfo(primary_monitor, &mon_info);

    //
    //  Center window on monitor.
    width = std::min(wnd_rect.right - wnd_rect.left,
                     mon_info.rcWork.right - mon_info.rcWork.left);
    height = std::min(wnd_rect.bottom - wnd_rect.top,
                      mon_info.rcWork.right - mon_info.rcWork.left);

    org_x = (mon_info.rcWork.left + mon_info.rcWork.right - width) / 2;
    org_y = (mon_info.rcWork.top + mon_info.rcWork.bottom - height) / 2;

    _wininfo.handle = CreateWindowEx(
        0L, APP_WINDOW_CLASS_NAME, "Direct3D Demo", win_style, org_x, org_y,
        width, height, HWND_DESKTOP, nullptr, _wininfo.instance, this);

    if (!_wininfo.handle) {
      XR_LOG_ERR("Failed to create window !");
      return;
    }

    ShowWindow(_wininfo.handle, SW_SHOWNORMAL);
    UpdateWindow(_wininfo.handle);
  }
}

xray::ui::application_window::~application_window() {}

LRESULT WINAPI xray::ui::application_window::wnd_proc_stub(HWND wnd, UINT msg,
                                                           WPARAM w_param,
                                                           LPARAM l_param) {
  if (msg == WM_CREATE) {
    auto create_data = reinterpret_cast<CREATESTRUCT*>(l_param);
    auto wnd_object =
        reinterpret_cast<application_window*>(create_data->lpCreateParams);

    SetWindowLongPtrW(wnd, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(wnd_object));
    return true;
  }

  application_window* wptr = reinterpret_cast<application_window*>(
      static_cast<LONG_PTR>(GetWindowLongPtrW(wnd, GWLP_USERDATA)));

  if (wptr)
    return wptr->window_procedure(msg, w_param, l_param);

  return DefWindowProc(wnd, msg, w_param, l_param);
}

void xray::ui::application_window::wm_size_event(
    const int32_t size_request, const int32_t width,
    const int32_t height) noexcept {

  assert(width >= 0);
  assert(height >= 0);

  _wininfo.width  = static_cast<uint32_t>(width);
  _wininfo.height = static_cast<uint32_t>(height);

  switch (size_request) {
  case SIZE_MINIMIZED:
    OUTPUT_DBG_MSG("Minimized!");
    _wininfo.active = false;
    return;
    break;

  case SIZE_MAXIMIZED:
  case SIZE_RESTORED:
    OUTPUT_DBG_MSG("Maximized/restored");
    _wininfo.active = true;
    break;

  default:
    OUTPUT_DBG_MSG("Resized!");
    break;
  }

  if (events.resize && !_wininfo.resizing)
    events.resize(_wininfo.width, _wininfo.height);
}

void xray::ui::application_window::run() noexcept {
  if (!is_valid()) {
    return;
  }

  if (_wininfo.should_be_fullscreen)
    toggle_fullscreen(true);

  events.resize(width(), height());
  _timer.start();

  pod_zero<MSG> message;

  while (message.message != WM_QUIT && !_wininfo.quitflag) {

    while (PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE) == FALSE) {
      make_frame();
    }

    do {
      const int32_t ret_code = GetMessage(&message, nullptr, 0, 0);
      if (ret_code <= 0) {
        //
        // -1 -> GetMessage() error, 0 -> WM_QUIT received
        _wininfo.quitflag = true;
        break;
      }

      TranslateMessage(&message);
      DispatchMessage(&message);

    } while (PeekMessage(&message, nullptr, 0, 0, PM_NOREMOVE) == TRUE);
  }
}

void xray::ui::application_window::make_frame() {

  if (_wininfo.active) {
    _timer.update_and_reset();
    const float delta_ms = _timer.elapsed_millis();
    events.tick(delta_ms);

    const window_draw_event draw_event{handle(), width(), height()};
    events.draw(draw_event);
    events.swap_buffers(false);
  } else {
    // OUTPUT_DBG_MSG("Inactive -> sleeping");
    Sleep(100);
  }
}

void xray::ui::application_window::toggle_fullscreen(
    const bool fs_value) noexcept {
  OUTPUT_DBG_MSG("Fullscreen toggle from %s",
                 _wininfo.fullscreen ? "fullscreen" : "windowed");

  if (!_wininfo.fullscreen) {
    _saved_wnd_info.maximized = !!IsZoomed(_wininfo.handle);
    if (_saved_wnd_info.maximized)
      SendMessage(_wininfo.handle, WM_SYSCOMMAND, SC_RESTORE, 0);

    _saved_wnd_info.style    = GetWindowLongPtr(_wininfo.handle, GWL_STYLE);
    _saved_wnd_info.ex_style = GetWindowLongPtr(_wininfo.handle, GWL_EXSTYLE);
    GetWindowRect(_wininfo.handle, &_saved_wnd_info.wnd_rect);
  }

  _wininfo.fullscreen = fs_value;

  if (_wininfo.fullscreen) {
    SetWindowLongPtr(_wininfo.handle, GWL_STYLE,
                     _saved_wnd_info.style & ~(WS_CAPTION | WS_THICKFRAME));
    SetWindowLongPtr(_wininfo.handle, GWL_EXSTYLE,
                     _saved_wnd_info.ex_style &
                         ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE |
                           WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

    MONITORINFO monitor_info;
    monitor_info.cbSize = sizeof(monitor_info);
    GetMonitorInfo(MonitorFromWindow(_wininfo.handle, MONITOR_DEFAULTTOPRIMARY),
                   &monitor_info);
    const auto* rect = &monitor_info.rcMonitor;

    SetWindowPos(_wininfo.handle, nullptr, rect->left, rect->top,
                 rect->right - rect->left, rect->bottom - rect->top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  } else {
    SetWindowLongPtr(_wininfo.handle, GWL_STYLE, _saved_wnd_info.style);
    SetWindowLongPtr(_wininfo.handle, GWL_EXSTYLE, _saved_wnd_info.ex_style);

    const auto rc = &_saved_wnd_info.wnd_rect;

    SetWindowPos(_wininfo.handle, nullptr, rc->left, rc->top,
                 rc->right - rc->left, rc->bottom - rc->top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    if (_saved_wnd_info.maximized)
      SendMessage(_wininfo.handle, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
  }

  if (events.toggle_fullscreen)
    events.toggle_fullscreen();
}

void xray::ui::application_window::key_event(
    const int32_t key_code, const bool down /*= true*/) noexcept {

  if (!events.input)
    return;

  input_event_t key_down_event;
  key_down_event.type            = ui::input_event_type::key;
  key_down_event.key_ev.key_code = os_key_scan_to_app_key(key_code);
  key_down_event.key_ev.down     = down;

  events.input(key_down_event);
}

void xray::ui::application_window::syskeydown_event(
    const WPARAM w_param, const LPARAM l_param) noexcept {

  if ((l_param & 1 << 29) && w_param == VK_RETURN) {
    toggle_fullscreen(!_wininfo.fullscreen);
    return;
  }

  DefWindowProc(_wininfo.handle, WM_SYSKEYDOWN, w_param, l_param);
}

// void xray::ui::application_window::handle_wm_syscommand(
//    WPARAM w_param, LPARAM l_param) noexcept {
//
//  if ((w_param == SC_MAXIMIZE) && (!winstats_.fullscreen)) {
//    toggle_fullscreen();
//    return;
//  }
//
//  DefWindowProc(winstats_.win, WM_SYSCOMMAND, w_param, l_param);
//}

void xray::ui::application_window::mouse_button_event(
    WPARAM /*w_param*/, LPARAM l_param, const mouse_button button,
    const bool down /*= true */) noexcept {
  assert(is_valid());

  if (!events.input)
    return;

  input_event_t mouse_evt;
  mouse_evt.type                   = ui::input_event_type::mouse_button;
  mouse_evt.mouse_button_ev.btn_id = button;
  mouse_evt.mouse_button_ev.down   = down;
  mouse_evt.mouse_button_ev.x_pos  = GET_X_LPARAM(l_param);
  mouse_evt.mouse_button_ev.y_pos  = GET_Y_LPARAM(l_param);

  events.input(mouse_evt);
}

void xray::ui::application_window::mouse_move_event(WPARAM /*w_param*/,
                                                    LPARAM l_param) noexcept {

  assert(is_valid());

  if (/*!winstats_.mousecaptured ||*/ !events.input)
    return;

  input_event_t move_evt;
  move_evt.type                = ui::input_event_type::mouse_movement;
  move_evt.mouse_move_ev.x_pos = GET_X_LPARAM(l_param);
  move_evt.mouse_move_ev.y_pos = GET_Y_LPARAM(l_param);

  events.input(move_evt);
}

void xray::ui::application_window::mouse_wheel_event(WPARAM w_param,
                                                     LPARAM l_param) noexcept {
  assert(is_valid());

  if (!events.input)
    return;

  input_event_t wheel_evt;
  wheel_evt.type                 = ui::input_event_type::mouse_wheel;
  wheel_evt.mouse_wheel_ev.delta = GET_WHEEL_DELTA_WPARAM(w_param) / 120;
  wheel_evt.mouse_wheel_ev.x_pos = GET_X_LPARAM(l_param);
  wheel_evt.mouse_wheel_ev.y_pos = GET_Y_LPARAM(l_param);

  events.input(wheel_evt);
}

LRESULT
xray::ui::application_window::window_procedure(UINT msg, WPARAM w_param,
                                               LPARAM l_param) {
  switch (msg) {
  case WM_CLOSE: {
    OUTPUT_DBG_MSG("WM Close!");
    DestroyWindow(_wininfo.handle);
  } break;

  case WM_DESTROY: {
    OUTPUT_DBG_MSG("Closing window !");
    PostQuitMessage(0);
  } break;

  case WM_SIZE:
    wm_size_event(static_cast<int32_t>(w_param), LOWORD(l_param),
                  HIWORD(l_param));
    break;

  case WM_ENTERSIZEMOVE: {
    _wininfo.resizing = true;
  } break;

  case WM_EXITSIZEMOVE: {
    _wininfo.resizing = false;
  } break;

  case WM_KEYDOWN:
    key_event(w_param, true);
    break;

  case WM_KEYUP:
    key_event(w_param, false);
    break;

  case WM_LBUTTONDOWN:
    mouse_button_event(w_param, l_param, ui::mouse_button::left, true);
    break;

  case WM_LBUTTONUP:
    mouse_button_event(w_param, l_param, ui::mouse_button::left, false);
    break;

  case WM_RBUTTONDOWN:
    mouse_button_event(w_param, l_param, ui::mouse_button::right, true);
    break;

  case WM_RBUTTONUP:
    mouse_button_event(w_param, l_param, ui::mouse_button::right, false);
    break;

   //case WM_GETMINMAXINFO:
  //  handle_wm_getminmaxinfo(reinterpret_cast<MINMAXINFO*>(l_param));
  //  return 0L;
    //break;

  case WM_MOUSEMOVE:
    mouse_move_event(w_param, l_param);
    break;

  case WM_MOUSEWHEEL:
    mouse_wheel_event(w_param, l_param);
    break;

  case WM_ACTIVATE: {
    _wininfo.active = (w_param != WA_INACTIVE);
    OUTPUT_DBG_MSG("Active status : %s",
                   w_param == WA_INACTIVE ? "deactivating" : "activating");
  } break;

  // case WM_SYSCOMMAND:
  //  handle_wm_syscommand(w_param, l_param);
  //  return 0L;
  //  break;

  case WM_SYSKEYDOWN:
    syskeydown_event(w_param, l_param);
    break;

  default:
    return DefWindowProc(_wininfo.handle, msg, w_param, l_param);
    break;
  }

  return 0L;
}
