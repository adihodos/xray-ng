#include "xray/ui/platform.window.x11.hpp"

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrender.h>
#include <X11/keysym.h>

#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <linux/input.h>

#include <algorithm>
#include <span>
#include <filesystem>
#include <ranges>
#include <system_error>

#include <tl/optional.hpp>
#include <fmt/format.h>
#include <fmt/std.h>

#include "xray/base/array_dimension.hpp"
#include "xray/base/syscall_wrapper.hpp"
#include "xray/base/containers/fixed_vector.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/events.pretty.print.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/resource_holder.hpp"

using namespace xray::base;
using namespace xray::ui;
using namespace std;

tl::optional<XineramaScreenInfo> get_primary_screen_info(Display* dpy) {
	int32_t num_screens{};
	unique_pointer<XineramaScreenInfo, decltype(&XFree)> screens{XineramaQueryScreens(dpy, &num_screens), &XFree};

	if (!num_screens) {
		return tl::nullopt;
	}

	const auto root_screen = DefaultScreen(dpy);
	auto itr_def_scr =
		std::find_if(raw_ptr(screens), raw_ptr(screens) + num_screens, [root_screen](const XineramaScreenInfo& si) {
			return root_screen == si.screen_number;
		});

	if (itr_def_scr == (raw_ptr(screens) + num_screens)) {
		return tl::nullopt;
	}

	return *itr_def_scr;
}

// clang-format off

static constexpr const KeySymbol X11_MISC_FUNCTION_KEYS_MAPPING_TABLE[] = {
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::backspace,
	KeySymbol::tab,
	KeySymbol::unknown,
	KeySymbol::clear,
	KeySymbol::unknown,
	KeySymbol::enter,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::pause,
	KeySymbol::scrol_lock,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::escape,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::home,
	KeySymbol::left,
	KeySymbol::up,
	KeySymbol::right,
	KeySymbol::down,
	KeySymbol::page_up,
	KeySymbol::page_down,
	KeySymbol::end,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::select,
	KeySymbol::print_screen,
	KeySymbol::unknown,
	KeySymbol::insert,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::num_lock,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::kp_multiply,
	KeySymbol::kp_add,
	KeySymbol::unknown,
	KeySymbol::kp_minus,
	KeySymbol::kp_decimal,
	KeySymbol::kp_divide,
	KeySymbol::kp0,
	KeySymbol::kp1,
	KeySymbol::kp2,
	KeySymbol::kp3,
	KeySymbol::kp4,
	KeySymbol::kp5,
	KeySymbol::kp6,
	KeySymbol::kp7,
	KeySymbol::kp8,
	KeySymbol::kp9,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::f1,
	KeySymbol::f2,
	KeySymbol::f3,
	KeySymbol::f4,
	KeySymbol::f5,
	KeySymbol::f6,
	KeySymbol::f7,
	KeySymbol::f8,
	KeySymbol::f9,
	KeySymbol::f10,
	KeySymbol::f11,
	KeySymbol::f12,
	KeySymbol::f13,
	KeySymbol::f14,
	KeySymbol::f15,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::left_shift,
	KeySymbol::right_shift,
	KeySymbol::left_control,
	KeySymbol::right_control,
	KeySymbol::caps_lock,
	KeySymbol::unknown,
	KeySymbol::left_menu,
	KeySymbol::right_menu,
	KeySymbol::left_alt,
	KeySymbol::right_alt,
	KeySymbol::left_win,
	KeySymbol::right_win,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::del,
};

// clang-format on

// clang-format off

static constexpr const KeySymbol X11_LATIN1_KEYS_MAPPING_TABLE[] = {
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::space,
	KeySymbol::exclam,
	KeySymbol::quotedbl,
	KeySymbol::numbersign,
	KeySymbol::dollar,
	KeySymbol::percent,
	KeySymbol::ampersand,
	KeySymbol::quoteright,
	KeySymbol::parenleft,
	KeySymbol::parenright,
	KeySymbol::asterisk,
	KeySymbol::plus,
	KeySymbol::comma,
	KeySymbol::minus,
	KeySymbol::period,
	KeySymbol::slash,
	KeySymbol::key_0,
	KeySymbol::key_1,
	KeySymbol::key_2,
	KeySymbol::key_3,
	KeySymbol::key_4,
	KeySymbol::key_5,
	KeySymbol::key_6,
	KeySymbol::key_7,
	KeySymbol::key_8,
	KeySymbol::key_9,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::key_a,
	KeySymbol::key_b,
	KeySymbol::key_c,
	KeySymbol::key_d,
	KeySymbol::key_e,
	KeySymbol::key_f,
	KeySymbol::key_g,
	KeySymbol::key_h,
	KeySymbol::key_i,
	KeySymbol::key_j,
	KeySymbol::key_k,
	KeySymbol::key_l,
	KeySymbol::key_m,
	KeySymbol::key_n,
	KeySymbol::key_o,
	KeySymbol::key_p,
	KeySymbol::key_q,
	KeySymbol::key_r,
	KeySymbol::key_s,
	KeySymbol::key_t,
	KeySymbol::key_u,
	KeySymbol::key_v,
	KeySymbol::key_w,
	KeySymbol::key_x,
	KeySymbol::key_y,
	KeySymbol::key_z,
	KeySymbol::bracketleft,
	KeySymbol::backslash,
	KeySymbol::bracketright,
	KeySymbol::asciicircum,
	KeySymbol::underscore,
	KeySymbol::quoteleft,
	KeySymbol::key_a,
	KeySymbol::key_b,
	KeySymbol::key_c,
	KeySymbol::key_d,
	KeySymbol::key_e,
	KeySymbol::key_f,
	KeySymbol::key_g,
	KeySymbol::key_h,
	KeySymbol::key_i,
	KeySymbol::key_j,
	KeySymbol::key_k,
	KeySymbol::key_l,
	KeySymbol::key_m,
	KeySymbol::key_n,
	KeySymbol::key_o,
	KeySymbol::key_p,
	KeySymbol::key_q,
	KeySymbol::key_r,
	KeySymbol::key_s,
	KeySymbol::key_t,
	KeySymbol::key_u,
	KeySymbol::key_v,
	KeySymbol::key_w,
	KeySymbol::key_x,
	KeySymbol::key_y,
	KeySymbol::key_z,
	KeySymbol::braceleft,
	KeySymbol::bar,
	KeySymbol::braceright,
	KeySymbol::asciitilde,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
};

// clang-format on

static xray::ui::KeySymbol map_x11_key_symbol(const xkb_keysym_t key_sym) noexcept {
	const uint32_t x11_key = static_cast<uint32_t>(key_sym);
	//
	// special keys have byte2 set to 0xFF, regular keys to 0x00
	const auto byte2 = (x11_key & 0xFF00) >> 8;
	//
	//  byte 0 is the table lookup index
	const auto sym_idx = x11_key & 0xFF;

	if (byte2 == 0x00) {
		assert(sym_idx < XR_COUNTOF(X11_LATIN1_KEYS_MAPPING_TABLE));
		return X11_LATIN1_KEYS_MAPPING_TABLE[sym_idx];
	} else if (byte2 == 0xFF) {
		assert(sym_idx < XR_COUNTOF(X11_MISC_FUNCTION_KEYS_MAPPING_TABLE));
		return X11_MISC_FUNCTION_KEYS_MAPPING_TABLE[sym_idx];
	} else {
		return xray::ui::KeySymbol::unknown;
	}
}

struct GamepadAxisMapping {
	int32_t native;
	GamepadAxis translated;
};

struct GamepadButtonMapping {
	int32_t native;
	GamepadButton translated;
};

struct PlatformGamepad {
	static constexpr const GamepadAxisMapping AXIS_MAPPING[] = {
		{ABS_X, GamepadAxis::LeftX},
		{ABS_Y, GamepadAxis::LeftY},
		{ABS_RX, GamepadAxis::RightX},
		{ABS_RY, GamepadAxis::RightY},
		{ABS_Z, GamepadAxis::LeftZ},
		{ABS_RZ, GamepadAxis::RightZ},
	};

	static constexpr const GamepadButtonMapping BUTTON_MAPPING[] = {
		{BTN_TL, GamepadButton::Left},
		{BTN_TL2, GamepadButton::Left2},
		{BTN_TR, GamepadButton::Right},
		{BTN_TR2, GamepadButton::Right2},
	};

	std::pair<bool, window_event> make_window_event(const input_event& evt) noexcept;

	uint32_t _device_id;
	std::filesystem::path _device_path;
	int32_t _fd;
	std::vector<xray::ui::GamepadAxisInfo> _axis_info;
};

std::pair<bool, xray::ui::window_event> PlatformGamepad::make_window_event(const input_event& evt) noexcept {
	if (evt.type == EV_ABS) {
		auto itr_translated =
			ranges::find_if(PlatformGamepad::AXIS_MAPPING, [axis = evt.code](const GamepadAxisMapping& axismapping) {
				return axismapping.native == axis;
			});

		if (itr_translated != ranges::cend(PlatformGamepad::AXIS_MAPPING)) {
			// XR_LOG_INFO("[[gamepad event]] {}", itr_translated->translated);
			window_event win_event;
			win_event.type				 = event_type::gamepad_axis;
			win_event.event.gamepad_axis = GamepadAxisEvent{
				.axis = itr_translated->translated,
				.i32  = evt.value,
				.f32  = static_cast<float>(evt.value) /
					   static_cast<float>(_axis_info[static_cast<size_t>(itr_translated->translated)].max_val),
				.timestamp = (evt.time.tv_sec * 1000000ULL + evt.time.tv_usec) * 1000,
			};

			return std::pair{true, win_event};
		}
	} else if (evt.type == EV_KEY) {
		auto itr_translated = ranges::find_if(
			PlatformGamepad::BUTTON_MAPPING,
			[button = evt.code](const GamepadButtonMapping& axismapping) { return axismapping.native == button; }
		);

		if (itr_translated != ranges::end(PlatformGamepad::BUTTON_MAPPING)) {
			window_event win_event;
			win_event.type				   = event_type::gamepad_button;
			win_event.event.gamepad_button = GamepadButtonEvent{
				.button	   = itr_translated->translated,
				.i32	   = evt.value,
				.timestamp = (evt.time.tv_sec * 1000000ULL + evt.time.tv_usec) * 1000,
			};

			return std::pair{true, win_event};
		}
	} else {
		// XR_LOG_INFO("unhandled: {} - {} - {}", evt.type, evt.code, evt.value);
	}

	return std::pair{false, window_event{}};
}

namespace xray::ui {
namespace detail {

struct x11_display_deleter {
	void operator()(Display* dpy) const noexcept { XCloseDisplay(dpy); }
};

using x11_unique_display = xray::base::unique_pointer<Display, x11_display_deleter>;

struct x11_window_deleter {
	using pointer = xray::base::resource_holder<Window, 0>;

	x11_window_deleter() noexcept = default;

	explicit x11_window_deleter(Display* dpy) noexcept : _display{dpy} {}

	void operator()(Window wnd) const noexcept { XDestroyWindow(_display, wnd); }

private:
	Display* _display{};
};

using x11_unique_window = xray::base::unique_pointer<Window, x11_window_deleter>;

struct XKBContextDeleter {
	void operator()(xkb_context* c) const noexcept { xkb_context_unref(c); }
};

using unique_xbk_context = xray::base::unique_pointer<xkb_context, XKBContextDeleter>;

struct XKBKeymapDeleter {
	void operator()(xkb_keymap* k) const noexcept { xkb_keymap_unref(k); }
};

using unique_xkb_keyman = xray::base::unique_pointer<xkb_keymap, XKBKeymapDeleter>;

struct XKBStateDeleter {
	void operator()(xkb_state* s) const noexcept { xkb_state_unref(s); }
};

using unique_xkb_state = xray::base::unique_pointer<xkb_state, XKBStateDeleter>;

struct XInputHelper {
	xcb_connection_t* xcb_con{};
	detail::unique_xbk_context xkb_ctx;
	detail::unique_xkb_keyman xkb_keymap;
	detail::unique_xkb_state xkb_state;
	int32_t xkb_event_base{};
	int32_t xkb_error_base{};
	struct KbMods {
		uint32_t shift_mod;
		uint32_t caps_mod;
		uint32_t ctrl_mod;
		uint32_t alt_mod;
		uint32_t num_mod;
		uint32_t logo_mod;
	} mod_index{};
};

struct PolledDevicesState {
	int32_t _epoll_fd;
	int32_t _x11_display_fd;
	std::vector<PlatformGamepad> _gamepads;

	static tl::expected<PolledDevicesState, std::error_condition> create(Display* dpy);
};

tl::expected<PolledDevicesState, std::error_condition> PolledDevicesState::create(Display* dpy) {
	const int32_t x11_fd = XConnectionNumber(dpy);
	if (x11_fd == -1) {
		return tl::make_unexpected(std::error_code{errno, std::system_category()}.default_error_condition());
	}

	const int32_t epoll_fd = epoll_create(32);
	if (epoll_fd == -1) {
		return tl::make_unexpected(std::error_code{errno, std::system_category()}.default_error_condition());
	}

	epoll_event e{.events = EPOLLIN | EPOLLERR, .data = {.fd = x11_fd}};
	if (const int32_t add_fd_result = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, x11_fd, &e); add_fd_result != 0) {
		return tl::make_unexpected(std::error_code{errno, std::system_category()}.default_error_condition());
	}

	namespace fs = std::filesystem;

	vector<PlatformGamepad> gamepads;

	auto gamepads_range =
		fs::directory_iterator{fs::path{"/dev/input/by-id"}} | std::views::filter([](const fs::directory_entry& de) {
			return de.is_character_file() && de.path().generic_string().find("event-joystick") != std::string::npos;
		}) |
		std::views::transform(
			[epoll_fd](const fs::directory_entry& de) -> tl::expected<PlatformGamepad, std::error_condition> {
				XR_LOG_INFO("gamepad device {}", de.path());
				int32_t gamepad_fd = open(de.path().generic_string().c_str(), O_RDONLY | O_NONBLOCK);
				if (gamepad_fd == -1) {
					return tl::make_unexpected(std::error_code{errno, std::system_category()}.default_error_condition()
					);
				}

				vector<GamepadAxisInfo> axis_info;
				for (const GamepadAxisMapping axis_mapping : PlatformGamepad::AXIS_MAPPING) {
					input_absinfo abs_info{};
					const int32_t res = syscall_wrapper(ioctl, gamepad_fd, EVIOCGABS(axis_mapping.native), &abs_info);
					if (res == -1) {
						return tl::make_unexpected(
							std::error_code{errno, std::system_category()}.default_error_condition()
						);
					}
					axis_info.push_back(GamepadAxisInfo{
						.min_val  = abs_info.minimum,
						.max_val  = abs_info.maximum,
						.deadzone = abs_info.flat,
					});

					XR_LOG_INFO(
						"axis: {}, min {}, max {}, deadzone {}",
						axis_mapping.native,
						abs_info.minimum,
						abs_info.maximum,
						abs_info.flat
					);
				}

				epoll_event e = {
					.events = EPOLLIN | EPOLLERR | EPOLLET,
					.data =
						{
							.fd = gamepad_fd,
						},
				};

				const int32_t add_res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, gamepad_fd, &e);
				if (add_res == -1) {
					return tl::make_unexpected(std::error_code{errno, std::system_category()}.default_error_condition()
					);
				}

				return tl::expected<PlatformGamepad, std::error_condition>{
					tl::in_place, FNV::fnv1a(de.path().generic_string()), de.path(), gamepad_fd, axis_info
				};
			}
		) |
		std::views::filter([](tl::expected<PlatformGamepad, std::error_condition> gp) { return gp.has_value(); });

	for (auto gp : gamepads_range) {
		XR_LOG_INFO("Gamepad {} -> {}", gp->_device_path.generic_string(), gp->_device_id);
		gamepads.push_back(std::move(*gp));
	}

	return tl::expected<PolledDevicesState, std::error_condition>{
		tl::in_place,
		epoll_fd,
		x11_fd,
		std::move(gamepads),
	};
}

}  // namespace detail
}  // namespace xray::ui

struct xray::ui::PlatformWindow::PlatformImpl {
	detail::x11_unique_display _display;
	detail::x11_unique_window _window;
	VisualID _visualid{};
	Atom _window_delete_atom{None};
	int32_t _default_screen{-1};
	int32_t _wnd_width{-1};
	int32_t _wnd_height{-1};
	bool _quit_flag{false};
	bool _kb_grabbed{false};
	bool _pointer_grabbed{false};

	detail::XInputHelper _input_helper{};
	detail::PolledDevicesState _poll_state{};

	PlatformImpl(
		detail::x11_unique_display display,
		detail::x11_unique_window window,
		VisualID visual,
		Atom wnd_delete_atom,
		int32_t def_screen,
		int32_t width,
		int32_t height,
		const bool kb_grab,
		const bool pointer_grab,
		detail::XInputHelper input_helper,
		detail::PolledDevicesState poll_state
	)
		: _display{std::move(display)},
		  _window{std::move(window)},
		  _visualid{visual},
		  _window_delete_atom{wnd_delete_atom},
		  _default_screen{def_screen},
		  _wnd_width{width},
		  _wnd_height{height},
		  _kb_grabbed{kb_grab},
		  _pointer_grabbed{pointer_grab},
		  _input_helper{std::move(input_helper)},
		  _poll_state{std::move(poll_state)} {}

	PlatformImpl(PlatformImpl&& rhs) noexcept
		: _display{std::move(rhs._display)},
		  _window{std::move(rhs._window)},
		  _visualid{rhs._visualid},
		  _window_delete_atom{rhs._window_delete_atom},
		  _default_screen{rhs._default_screen},
		  _wnd_width{rhs._wnd_width},
		  _wnd_height{rhs._wnd_height},
		  _kb_grabbed{std::exchange(rhs._kb_grabbed, false)},
		  _pointer_grabbed{std::exchange(rhs._pointer_grabbed, false)},
		  _input_helper{std::move(rhs._input_helper)},
		  _poll_state{std::move(rhs._poll_state)} {}

	enum class ClientMessageResult_t : uint8_t {
		Quit,
		Continue,
	};

	void event_mouse_button(const XButtonEvent* x11evt, PlatformWindow* wnd);
	ClientMessageResult_t event_client_message(const XClientMessageEvent* x11evt);
	void event_motion_notify(const XMotionEvent* x11evt, PlatformWindow* wnd);
	void event_key(const XKeyEvent* x11evt, PlatformWindow* wnd);
	void event_configure(const XConfigureEvent* x11evt, PlatformWindow* wnd);
};

void xray::ui::PlatformWindow::PlatformImpl::event_mouse_button(const XButtonEvent* x11evt, PlatformWindow* wnd) {
	struct x11_button_mapping {
		uint32_t x11_btn;
		mouse_button xray_btn;
	};

	{
		static constexpr x11_button_mapping REGULAR_BUTTON_MAPPINGS[] = {
			{Button1, mouse_button::button1},
			{Button2, mouse_button::button2},
			{Button3, mouse_button::button3},
		};

		auto mapped_button = find_if(
			begin(REGULAR_BUTTON_MAPPINGS),
			end(REGULAR_BUTTON_MAPPINGS),
			[btnid = x11evt->button](const auto& mapping) { return mapping.x11_btn == btnid; }
		);

		if (mapped_button != end(REGULAR_BUTTON_MAPPINGS)) {
			//
			//
			mouse_button_event mbe;
			mbe.type	  = x11evt->type == ButtonPress ? event_action_type::press : event_action_type::release;
			mbe.pointer_x = x11evt->x;
			mbe.pointer_y = x11evt->y;
			mbe.button	  = mapped_button->xray_btn;
			mbe.button1	  = (x11evt->state & Button1Mask) != 0;
			mbe.button2	  = (x11evt->state & Button2Mask) != 0;
			mbe.button3	  = (x11evt->state & Button3Mask) != 0;
			mbe.button4	  = (x11evt->state & Button4Mask) != 0;
			mbe.button5	  = (x11evt->state & Button5Mask) != 0;
			mbe.shift	  = (x11evt->state & ShiftMask) != 0;
			mbe.control	  = (x11evt->state & ControlMask) != 0;
			mbe.wnd		  = wnd;

			window_event we;
			we.type			= event_type::mouse_button;
			we.event.button = mbe;

			wnd->core.events.window(we);
		}
	}

	mouse_wheel_event mwe;
	mwe.delta	  = x11evt->button == Button4 ? +1 : -1;
	mwe.fdelta	  = mwe.delta;
	mwe.pointer_x = x11evt->x;
	mwe.pointer_y = x11evt->y;
	mwe.button1	  = (x11evt->state & Button1Mask) != 0;
	mwe.button2	  = (x11evt->state & Button2Mask) != 0;
	mwe.button3	  = (x11evt->state & Button3Mask) != 0;
	mwe.button4	  = (x11evt->state & Button4Mask) != 0;
	mwe.button5	  = (x11evt->state & Button5Mask) != 0;
	mwe.shift	  = (x11evt->state & ShiftMask) != 0;
	mwe.control	  = (x11evt->state & ControlMask) != 0;
	mwe.wnd		  = wnd;

	window_event we;
	we.type		   = event_type::mouse_wheel;
	we.event.wheel = mwe;

	wnd->core.events.window(we);
}

xray::ui::PlatformWindow::PlatformImpl::ClientMessageResult_t
xray::ui::PlatformWindow::PlatformImpl::event_client_message(const XClientMessageEvent* x11evt) {
	if ((uint32_t)x11evt->data.l[0] == _window_delete_atom) {
		XR_LOG_INFO("Quit received, exiting message loop...");
		return ClientMessageResult_t::Quit;
	}

	return ClientMessageResult_t::Continue;
}

void xray::ui::PlatformWindow::PlatformImpl::event_motion_notify(const XMotionEvent* x11evt, PlatformWindow* wnd) {
	mouse_motion_event mme;
	mme.pointer_x = x11evt->x;
	mme.pointer_y = x11evt->y;
	mme.button1	  = (x11evt->state & Button1Mask) != 0;
	mme.button2	  = (x11evt->state & Button2Mask) != 0;
	mme.button3	  = (x11evt->state & Button3Mask) != 0;
	mme.button4	  = (x11evt->state & Button4Mask) != 0;
	mme.button5	  = (x11evt->state & Button5Mask) != 0;
	mme.shift	  = (x11evt->state & ShiftMask) != 0;
	mme.control	  = (x11evt->state & ControlMask) != 0;
	mme.wnd		  = wnd;

	window_event we;
	we.type			= event_type::mouse_motion;
	we.event.motion = mme;

	wnd->core.events.window(we);
}

void xray::ui::PlatformWindow::PlatformImpl::event_key(const XKeyEvent* x11evt, PlatformWindow* wnd) {
	const xkb_keycode_t key_code{x11evt->keycode};
	const xkb_keysym_t key_sym{xkb_state_key_get_one_sym(raw_ptr(_input_helper.xkb_state), key_code)};

	key_event ke;
	if (const int bytes_len = xkb_keysym_get_name(key_sym, ke.name, sizeof(ke.name)); bytes_len >= 0)
		ke.name[bytes_len] = 0;

	if (const KeySymbol mapped_key = map_x11_key_symbol(key_sym); mapped_key != KeySymbol::unknown) {
		ke.type		 = x11evt->type == KeyPress ? event_action_type::press : event_action_type::release;
		ke.keycode	 = mapped_key;
		ke.pointer_x = x11evt->x;
		ke.pointer_y = x11evt->y;
		ke.wnd		 = wnd;

		ke.meta	   = (x11evt->state & Mod4Mask) != 0;
		ke.alt	   = (x11evt->state & Mod1Mask) != 0;
		ke.shift   = (x11evt->state & ShiftMask) != 0;
		ke.control = (x11evt->state & ControlMask) != 0;

		window_event we;
		we.type		 = event_type::key;
		we.event.key = ke;

		wnd->core.events.window(we);
	}

	if (x11evt->type == KeyPress) {
		// https://gist.github.com/bluetech/6038239
		// https://stackoverflow.com/questions/64722105/without-creating-a-window-is-it-possible-to-detect-the-current-xcb-modifier-sta
		// https://www.x.org/releases/X11R7.7/doc/kbproto/xkbproto.html
		// https://github.com/xkbcommon/libxkbcommon/blob/master/test/state.c
		// https://github.com/xkbcommon/libxkbcommon/blob/master/doc/quick-guide.md
		// https://stackoverflow.com/questions/10157826/xkb-how-to-convert-a-keycode-to-keysym?rq=3
		char_input_event ch_input;
		ch_input.wnd	  = wnd;
		ch_input.key_code = ke.keycode;

		if (wnd->core.key_sym_handling == InputKeySymHandling::Unicode) {
			ch_input.unicode_point = xkb_state_key_get_utf32(raw_ptr(_input_helper.xkb_state), key_code);

			if (ch_input.unicode_point != 0) {
				window_event we;
				we.type				= event_type::char_input;
				we.event.char_input = ch_input;
				wnd->core.events.window(we);
			}
		} else {
			KeySym x11_key_sym;
			uint32_t mod_return{};
			XkbLookupKeySym(raw_ptr(_display), x11evt->keycode, x11evt->state, &mod_return, &x11_key_sym);

			const int32_t num_bytes = XkbTranslateKeySym(
				raw_ptr(_display), &x11_key_sym, x11evt->state, ch_input.utf8, sizeof(ch_input.utf8), nullptr
			);
			ch_input.utf8[num_bytes > 0 ? num_bytes : 0] = 0;

			if (num_bytes > 0) {
				window_event we;
				we.type				= event_type::char_input;
				we.event.char_input = ch_input;
				wnd->core.events.window(we);
			}
		}
	}
}

void xray::ui::PlatformWindow::PlatformImpl::event_configure(const XConfigureEvent* x11evt, PlatformWindow* wnd) {
	_wnd_width	= x11evt->width;
	_wnd_height = x11evt->height;

	window_configure_event cfg_evt;
	cfg_evt.width  = x11evt->width;
	cfg_evt.height = x11evt->height;
	cfg_evt.wnd	   = wnd;

	window_event we;
	we.type			   = event_type::configure;
	we.event.configure = cfg_evt;

	wnd->core.events.window(we);
}

tl::expected<xray::ui::PlatformWindow, xray::ui::PlatformWindowError> xray::ui::PlatformWindow::create(
	const xray::ui::window_params_t& win_params
) {
	using namespace xray::ui::detail;

	int32_t ver_major{XkbMajorVersion};
	int32_t ver_minor{XkbMinorVersion};
	int32_t err_code{};
	int32_t xkb_event_base{};
	int32_t xkb_error_base{};
	x11_unique_display _display{
		XkbOpenDisplay(nullptr, &xkb_event_base, &xkb_error_base, &ver_major, &ver_minor, &err_code)
	};

	if (!_display) {
		XR_LOG_CRITICAL("Failed to open display, error {}", err_code);
		return tl::make_unexpected(PlatformWindowError{});
	}

	constexpr const uint32_t xkb_details_mask =
		XkbModifierBaseMask | XkbModifierStateMask | XkbModifierLatchMask | XkbModifierLockMask;

	XkbSelectEvents(raw_ptr(_display), XkbUseCoreKbd, XkbStateNotifyMask, XkbStateNotifyMask);
	XkbSelectEventDetails(raw_ptr(_display), XkbUseCoreKbd, XkbStateNotifyMask, xkb_details_mask, xkb_details_mask);

	// xcb
	xcb_connection_t* connection = XGetXCBConnection(raw_ptr(_display));
	if (!connection) {
		XR_LOG_ERR("Failed to get XCB connection");
	}

	detail::unique_xbk_context xkb_ctx{xkb_context_new(XKB_CONTEXT_NO_FLAGS)};
	if (!xkb_ctx) {
		XR_LOG_ERR("Failed to get XKB context!");
	}

	const int32_t device_id = xkb_x11_get_core_keyboard_device_id(connection);
	XR_LOG_INFO("Core keyboard id {}", device_id);

	detail::unique_xkb_keyman keymap{
		xkb_x11_keymap_new_from_device(raw_ptr(xkb_ctx), connection, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS)
	};
	if (!keymap) {
		XR_LOG_ERR("Failed to get XKB keymap!");
	}

	detail::unique_xkb_state state{xkb_x11_state_new_from_device(raw_ptr(keymap), connection, device_id)};

	const auto main_screen_info = get_primary_screen_info(raw_ptr(_display));
	if (!main_screen_info) {
		XR_LOG_ERR("Failed to get primary screen info!");
		return tl::make_unexpected(PlatformWindowError{});
	}

	const int32_t _default_screen = XDefaultScreen(raw_ptr(_display));

	pod_zero<XVisualInfo> xvisual_info{};
	xvisual_info.screen = _default_screen;

	int32_t visuals_count{};
	unique_pointer<XVisualInfo, decltype(&XFree)> visual_info{
		XGetVisualInfo(raw_ptr(_display), VisualScreenMask, &xvisual_info, &visuals_count), &XFree
	};

	if (!visuals_count) {
		XR_LOG_ERR("Failed to query visuals!");
		return tl::make_unexpected(PlatformWindowError{});
	}

	const Window root_window = RootWindow(raw_ptr(_display), _default_screen);
	const VisualID _visualid = visual_info->visualid;

	pod_zero<XSetWindowAttributes> window_attribs;
	window_attribs.override_redirect = win_params.grab_input ? True : False;
	window_attribs.background_pixel	 = WhitePixel(raw_ptr(_display), _default_screen);
	window_attribs.colormap			 = XCreateColormap(raw_ptr(_display), root_window, visual_info->visual, AllocNone);
	window_attribs.event_mask		 = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
								PointerMotionMask | ExposureMask | StructureNotifyMask | LeaveWindowMask |
								EnterWindowMask | SubstructureNotifyMask;

	const auto msi = main_screen_info.value();

	x11_unique_window _window{
		XCreateWindow(
			raw_ptr(_display),
			root_window,
			msi.x_org,
			msi.y_org,
			static_cast<unsigned int>(msi.width),
			static_cast<unsigned int>(msi.height),
			0,
			visual_info->depth,
			InputOutput,
			visual_info->visual,
			CWEventMask | CWColormap | CWBackPixel | CWOverrideRedirect,
			&window_attribs
		),
		x11_window_deleter{
			raw_ptr(_display),
		},
	};

	if (!_window) {
		XR_LOG_ERR("Failed to create window !");
		return tl::make_unexpected(PlatformWindowError{});
	}

	//
	//  set window properties
	{
		unique_pointer<XSizeHints, decltype(&XFree)> size_hints{XAllocSizeHints(), &XFree};
		size_hints->flags		= PMinSize | PBaseSize;
		size_hints->min_width	= 1024;
		size_hints->min_height	= 1024;
		size_hints->base_width	= msi.width;
		size_hints->base_height = msi.height;

		unique_pointer<XWMHints, decltype(&XFree)> wm_hints{XAllocWMHints(), &XFree};
		wm_hints->flags			= StateHint | InputHint;
		wm_hints->initial_state = NormalState;
		wm_hints->input			= True;

		XTextProperty wnd_name;
		if (!XStringListToTextProperty((char**)&win_params.title, 1, &wnd_name)) {
			XR_LOG_ERR("XStringListToTextProperty failed!");
			return tl::make_unexpected(PlatformWindowError{});
		}

		XTextProperty icon_name;
		if (!XStringListToTextProperty((char**)&win_params.title, 1, &icon_name)) {
			XR_LOG_ERR("XStringListToTextProperty failed!");
			return tl::make_unexpected(PlatformWindowError{});
		}

		XSetWMProperties(
			raw_ptr(_display),
			raw_ptr(_window),
			&wnd_name,
			&icon_name,
			nullptr,
			0,
			raw_ptr(size_hints),
			raw_ptr(wm_hints),
			nullptr
		);
	}

	Atom _window_delete_atom = XInternAtom(raw_ptr(_display), "WM_DELETE_WINDOW", False);
	XSetWMProtocols(raw_ptr(_display), raw_ptr(_window), &_window_delete_atom, 1);

	const Atom window_atoms[] = {
		XInternAtom(raw_ptr(_display), "_NET_WM_STATE", False),
		XInternAtom(raw_ptr(_display), "_NET_WM_STATE_MAXIMIZED_VERT", False),
		XInternAtom(raw_ptr(_display), "_NET_WM_STATE_MAXIMIZED_HORZ", False),
		XInternAtom(raw_ptr(_display), "_NET_WM_STATE_SKIP_PAGER", False),
		XInternAtom(raw_ptr(_display), "_NET_WM_STATE_ABOVE", False),
		XInternAtom(raw_ptr(_display), "_NET_WM_STATE_FULLSCREEN", False),
	};

	XChangeProperty(
		raw_ptr(_display),
		raw_ptr(_window),
		window_atoms[0],
		XA_ATOM,
		32,
		PropModeReplace,
		(unsigned char*)(&window_atoms[1]),
		XR_I32_COUNTOF(window_atoms) - 1
	);

	const Atom name_atoms[] = {
		XInternAtom(raw_ptr(_display), "_NET_WM_NAME", False),
		XInternAtom(raw_ptr(_display), "UTF8_STRING", False),
	};

	XClearWindow(raw_ptr(_display), raw_ptr(_window));
	XMapRaised(raw_ptr(_display), raw_ptr(_window));
	XMoveWindow(raw_ptr(_display), raw_ptr(_window), msi.x_org, msi.y_org);

	bool _kb_grabbed	  = false;
	bool _pointer_grabbed = false;
	if (win_params.grab_input) {
		_kb_grabbed =
			XGrabKeyboard(raw_ptr(_display), raw_ptr(_window), False, GrabModeAsync, GrabModeAsync, CurrentTime) ==
			GrabSuccess;

		if (!_kb_grabbed) {
			XR_LOG_WARN("Failed to grab keyboard!");
		}

		_pointer_grabbed = XGrabPointer(
							   raw_ptr(_display),
							   raw_ptr(_window),
							   False,
							   ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask |
								   EnterWindowMask | LeaveWindowMask,
							   GrabModeAsync,
							   GrabModeAsync,
							   raw_ptr(_window),
							   None,
							   CurrentTime
						   ) == GrabSuccess;

		if (!_pointer_grabbed) {
			XR_LOG_WARN("Failed to grab mouse pointer!");
		}
	}

	XFlush(raw_ptr(_display));

	Window root_wnd{};
	int32_t x_location{};
	int32_t y_location{};
	uint32_t width{};
	uint32_t height{};
	uint32_t border_width{};
	uint32_t depth{};

	XGetGeometry(
		raw_ptr(_display), raw_ptr(_window), &root_wnd, &x_location, &y_location, &width, &height, &border_width, &depth
	);

	const uint32_t mod_shift = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_SHIFT);
	const uint32_t mod_caps	 = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_CAPS);
	const uint32_t mod_alt	 = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_ALT);
	const uint32_t mod_ctrl	 = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_CTRL);
	const uint32_t mod_num	 = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_NUM);
	const uint32_t mod_logo	 = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_LOGO);

	XR_LOG_INFO(
		"Window [{} x {}], border size {}, depth {}, screen position ({} x {})",
		width,
		height,
		border_width,
		depth,
		x_location,
		y_location
	);

	auto polled_state = detail::PolledDevicesState::create(raw_ptr(_display));
	if (!polled_state) {
		XR_LOG_ERR("Failed to query polled devices, error {}", polled_state.error().message());
		return tl::make_unexpected(PlatformWindowError{});
	}

	return tl::expected<PlatformWindow, PlatformWindowError>{
		tl::in_place,
		xray::base::make_unique<PlatformImpl>(
			std::move(_display),
			std::move(_window),
			_visualid,
			_window_delete_atom,
			_default_screen,
			static_cast<int32_t>(width),
			static_cast<int32_t>(height),
			_kb_grabbed,
			_pointer_grabbed,
			XInputHelper{
				connection,
				std::move(xkb_ctx),
				std::move(keymap),
				std::move(state),
				xkb_event_base,
				xkb_error_base,
				XInputHelper::KbMods{
					.shift_mod = mod_shift,
					.caps_mod  = mod_caps,
					.ctrl_mod  = mod_ctrl,
					.alt_mod   = mod_alt,
					.num_mod   = mod_num,
					.logo_mod  = mod_logo
				},
			},
			std::move(*polled_state)
		),
	};
}

xray::ui::PlatformWindow::PlatformWindow(PlatformWindow&& rhs)
	: core{std::move(rhs.core)}, _platform{std::move(rhs._platform)} {
	// rhs._platform->_kb_grabbed		= false;
	// rhs._platform->_pointer_grabbed = false;
}

xray::ui::PlatformWindow::PlatformWindow(xray::base::unique_pointer<PlatformImpl> impl) : _platform{std::move(impl)} {}

xray::ui::PlatformWindow::~PlatformWindow() {
	if (_platform) {
		if (_platform->_kb_grabbed) {
			XUngrabKeyboard(raw_ptr(_platform->_display), CurrentTime);
		}

		if (_platform->_pointer_grabbed) {
			XUngrabPointer(raw_ptr(_platform->_display), CurrentTime);
		}
	}
}

xray::rendering::WindowPlatformDataXlib xray::ui::PlatformWindow::platform_data() const noexcept {
	return xray::rendering::WindowPlatformDataXlib{
		.display = reinterpret_cast<uintptr_t>(raw_ptr(_platform->_display)),
		.window	 = static_cast<uintptr_t>(raw_ptr(_platform->_window)),
		.visual	 = static_cast<uintptr_t>(_platform->_visualid),
		.width	 = static_cast<uint32_t>(_platform->_wnd_width),
		.height	 = static_cast<uint32_t>(_platform->_wnd_height),
	};
}

void xray::ui::PlatformWindow::message_loop() {
	assert(valid());

	while (_platform->_quit_flag == 0) {
		core.events.poll_start(poll_start_event{});

		epoll_event queued_events[32];
		const int32_t events_count =
			syscall_wrapper(epoll_wait, _platform->_poll_state._epoll_fd, queued_events, 32, 0);

		if (events_count == -1) {
			XR_LOG_CRITICAL("epoll_wait error {}", errno);
		} else {
			for (int32_t event = 0; event < events_count; ++event) {
				const epoll_event& e = queued_events[event];

				//
				// handle X11 event
				if (e.data.fd == _platform->_poll_state._x11_display_fd) {
					while ((_platform->_quit_flag == false) &&
						   XEventsQueued(raw_ptr(_platform->_display), QueuedAfterFlush)) {
						XEvent window_event;
						XNextEvent(raw_ptr(_platform->_display), &window_event);

						if (window_event.type == _platform->_input_helper.xkb_event_base) {
							const XkbEvent* xkb_evt = reinterpret_cast<const XkbEvent*>(&window_event);
							if (xkb_evt->any.xkb_type == XkbStateNotify) {
								auto get_xkb_mod_mask_fn = [xi = &_platform->_input_helper](const uint32_t in) {
									uint32_t ret = 0;
									if ((in & ShiftMask) && xi->mod_index.shift_mod != XKB_MOD_INVALID)
										ret |= (1 << xi->mod_index.shift_mod);
									if ((in & LockMask) && xi->mod_index.caps_mod != XKB_MOD_INVALID)
										ret |= (1 << xi->mod_index.caps_mod);
									if ((in & ControlMask) && xi->mod_index.ctrl_mod != XKB_MOD_INVALID)
										ret |= (1 << xi->mod_index.ctrl_mod);
									if ((in & Mod1Mask) && xi->mod_index.alt_mod != XKB_MOD_INVALID)
										ret |= (1 << xi->mod_index.alt_mod);
									if ((in & Mod2Mask) && xi->mod_index.num_mod != XKB_MOD_INVALID)
										ret |= (1 << xi->mod_index.num_mod);

									// mod3 - scroll lock, don’t need it for now
									// if ((in & Mod3Mask) && xi->mod_index.mod3_mod != XKB_MOD_INVALID)
									// 	ret |= (1 << xi->mod_index.mod3_mod);

									if ((in & Mod4Mask) && xi->mod_index.logo_mod != XKB_MOD_INVALID)
										ret |= (1 << xi->mod_index.logo_mod);

									// what is mod5 ??!!
									// if ((in & Mod5Mask) && xi->mod_index.mod5_mod != XKB_MOD_INVALID)
									// ret |= (1 << xi->mod_index.mod5_mod);

									return ret;
								};

								// adapted from here
								// https://coral.googlesource.com/weston-imx/+/refs/heads/master/libweston/compositor-x11.c

								xkb_state_update_mask(
									raw_ptr(_platform->_input_helper.xkb_state),
									get_xkb_mod_mask_fn(xkb_evt->state.base_mods),
									get_xkb_mod_mask_fn(xkb_evt->state.latched_mods),
									get_xkb_mod_mask_fn(xkb_evt->state.locked_mods),
									0,
									0,
									xkb_evt->state.group
								);
							}
							continue;
						}

						if ((window_event.type == ButtonPress) || (window_event.type == ButtonRelease)) {
							_platform->event_mouse_button(&window_event.xbutton, this);
							continue;
						}

						if (window_event.type == MotionNotify) {
							_platform->event_motion_notify(&window_event.xmotion, this);
							continue;
						}

						if (window_event.type == ClientMessage) {
							if (const PlatformImpl::ClientMessageResult_t msg_result =
									_platform->event_client_message(&window_event.xclient);
								msg_result == PlatformImpl::ClientMessageResult_t::Quit) {
								_platform->_quit_flag = true;
							}
							continue;
						}

						if ((window_event.type == KeyPress) || (window_event.type == KeyRelease)) {
							_platform->event_key(&window_event.xkey, this);
							continue;
						}

						if (window_event.type == ConfigureNotify) {
							_platform->event_configure(&window_event.xconfigure, this);
						}
					}
					continue;
				}

				//
				// Find if any gamepad has data to be read
				auto itr_gamepad =
					ranges::find_if(_platform->_poll_state._gamepads, [&](const PlatformGamepad& gamepad) {
						return e.data.fd == gamepad._fd;
					});

				if (itr_gamepad != std::cend(_platform->_poll_state._gamepads)) {
					//
					// drain all events
					input_event gamepad_events[8];
					for (;;) {
						const ssize_t bytes_read =
							syscall_wrapper(read, itr_gamepad->_fd, gamepad_events, sizeof(gamepad_events));

						//
						// nothing to read
						if (bytes_read <= 0) {
							if (errno != EAGAIN) {
								XR_LOG_ERR(
									"Failed to read from gamepad {}, error {}",
									itr_gamepad->_device_path.generic_string(),
									errno
								);
							}
							break;
						}

						//
						// translate and forward
						for (ssize_t i = 0; i < (bytes_read / sizeof(input_event)); ++i) {
							const input_event& ine = gamepad_events[i];
							if (const auto [valid, win_event] = itr_gamepad->make_window_event(gamepad_events[i]);
								valid) {
								core.events.window(win_event);
							}
						}
					}

					continue;
				}

				//
				// handle any other events here registered with the epoll fd
			}
		}

		core.events.poll_end(poll_end_event{});

		//
		// user loop
		core.events.loop(window_loop_event{
			_platform->_wnd_width,
			_platform->_wnd_height,
			this,
		});
	}
}

void xray::ui::PlatformWindow::quit() noexcept { _platform->_quit_flag = true; }

std::span<const xray::ui::GamepadAxisInfo> xray::ui::PlatformWindow::gamepad_axis_info() const noexcept {
	if (_platform->_poll_state._gamepads.empty()) {
		return {};
	} else {
		return std::span{_platform->_poll_state._gamepads[0]._axis_info};
	}
}

int32_t xray::ui::PlatformWindow::width() const noexcept { return _platform->_wnd_width; }
int32_t xray::ui::PlatformWindow::height() const noexcept { return _platform->_wnd_height; }
