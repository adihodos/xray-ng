#pragma once

#include "xray/base/basic_timer.hpp"
#include "xray/ui/event_delegate_types.hpp"
#include "xray/xray.hpp"
#include <cstdint>
#include <windows.h>

namespace xray {
namespace ui {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace win32 {
#else
namespace win32 {
#endif

class basic_window
{
  public:
    struct
    {
        key_delegate keys;
        mouse_button_delegate mouse_button;
        mouse_scroll_delegate mouse_scroll;
        mouse_enter_exit_delegate mouse_enter_exit;
        mouse_move_delegate mouse_move;
        window_size_delegate window_resize;
        draw_delegate_type draw;
        tick_delegate_type tick;
        debug_delegate_type debug;
    } events;

  public:
    static constexpr auto class_name = "##@@__basic_window_class__@@##";
    using handle_type = HWND;

    basic_window(const char* title = nullptr);

    void pump_messages();

    handle_type handle() const noexcept { return window_handle_; }

    explicit operator bool() const noexcept { return handle() != nullptr; }

    auto width() const noexcept { return width_; }

    auto height() const noexcept { return height_; }

  private:
    static LRESULT wnd_proc_stub(HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param);

    LRESULT window_proc(UINT msg_code, WPARAM w_param, LPARAM l_param);

    void init();

  private:
  private:
    HWND window_handle_;

    ///< Timer for update events sent to delegates.
    base::timer_stdp timer_;
    uint32_t width_{ 0 };
    uint32_t height_{ 0 };
};

} // namespace win32
} // namespace ui
} // namespace xray
