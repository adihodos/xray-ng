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
#include "xray/ui/key_sym.hpp"
#include "xray/ui/events.gamepad.hpp"

namespace xray {
namespace ui {

class PlatformWindow;

enum class event_type {
	key,
	char_input,
	mouse_button,
	mouse_motion,
	mouse_crossing,
	mouse_wheel,
	configure,
	gamepad_axis,
	gamepad_button,
};

enum class event_action_type : U8 { press, release };

enum class mouse_button : U8 {
	button1,
	button2,
	button3,
	button4,
	button5,
	count,
};

struct modifier_mask {
	enum {
		button1 = 1,
		button2 = 1 << 1,
		button3 = 1 << 2,
		button4 = 1 << 3,
		button5 = 1 << 4,
		shift	= 1 << 5,
		control = 1 << 6
	};
};

struct mouse_button_event {
	///< Press or release
	event_action_type type;
	///< Pointer to window
	PlatformWindow* wnd;
	///< X pointer position (client coords)
	I32 pointer_x;
	///< Y pointer position (client coords)
	I32 pointer_y;
	///< Id of the mouse button
	mouse_button button;
	union {
		///< Active modifiers
		U32 modifiers;
		struct {
			U32 button1 : 1;
			U32 button2 : 1;
			U32 button3 : 1;
			U32 button4 : 1;
			U32 button5 : 1;
			U32 shift	: 1;
			U32 control : 1;
		};
	};
};

struct mouse_wheel_event {
	///< Amount of motion.
	I32 delta;
	F32 fdelta;
	///< Pointer to window
	PlatformWindow* wnd;
	///< X pointer position (client coords)
	I32 pointer_x;
	///< Y pointer position (client coords)
	I32 pointer_y;
	union {
		///< Active modifiers
		U32 modifiers;
		struct {
			U32 button1 : 1;
			U32 button2 : 1;
			U32 button3 : 1;
			U32 button4 : 1;
			U32 button5 : 1;
			U32 shift	: 1;
			U32 control : 1;
		};
	};
};

struct mouse_motion_event {
	///< Pointer to window
	PlatformWindow* wnd;
	///< X pointer position (client coords)
	I32 pointer_x;
	///< Y pointer position (client coords)
	I32 pointer_y;
	union {
		///< Active modifiers
		U32 modifiers;
		struct {
			U32 button1 : 1;
			U32 button2 : 1;
			U32 button3 : 1;
			U32 button4 : 1;
			U32 button5 : 1;
			U32 shift	: 1;
			U32 control : 1;
		};
	};
};

struct key_event {
	///< Pointer to window
	PlatformWindow* wnd;
	///< X pointer position (client coords)
	I32 pointer_x;
	///< Y pointer position (client coords)
	I32 pointer_y;
	///< Code of key that generated the event.
	KeySymbol keycode;
	///< Press or release
	event_action_type type;
	union {
		///< Active modifiers
		U32 modifiers;
		struct {
			// U32 mod1 : 1;
			// U32 mod2 : 1;
			// U32 mod3 : 1;
			// U32 mod4 : 1;
			// U32 mod5 : 1;

			U32 meta	: 1;
			U32 alt		: 1;
			U32 shift	: 1;
			U32 control : 1;
		};
	};

	C8 name[32];
};

struct char_input_event {
	PlatformWindow* wnd;
	KeySymbol key_code;
	U32 unicode_point;
	C8 utf8[32];
};

struct window_configure_event {
	I32 width;
	I32 height;
	PlatformWindow* wnd;
};

struct window_loop_event {
	I32 wnd_width;
	I32 wnd_height;
	PlatformWindow* wnd;
};

struct window_event {
	event_type type;
	union {
		mouse_button_event button;
		mouse_wheel_event wheel;
		mouse_motion_event motion;
		key_event key;
		char_input_event char_input;
		window_configure_event configure;
		GamepadAxisEvent gamepad_axis;
		GamepadButtonEvent gamepad_button;
	} event;

	static window_event configure_event(const I32 width, const I32 height, PlatformWindow* wnd) noexcept {
		window_event we;
		we.type					  = event_type::configure;
		we.event.configure.wnd	  = wnd;
		we.event.configure.width  = width;
		we.event.configure.height = height;

		return we;
	}
};

inline bool is_input_event(const window_event& we) noexcept {
	return we.type == event_type::key || we.type == event_type::mouse_button || we.type == event_type::mouse_motion ||
		   we.type == event_type::mouse_crossing || we.type == event_type::mouse_wheel ||
		   we.type == event_type::char_input || we.type == event_type::gamepad_button ||
		   we.type == event_type::gamepad_axis;
}
	
}  // namespace ui
}  // namespace xray
