#include "basic_window.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/ui/window_context.hpp"

static HMONITOR get_primary_monitor_handle() noexcept {
  const POINT origin_pt = {0, 0};
  return MonitorFromPoint(origin_pt, MONITOR_DEFAULTTOPRIMARY);
}

xray::ui::win32::basic_window::basic_window(const char* title /*= nullptr*/) {
  {
    WNDCLASSEX class_info  = {};
    class_info.cbSize      = sizeof(class_info);
    class_info.style       = 0;
    class_info.lpfnWndProc = basic_window::wnd_proc_stub;
    class_info.hInstance   = GetModuleHandle(nullptr);
    class_info.hCursor     = LoadCursor(nullptr, IDC_ARROW);
    class_info.hIcon       = LoadIcon(nullptr, IDI_APPLICATION);
    class_info.hbrBackground =
        reinterpret_cast<HBRUSH>(GetStockObject(COLOR_WINDOW + 1));
    class_info.lpszClassName = basic_window::class_name;

    if (!RegisterClassEx(&class_info))
      return;
  }

  {
    auto primary_monitor = get_primary_monitor_handle();
    if (!primary_monitor)
      return;

    MONITORINFOEX mon_info{};
    mon_info.cbSize = sizeof(mon_info);

    if (!GetMonitorInfo(primary_monitor, &mon_info)) {
      return;
    }

    const auto r = &mon_info.rcWork;
    width_       = static_cast<uint32_t>(r->right - r->left);
    height_      = static_cast<uint32_t>(r->bottom - r->top);

    OUTPUT_DBG_MSG("Window dimensions [%u x %u]", width(), height());

    constexpr auto wnd_style = WS_POPUP;

    window_handle_ = CreateWindowEx(0L, basic_window::class_name,
                                    title ? title : "Simple window", wnd_style,
                                    0, 0, static_cast<int32_t>(width()),
                                    static_cast<int32_t>(height()), nullptr,
                                    nullptr, GetModuleHandle(nullptr), this);

    if (!window_handle_) {
      return;
    }
  }

  {
    ShowWindow(handle(), SW_SHOWNORMAL);
    timer_.start();
  }
}

LRESULT xray::ui::win32::basic_window::wnd_proc_stub(HWND wnd, UINT msg,
                                                     WPARAM w_param,
                                                     LPARAM l_param) {
  if (msg == WM_NCCREATE) {
    auto cs = reinterpret_cast<const CREATESTRUCT*>(l_param);
    SetWindowLongPtr(wnd, GWLP_USERDATA,
                     reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    return true;
  }

  if (auto obj_ptr = reinterpret_cast<basic_window*>(
          GetWindowLongPtr(wnd, GWLP_USERDATA))) {
    return obj_ptr->window_proc(msg, w_param, l_param);
  }

  return DefWindowProc(wnd, msg, w_param, l_param);
}

LRESULT xray::ui::win32::basic_window::window_proc(UINT   msg_code,
                                                   WPARAM w_param,
                                                   LPARAM l_param) {
  bool msg_handled = true;

  switch (msg_code) {
  case WM_CLOSE:
    DestroyWindow(handle());
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_KEYDOWN:
    break;

  default:
    msg_handled = false;
    break;
  }

  if (!msg_handled)
    return DefWindowProc(handle(), msg_code, w_param, l_param);

  return 0L;
}

void xray::ui::win32::basic_window::pump_messages() {

  for (bool quit_flag = false; !quit_flag;) {
    MSG msg_info{};

    if (PeekMessage(&msg_info, nullptr, 0, 0, PM_NOREMOVE) == FALSE) {
      do {

        //
        // update
        timer_.update_and_reset();
        const auto delta_time = timer_.elapsed_millis();

        if (events.tick)
          events.tick(delta_time);

        if (events.draw) {
          const window_context win_ctx{width(), height()};
          events.draw(win_ctx);
        }

      } while (PeekMessage(&msg_info, nullptr, 0, 0, PM_NOREMOVE) == FALSE);
    }

    do {

      const auto ret_code = GetMessage(&msg_info, nullptr, 0, 0);

      if (ret_code == -1) {
        quit_flag = true;
        break;
      }

      if (ret_code == 0) {
        OUTPUT_DBG_MSG("Caught WM_QUIT");
        quit_flag = true;
        break;
      }

      TranslateMessage(&msg_info);
      DispatchMessage(&msg_info);

    } while (PeekMessage(&msg_info, nullptr, 0, 0, PM_NOREMOVE) == TRUE);
  }
}
