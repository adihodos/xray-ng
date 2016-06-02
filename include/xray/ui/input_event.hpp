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
#include <cstdint>

namespace xray {
namespace ui {

/// \addtogroup __GroupXrayUI
/// @{

///
/// \brief Identifies the buttons of a mouse input device.
enum class mouse_button { left, right, xbutton1, xbutton2 };

///
/// \brief Identifies the source of the input event.
enum class input_event_type {
  key,
  mouse_button,
  mouse_wheel,
  mouse_movement,
  mouse_enter_exit,
  joystick
};

///
/// \brief Data for a key event (press, release).
struct key_event_data_t {
  ///< Identifies the pressed key.
  int32_t key_code;

  ///< If true, the key was pressed, false if depressed.
  bool down;
};

///
/// \brief Mouse button event information.
struct mouse_button_event_data_t {
  ///< The button that generated the event.
  mouse_button btn_id;
  ///< X coordinate of the mouse.
  int32_t x_pos;
  ///< Y coordinate of the mouse.
  int32_t y_pos;
  ///< True if button was pressed, false if it was depressed.
  bool down;
};

///
/// \brief Mouse wheel event data.
struct mouse_wheel_event_data_t {
  ///< Number of revolutions.
  int32_t delta;
  ///< X coordinate of the mouse.
  int32_t x_pos;
  ///< Y coordinate of the mouse.
  int32_t y_pos;
};

///
/// \brief Mouse movement data.
struct mouse_move_event_data_t {
  ///< X coordinate of the mouse.
  int32_t x_pos;
  ///< Y coordinate of the mouse.
  int32_t y_pos;
  union {
    uint32_t value;
    struct {
      uint32_t ctrl : 1;
      uint32_t lbutton : 1;
      uint32_t rbutton : 1;
      uint32_t shift : 1;
    };
  } params;
};

///
/// \brief Sent when the mouse is leaving/entering the client
/// area of a window.
struct mouse_enter_leave_event_t {
  bool entered;
};

///
/// \brief Contains information about an input event (key press/release,
///  mouse button press/release, etc).
struct input_event_t {

  ///< Event data.
  union {
    key_event_data_t          key_ev;
    mouse_button_event_data_t mouse_button_ev;
    mouse_wheel_event_data_t  mouse_wheel_ev;
    mouse_move_event_data_t   mouse_move_ev;
    mouse_enter_leave_event_t mouse_enter_leave;
  };

  ///< Type of event.
  input_event_type type;
};

/// @}

} // namespace ui
} // namespace xray
