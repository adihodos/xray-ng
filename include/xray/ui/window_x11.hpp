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

#include "xray/xray.hpp"
#include "xray/base/resource_holder.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/window_params.hpp"
#include "xray/ui/window.common.core.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"

#include <X11/Xlib.h>
#include <atomic>
#include <cstdint>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#if !defined(XRAY_GRAPHICS_API_VULKAN)
#include <GL/glx.h>
#endif

// TODO: needs refactoring so that platform specific things don't leak everywhere this file is included

namespace xray {
namespace ui {

namespace detail {

struct x11_display_deleter
{
    void operator()(Display* dpy) const noexcept { XCloseDisplay(dpy); }
};

using x11_unique_display = xray::base::unique_pointer<Display, x11_display_deleter>;

struct x11_window_deleter
{
    using pointer = xray::base::resource_holder<Window, 0>;

    x11_window_deleter() noexcept = default;

    explicit x11_window_deleter(Display* dpy) noexcept
        : _display{ dpy }
    {
    }

    void operator()(Window wnd) const noexcept { XDestroyWindow(_display, wnd); }

  private:
    Display* _display{};
};

using x11_unique_window = xray::base::unique_pointer<Window, x11_window_deleter>;

#ifndef XRAY_GRAPHICS_API_VULKAN
struct glx_context_deleter
{
    glx_context_deleter() noexcept = default;

    explicit glx_context_deleter(Display* dpy) noexcept
        : _display{ dpy }
    {
    }

    void operator()(GLXContext glx_ctx) const noexcept { glXDestroyContext(_display, glx_ctx); }

    Display* _display{};
};

using glx_unique_context = xray::base::unique_pointer<std::remove_pointer<GLXContext>::type, glx_context_deleter>;

#endif

struct XKBContextDeleter
{
    void operator()(xkb_context* c) const noexcept { xkb_context_unref(c); }
};

using unique_xbk_context = xray::base::unique_pointer<xkb_context, XKBContextDeleter>;

struct XKBKeymapDeleter
{
    void operator()(xkb_keymap* k) const noexcept { xkb_keymap_unref(k); }
};

using unique_xkb_keyman = xray::base::unique_pointer<xkb_keymap, XKBKeymapDeleter>;

struct XKBStateDeleter
{
    void operator()(xkb_state* s) const noexcept { xkb_state_unref(s); }
};

using unique_xkb_state = xray::base::unique_pointer<xkb_state, XKBStateDeleter>;

} // namespace detail

class window
{

  public:
    WindowCommonCore core;
    /// \name Construction and destruction.
    /// @{
  public:
    using handle_type = Window;
    using native_display_handle = Display*;

    explicit window(const window_params_t& wparam);
    window(window&&);

    ~window();

    /// @}

    xray::rendering::WindowPlatformDataXlib platform_data() const noexcept;
    handle_type native_window() const noexcept { return base::raw_ptr(_window); }
    native_display_handle native_display() const noexcept { return base::raw_ptr(_display); }
    VisualID native_visual() const noexcept { return _visualid; }

    void disable_cursor() noexcept;
    void enable_cursor() noexcept;

    /// \name Sanity checks.
    /// @{
  public:
    explicit operator bool() const noexcept { return valid(); }
    bool valid() const noexcept
    {
#if defined(XRAY_GRAPHICS_API_VULKAN)
        return true;
#else
        return static_cast<bool>(_glx_context);
#endif
    }

    /// @}

    int32_t width() const noexcept { return _wnd_width; }
    int32_t height() const noexcept { return _wnd_height; }

    void message_loop();
    void quit() noexcept { _quit_flag = true; }
    void swap_buffers() noexcept;

  private:
    void event_mouse_button(const XButtonEvent* x11evt);
    void event_client_message(const XClientMessageEvent* x11evt);
    void event_motion_notify(const XMotionEvent* x11evt);
    void event_key(const XKeyEvent* x11evt);
    void event_configure(const XConfigureEvent* x11evt);

  private:
    detail::x11_unique_display _display;
    detail::x11_unique_window _window;
#ifndef XRAY_GRAPHICS_API_VULKAN
    detail::glx_unique_context _glx_context;
#endif
    VisualID _visualid{};
    Atom _window_delete_atom{ None };
    Atom _motif_hints{ None };
    int32_t _default_screen{ -1 };
    int32_t _wnd_width{ -1 };
    int32_t _wnd_height{ -1 };
    std::atomic<int32_t> _quit_flag{ false };
    bool _kb_grabbed{ false };
    bool _pointer_grabbed{ false };

    struct XInputHelper
    {
        xcb_connection_t* xcb_con{};
        detail::unique_xbk_context xkb_ctx;
        detail::unique_xkb_keyman xkb_keymap;
        detail::unique_xkb_state xkb_state;
        int32_t xkb_event_base{};
        int32_t xkb_error_base{};
        struct KbMods
        {
            uint32_t shift_mod;
            uint32_t caps_mod;
            uint32_t ctrl_mod;
            uint32_t alt_mod;
            uint32_t num_mod;
            uint32_t logo_mod;
        } mod_index{};

    } _input_helper{};

  private:
    XRAY_NO_COPY(window);
};

} // namespace ui
} // namespace xray
