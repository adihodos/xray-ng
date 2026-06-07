#pragma once

#include "xray/xray.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/base/xray.slice.hpp"

namespace xray::ui {

enum class InputKeySymHandling { Utf8, Unicode };

enum class xrMouseButton_t : U8 {
	button1,
	button2,
	button3,
	button4,
	button5,
	count,
};

//
// Mostly modeled after this: https://gist.github.com/nothings/ef38135f4aa4799e8f09069a44ded5a2
struct xrButton_t {
	U32 is_down		 : 1;
	U32 was_pressed	 : 1;
	U32 was_released : 1;
};

struct xrButtonHistory_t {
	U8 is_down		: 1;
	U8 was_pressed	: 1;
	U8 was_released : 1;
};

struct uiPoint_t {
	I32 x{};
	I32 y{};
};

struct uiSize_t {
	I32 width{};
	I32 height{};
};

struct xrMouseState_t {
	uiPoint_t position{};
	uiPoint_t delta_position{};
	xrButton_t buttons[static_cast<U8>(xrMouseButton_t::count)]{};
	F32 delta_mousewheel{};
};

inline constexpr ISIZE XR_KEYS_COUNT = static_cast<ISIZE>(KeySymbol::count);

struct xrInputHistoryState_t {
	U64 id{};
	F64 wallclock_seconds;
	xrMouseState_t mouse;
	xrButtonHistory_t keys[XR_KEYS_COUNT];
};

inline constexpr ISIZE XR_INPUT_HISTORY_COUNT = 128;

struct WindowState {
	uiSize_t size;
	uiPoint_t position;
	struct {
		U32 maximized : 1;
		U32 minimized : 1;
		U32 quit	  : 1;
	} flags{};
};

struct xrKeySymName_t {
	C8 name[64];
	ISIZE len{};
};

struct WindowCommonCore {
	InputKeySymHandling key_sym_handling{InputKeySymHandling::Unicode};
	WindowState state{};
	xrMouseState_t mouse{};
	xrKeySymName_t key_names[XR_KEYS_COUNT]; // name of all keys
	xrButton_t keys[XR_KEYS_COUNT];		  // keyboard state for this frame
	C32 typing_buff[64];				  // buffer with all chars typed since last frame
	base::xrSlice_t<const C32> typing{};  // slice in the above buffer, for the current frame
	struct {
		xrInputHistoryState_t history[XR_INPUT_HISTORY_COUNT];
	} input{};
	USIZE frame{};
};

}  // namespace xray::ui
