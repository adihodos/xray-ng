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

/// \file window_x11.hpp

#pragma once

#include "xray/base/unique_pointer.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/window.common.core.hpp"
#include "xray/ui/window_params.hpp"
#include "xray/xray.hpp"
#include <atomic>
#include <cstdint>
#include <type_traits>
#include <windows.h>

namespace xray {
namespace ui {

namespace detail {

struct hdc_deleter
{
    HWND _window;
    hdc_deleter() noexcept = default;
    explicit hdc_deleter(const HWND wnd) noexcept
        : _window{ wnd }
    {
    }

    void operator()(HDC dc) const noexcept { ReleaseDC(_window, dc); }
};

using scoped_window_dc = base::unique_pointer<std::remove_pointer_t<HDC>, hdc_deleter>;

struct hglrc_deleter
{
    HDC _windowDC;
    hglrc_deleter() noexcept = default;
    explicit hglrc_deleter(const HDC windowDC) noexcept
        : _windowDC{ windowDC }
    {
    }

    void operator()(HGLRC openglContext) const noexcept
    {
        wglMakeCurrent(_windowDC, nullptr);
        if (openglContext)
            wglDeleteContext(openglContext);
    }
};

using scoped_opengl_context = base::unique_pointer<std::remove_pointer_t<HGLRC>, hglrc_deleter>;

struct wnd_deleter
{
    void operator()(HWND wnd) const noexcept
    {
        if (wnd)
            DestroyWindow(wnd);
    }
};

using scoped_window = base::unique_pointer<std::remove_pointer_t<HWND>, wnd_deleter>;

} // namespace detail

class window
{
  public:
    WindowCommonCore core;

    /// \name Construction and destruction.
    /// @{
  public:
    using handle_type = HWND;

    explicit window(const window_params_t& wparam);
    window(window&&) noexcept;
    window(const window&) = delete;
    window& operator=(const window&) = delete;

    ~window();

    /// @}

    handle_type handle() const noexcept { return _window; }
    handle_type native_window() const noexcept { return _window; }
    HMODULE native_module() const noexcept { return GetModuleHandle(nullptr); }

    void disable_cursor() noexcept;
    void enable_cursor() noexcept;

    /// \name Sanity checks.
    /// @{
  public:
    explicit operator bool() const noexcept { return valid(); }

    bool valid() const noexcept
    {
#if defined(XRAY_RENDERER_OPENGL)
        return _glcontext != nullptr;
#else  /* defined XRAY_RENDERER_OPENGL */
        return _window != nullptr;
#endif /* !defined XRAY_RENDERER_OPENGL */
    }

    /// @}

    int32_t width() const noexcept { return _wnd_width; }
    int32_t height() const noexcept { return _wnd_height; }

    void message_loop();
    void quit() noexcept { _quit_flag = true; }

  private:
    static LRESULT WINAPI window_proc_stub(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);

    LRESULT window_proc(UINT message, WPARAM wparam, LPARAM lparam);

    void initialize(const window_params_t* wpar);

    void event_mouse_button(const uint32_t type, const WPARAM wp, const LPARAM lp);

    void event_mouse_wheel(const WPARAM wparam, const LPARAM lparam);

    void event_key(const uint32_t type, const WPARAM wp, const LPARAM lp);

    void event_motion_notify(const WPARAM wparam, const LPARAM lparam);

    void event_configure(const WPARAM wparam, const LPARAM lparam);

  private:
    HWND _window{ nullptr };

#if defined(XRAY_RENDERER_OPENGL)
    int32_t _pixelformat_id{};
    detail::scoped_window_dc _window_dc{};
    detail::scoped_opengl_context _glcontext{};
#endif /* defined XRAY_RENDERER_OPENGL */

    int32_t _wnd_width{ -1 };
    int32_t _wnd_height{ -1 };
    std::atomic<int32_t> _quit_flag{ false };
};

} // namespace ui
} // namespace xray
