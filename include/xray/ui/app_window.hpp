//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
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

#pragma once

/// \file   input_event.hpp

#include "xray/xray.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/ui/fwd_input_events.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
#include <windows.h>

namespace xray {
namespace ui {

/// \addtogroup __GroupXrayUI
/// @{

struct input_event_t;

struct window_params {
  uint32_t    width;
  uint32_t    height;
  bool        fullscreen;
  const char* title;
};

struct window_draw_event {
  HWND     window;
  uint32_t width;
  uint32_t height;
};

class application_window {
public:
  using tick_event_type = xray::base::fast_delegate<void(const float)>;

  using resize_event_type =
      xray::base::fast_delegate<void(const uint32_t, const uint32_t)>;

  using draw_event_type =
      xray::base::fast_delegate<void(const window_draw_event&)>;

  using input_event_type =
      xray::base::fast_delegate<void(const input_event_t&)>;

  using swap_buffers_event_type =
      xray::base::fast_delegate<HRESULT(const bool)>;

  using toggle_full_screen_event_type = xray::base::fast_delegate<void(void)>;

  struct tag_events {
    tick_event_type               tick;
    resize_event_type             resize;
    draw_event_type               draw;
    input_event_type              input;
    swap_buffers_event_type       swap_buffers;
    toggle_full_screen_event_type toggle_fullscreen;
  } events;

public:
  application_window(const window_params& init_params);

  ~application_window();

  void run() noexcept;

  HWND handle() const noexcept { return _wininfo.handle; }

  explicit operator bool() const noexcept { return is_valid(); }

  uint32_t width() const noexcept { return _wininfo.width; }

  uint32_t height() const noexcept { return _wininfo.height; }

  bool fullscreen() const noexcept { return _wininfo.fullscreen; }

  void quit() noexcept { _wininfo.quitflag = true; }

private:
  bool is_valid() const noexcept { return _wininfo.handle != nullptr; }

  static LRESULT WINAPI wnd_proc_stub(HWND wnd, UINT msg, WPARAM w_param,
                                      LPARAM l_param);

  LRESULT window_procedure(const UINT msg, const WPARAM w_param,
                           const LPARAM l_param);

  void make_frame();

private:
  void wm_size_event(const int32_t size_request, const int32_t width,
                     const int32_t height) noexcept;

  void key_event(const int32_t key_code, const bool down) noexcept;

  void syskeydown_event(const WPARAM w_param, const LPARAM l_param) noexcept;

  void mouse_button_event(WPARAM w_param, LPARAM l_param,
                          const mouse_button button, const bool down) noexcept;

  void mouse_move_event(WPARAM w_param, LPARAM l_param) noexcept;

  void mouse_wheel_event(WPARAM w_param, LPARAM l_param) noexcept;

  void toggle_fullscreen(const bool fs_value) noexcept;

private:
  struct window_info {
    HINSTANCE instance{};
    HWND      handle{};
    uint32_t  width{};
    uint32_t  height{};
    bool      fullscreen{};
    bool      mouse_captured{};
    bool      quitflag{};
    bool      resizing{};
    bool      active{true};
    bool      should_be_fullscreen{false};

    window_info() = default;
  };

  struct saved_window_info {
    bool     maximized{};
    LONG_PTR style{};
    LONG_PTR ex_style{};
    RECT     wnd_rect;
  };

  window_info            _wininfo;
  saved_window_info      _saved_wnd_info;
  xray::base::timer_stdp _timer;
  XRAY_NO_COPY(application_window);
};

/// @}

} // namespace ui
} // namespace xray
