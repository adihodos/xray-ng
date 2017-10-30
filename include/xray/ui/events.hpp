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

/// \file   window_events.hpp

#pragma once

#include "xray/xray.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/ui/key_sym.hpp"
#include <cstddef>
#include <cstdint>

namespace xray {
namespace ui {

class window;

enum class event_type {
  key,
  mouse_button,
  mouse_motion,
  mouse_crossing,
  mouse_wheel,
  configure
};

enum class event_action_type { press, release };

enum class mouse_button { button1, button2, button3, button4, button5 };

struct modifier_mask {
  enum {
    button1 = 1,
    button2 = 1 << 1,
    button3 = 1 << 2,
    button4 = 1 << 3,
    button5 = 1 << 4,
    shift   = 1 << 5,
    control = 1 << 6
  };
};

struct mouse_button_event {
  ///< Press or release
  event_action_type type;
  ///< Pointer to window
  window* wnd;
  ///< X pointer position (client coords)
  int32_t pointer_x;
  ///< Y pointer position (client coords)
  int32_t pointer_y;
  ///< Id of the mouse button
  mouse_button button;
  union {
    ///< Active modifiers
    uint32_t modifiers;
    struct {
      uint32_t button1 : 1;
      uint32_t button2 : 1;
      uint32_t button3 : 1;
      uint32_t button4 : 1;
      uint32_t button5 : 1;
      uint32_t shift : 1;
      uint32_t control : 1;
    };
  };
};

struct mouse_wheel_event {
  ///< Amount of motion.
  int32_t delta;
  ///< Pointer to window
  window* wnd;
  ///< X pointer position (client coords)
  int32_t pointer_x;
  ///< Y pointer position (client coords)
  int32_t pointer_y;
  union {
    ///< Active modifiers
    uint32_t modifiers;
    struct {
      uint32_t button1 : 1;
      uint32_t button2 : 1;
      uint32_t button3 : 1;
      uint32_t button4 : 1;
      uint32_t button5 : 1;
      uint32_t shift : 1;
      uint32_t control : 1;
    };
  };
};

struct mouse_motion_event {
  ///< Pointer to window
  window* wnd;
  ///< X pointer position (client coords)
  int32_t pointer_x;
  ///< Y pointer position (client coords)
  int32_t pointer_y;
  union {
    ///< Active modifiers
    uint32_t modifiers;
    struct {
      uint32_t button1 : 1;
      uint32_t button2 : 1;
      uint32_t button3 : 1;
      uint32_t button4 : 1;
      uint32_t button5 : 1;
      uint32_t shift : 1;
      uint32_t control : 1;
    };
  };
};

struct key_event {
  ///< Pointer to window
  window* wnd;
  ///< X pointer position (client coords)
  int32_t pointer_x;
  ///< Y pointer position (client coords)
  int32_t pointer_y;
  ///< Code of key that generated the event.
  key_sym::e keycode;
  ///< Press or release
  event_action_type type;
  union {
    ///< Active modifiers
    uint32_t modifiers;
    struct {
      uint32_t button1 : 1;
      uint32_t button2 : 1;
      uint32_t button3 : 1;
      uint32_t button4 : 1;
      uint32_t button5 : 1;
      uint32_t shift : 1;
      uint32_t control : 1;
    };
  };

  char name[32];
};

struct window_configure_event {
  int32_t width;
  int32_t height;
  window* wnd;
};

struct window_loop_event {
  int32_t wnd_width;
  int32_t wnd_height;
  window* wnd;
};

struct window_event {
  event_type type;
  union {
    mouse_button_event     button;
    mouse_wheel_event      wheel;
    mouse_motion_event     motion;
    key_event              key;
    window_configure_event configure;
  } event;
};

inline bool is_input_event(const window_event& we) noexcept {
  return we.type == event_type::key || we.type == event_type::mouse_button ||
         we.type == event_type::mouse_motion ||
         we.type == event_type::mouse_crossing ||
         we.type == event_type::mouse_wheel;
}

struct poll_start_event {};
struct poll_end_event {
  int32_t surface_width;
  int32_t surface_height;
};

using loop_event_delegate = base::fast_delegate<void(const window_loop_event&)>;
using window_event_delegate = base::fast_delegate<void(const window_event&)>;
using poll_start_event_delegate =
  base::fast_delegate<void(const poll_start_event&)>;
using poll_end_event_delegate =
  base::fast_delegate<void(const poll_end_event&)>;

} // namespace ui
} // namespace xray
