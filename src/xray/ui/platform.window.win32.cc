#include "xray/ui/platform.window.hpp"

#include <GameInput.h>
#include <windowsx.h>
#include <winrt/base.h>

#include <tl/optional.hpp>

#include "xray/base/array_dimension.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/events.gamepad.hpp"
#include "xray/ui/events.pretty.print.hpp"

using namespace xray::base;
using namespace xray::ui;
using namespace std;

static constexpr auto kWindowClassName = "__@@##!!__Windows_XRAY__!!##@@__";

static constexpr const xray::ui::KeySymbol WIN32_KEYS_MAPPING_TABLE[] = {
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
	KeySymbol::unknown,
	KeySymbol::clear,
	KeySymbol::enter,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::pause,
	KeySymbol::unknown,
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
	KeySymbol::space,
	KeySymbol::page_up,
	KeySymbol::page_down,
	KeySymbol::end,
	KeySymbol::home,
	KeySymbol::left,
	KeySymbol::up,
	KeySymbol::right,
	KeySymbol::down,
	KeySymbol::select,
	KeySymbol::print_screen,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::insert,
	KeySymbol::del,
	KeySymbol::unknown,
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
	KeySymbol::left_win,
	KeySymbol::right_win,
	KeySymbol::unknown,
	KeySymbol::unknown,
	KeySymbol::unknown,
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
	KeySymbol::kp_multiply,
	KeySymbol::kp_add,
	KeySymbol::unknown,
	KeySymbol::kp_minus,
	KeySymbol::unknown,
	KeySymbol::kp_divide,
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
	KeySymbol ::f11,
	KeySymbol ::f12,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::scrol_lock,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::left_shift,
	KeySymbol ::right_shift,
	KeySymbol ::left_control,
	KeySymbol ::right_control,
	KeySymbol ::left_menu,
	KeySymbol ::right_menu,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
	KeySymbol ::unknown,
};

static KeySymbol map_key(const int32_t key_code) {
	if (key_code == VK_MENU) {
		return KeySymbol ::unknown;
	}

	if (key_code < XR_I32_COUNTOF(WIN32_KEYS_MAPPING_TABLE)) {
		return WIN32_KEYS_MAPPING_TABLE[key_code];
	}

	XR_LOG_INFO("Unmaped key code {}", key_code);
	return KeySymbol ::unknown;
}

static POINT get_primary_monitor_resolution() {
	auto primaryMonitor = MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY);
	if (!primaryMonitor) return {1024, 1024};

	MONITORINFO monInfo;
	monInfo.cbSize = sizeof(monInfo);
	if (!GetMonitorInfo(primaryMonitor, &monInfo)) {
		return {1024, 1024};
	}

	return {std::abs(monInfo.rcWork.right - monInfo.rcWork.left), std::abs(monInfo.rcWork.bottom - monInfo.rcWork.top)};
}

struct xray::ui::PlatformWindow::PlatformImpl {
	HWND _window{nullptr};
	int32_t _wnd_width{-1};
	int32_t _wnd_height{-1};
	winrt::com_ptr<IGameInput> input_interface{nullptr};
	winrt::com_ptr<IGameInputDevice> input_device{nullptr};
	winrt::com_ptr<IGameInputReading> prev_reading{nullptr};
	PlatformWindow* platform_window{nullptr};
	bool _quit_flag{false};

	static LRESULT WINAPI window_proc_stub(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT window_proc(UINT message, WPARAM wparam, LPARAM lparam);
	void event_mouse_button(const uint32_t type, const WPARAM wp, const LPARAM lp);
	void event_mouse_wheel(const WPARAM wparam, const LPARAM lparam);
	void event_key(const uint32_t type, const WPARAM wp, const LPARAM lp);
	void event_motion_notify(const WPARAM wparam, const LPARAM lparam);
	void event_configure(const WPARAM wparam, const LPARAM lparam);
};

LRESULT WINAPI
xray::ui::PlatformWindow::PlatformImpl::window_proc_stub(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	auto obj = reinterpret_cast<PlatformWindow::PlatformImpl*>(GetWindowLongPtr(wnd, GWLP_USERDATA));
	return obj ? obj->window_proc(msg, wparam, lparam) : DefWindowProc(wnd, msg, wparam, lparam);
}

LRESULT xray::ui::PlatformWindow::PlatformImpl::window_proc(UINT message, WPARAM wparam, LPARAM lparam) {
	tl::optional<LRESULT> msg_result{0L};

	switch (message) {
		case WM_CLOSE:
			DestroyWindow(_window);
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			event_mouse_button(message, wparam, lparam);
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
			event_key(message, wparam, lparam);
			break;

		case WM_MOUSEWHEEL:
			event_mouse_wheel(wparam, lparam);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_MOUSEMOVE:
			event_motion_notify(wparam, lparam);
			break;

		case WM_SIZE:
			event_configure(wparam, lparam);
			break;

		default:
			msg_result = tl::nullopt;
			break;
	}

	if (msg_result) {
		return *msg_result;
	}

	return DefWindowProc(_window, message, wparam, lparam);
}

void xray::ui::PlatformWindow::PlatformImpl::event_mouse_button(const uint32_t type, const WPARAM wp, const LPARAM lp) {
	mouse_button_event mbe;

	mbe.type = (type == WM_LBUTTONDOWN || type == WM_RBUTTONDOWN || type == WM_MBUTTONDOWN || type == WM_XBUTTONDOWN)
				   ? event_action_type::press
				   : event_action_type::release;

	auto fn_translate_button = [](const uint32_t b, const WPARAM wp) {
		switch (b) {
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
				return mouse_button::button1;

			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
				return mouse_button::button3;

			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
				return mouse_button::button2;

			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP: {
				const auto id = HIWORD(wp);
				return id == XBUTTON1 ? mouse_button::button4 : mouse_button::button5;
			}

			default:
				return mouse_button::button1;
		}
	};

	mbe.wnd		  = platform_window;
	mbe.pointer_x = GET_X_LPARAM(lp);
	mbe.pointer_y = GET_Y_LPARAM(lp);
	mbe.button	  = fn_translate_button(type, wp);
	mbe.button1	  = (wp & MK_LBUTTON) != 0;
	mbe.button2	  = (wp & MK_MBUTTON) != 0;
	mbe.button3	  = (wp & MK_RBUTTON) != 0;
	mbe.button4	  = (wp & MK_XBUTTON1) != 0;
	mbe.button5	  = (wp & MK_XBUTTON2) != 0;
	mbe.shift	  = (wp & MK_SHIFT) != 0;
	mbe.control	  = (wp & MK_CONTROL) != 0;

	window_event we;
	we.type			= event_type::mouse_button;
	we.event.button = mbe;

	platform_window->core.events.window(we);
}

void xray::ui::PlatformWindow::PlatformImpl::event_mouse_wheel(const WPARAM wparam, const LPARAM lparam) {
	XR_LOG_INFO("Wheel event!");
	mouse_wheel_event mwe;
	mwe.delta	  = GET_WHEEL_DELTA_WPARAM(wparam) < 0 ? +1 : -1;
	mwe.fdelta	  = GET_WHEEL_DELTA_WPARAM(wparam) / 120.0f;
	mwe.wnd		  = platform_window;
	mwe.pointer_x = GET_X_LPARAM(lparam);
	mwe.pointer_y = GET_Y_LPARAM(lparam);
	mwe.button1	  = (wparam & MK_LBUTTON) != 0;
	mwe.button2	  = (wparam & MK_MBUTTON) != 0;
	mwe.button3	  = (wparam & MK_RBUTTON) != 0;
	mwe.button4	  = (wparam & MK_XBUTTON1) != 0;
	mwe.button5	  = (wparam & MK_XBUTTON2) != 0;
	mwe.shift	  = (wparam & MK_SHIFT) != 0;
	mwe.control	  = (wparam & MK_CONTROL) != 0;

	window_event we;
	we.type		   = event_type::mouse_wheel;
	we.event.wheel = mwe;

	platform_window->core.events.window(we);
}

void xray::ui::PlatformWindow::PlatformImpl::event_key(const uint32_t type, const WPARAM wp, const LPARAM lp) {
	key_event ke;
	ke.wnd	= platform_window;
	ke.type = (type == WM_KEYDOWN ? event_action_type::press : event_action_type::release);
	memset(ke.name, 0, sizeof(ke.name));

	const auto cursor_pos = [w = _window]() {
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(w, &pt);
		return pt;
	}();

	ke.keycode	 = map_key(wp);
	ke.pointer_x = cursor_pos.x;
	ke.pointer_y = cursor_pos.y;
	// ke.button1 = (GetAsyncKeyState(VK_LBUTTON) & (1 << 15)) != 0;
	// ke.button2 = (GetAsyncKeyState(VK_MBUTTON) & (1 << 15)) != 0;
	// ke.button3 = (GetAsyncKeyState(VK_RBUTTON) & (1 << 15)) != 0;
	// ke.button4 = (GetAsyncKeyState(VK_XBUTTON1) & (1 << 15)) != 0;
	// ke.button5 = (GetAsyncKeyState(VK_XBUTTON2) & (1 << 15)) != 0;
	ke.shift   = (GetAsyncKeyState(VK_SHIFT) & (1 << 15)) != 0;
	ke.control = (GetAsyncKeyState(VK_CONTROL) & (1 << 15)) != 0;

	GetKeyNameText(lp, ke.name, XR_I32_COUNTOF(ke.name));

	window_event we;
	we.type		 = event_type::key;
	we.event.key = ke;

	platform_window->core.events.window(we);
}

void xray::ui::PlatformWindow::PlatformImpl::event_motion_notify(const WPARAM wparam, const LPARAM lparam) {
	mouse_motion_event mme;
	mme.wnd		  = platform_window;
	mme.pointer_x = GET_X_LPARAM(lparam);
	mme.pointer_y = GET_Y_LPARAM(lparam);
	mme.button1	  = (wparam & MK_LBUTTON) != 0;
	mme.button2	  = (wparam & MK_MBUTTON) != 0;
	mme.button3	  = (wparam & MK_RBUTTON) != 0;
	mme.button4	  = (wparam & MK_XBUTTON1) != 0;
	mme.button5	  = (wparam & MK_XBUTTON2) != 0;
	mme.shift	  = (wparam & MK_SHIFT) != 0;
	mme.control	  = (wparam & MK_CONTROL) != 0;

	window_event we;
	we.type			= event_type::mouse_motion;
	we.event.motion = mme;

	platform_window->core.events.window(we);
}

void xray::ui::PlatformWindow::PlatformImpl::event_configure(const WPARAM wparam, const LPARAM lparam) {
	if ((wparam != SIZE_MAXIMIZED) || (wparam != SIZE_RESTORED)) {
		return;
	}

	_wnd_width	= GET_X_LPARAM(lparam);
	_wnd_height = GET_Y_LPARAM(lparam);

	window_configure_event cfg_evt;
	cfg_evt.width  = _wnd_width;
	cfg_evt.height = _wnd_height;
	cfg_evt.wnd	   = platform_window;

	window_event we;
	we.type			   = event_type::configure;
	we.event.configure = cfg_evt;

	platform_window->core.events.window(we);
}

xray::ui::PlatformWindow::PlatformWindow(xray::base::unique_pointer<PlatformImpl> impl) : _platform{std::move(impl)} {}

xray::ui::PlatformWindow::~PlatformWindow() {}

xray::ui::PlatformWindow::PlatformWindow(PlatformWindow&& other) noexcept
	: core{std::move(other.core)}, _platform{std::move(other._platform)} {}

tl::expected<xray::ui::PlatformWindow, xray::ui::PlatformWindowError> xray::ui::PlatformWindow::create(
	const window_params_t& wp
) {
	const WNDCLASSEX wndClass = {
		sizeof(wndClass),
		CS_OWNDC,
		&PlatformWindow::PlatformImpl::window_proc_stub,
		0,
		0,
		GetModuleHandle(nullptr),
		LoadIcon(nullptr, IDI_APPLICATION),
		LoadCursor(nullptr, IDC_ARROW),
		reinterpret_cast<HBRUSH>(GetStockObject(COLOR_WINDOW + 1)),
		nullptr,
		kWindowClassName,
		nullptr,
	};

	if (!RegisterClassEx(&wndClass)) {
		XR_LOG_CRITICAL("Failed to register real window class!");
		return tl::make_unexpected(PlatformWindowError{});
	}

	constexpr auto kWindowStyle = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	const auto monSize			= get_primary_monitor_resolution();
	RECT windowGeom{0, 0, monSize.x, monSize.y};
	AdjustWindowRectEx(&windowGeom, kWindowStyle, false, 0L);

	HWND window = CreateWindowEx(
		0,
		kWindowClassName,
		wp.title,
		kWindowStyle,
		windowGeom.left,
		windowGeom.top,
		windowGeom.right - windowGeom.left,
		windowGeom.bottom - windowGeom.top,
		nullptr,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr
	);

	if (!window) {
		XR_LOG_CRITICAL("Failed to create main window !");
		return tl::make_unexpected(PlatformWindowError{});
	}

	winrt::com_ptr<IGameInput> input_interface{};
	const HRESULT result = ::GameInputCreate(input_interface.put());
	if (!SUCCEEDED(result)) {
		return tl::make_unexpected(PlatformWindowError{});
	}

	ShowWindow(window, SW_SHOWNORMAL);
	UpdateWindow(window);

	RECT rc;
	GetClientRect(window, &rc);

	return tl::expected<PlatformWindow, PlatformWindowError>{
		tl::in_place,
		base::make_unique<PlatformImpl>(
			window,
			static_cast<int32_t>(std::abs(rc.right - rc.left)),
			static_cast<int32_t>(std::abs(rc.bottom - rc.top)),
			input_interface
		),
	};
}

xray::rendering::WindowPlatformData xray::ui::PlatformWindow::platform_data() const noexcept {
	return xray::rendering::WindowPlatformDataWin32{
		.module = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr)),
		.window = reinterpret_cast<uintptr_t>(_platform->_window),
		.width	= static_cast<uint32_t>(_platform->_wnd_width),
		.height = static_cast<uint32_t>(_platform->_wnd_height),
	};
}

void xray::ui::PlatformWindow::disable_cursor() noexcept {
	// TODO: implement this
}

void xray::ui::PlatformWindow::enable_cursor() noexcept {
	// TODO: implement this
}

std::span<const xray::ui::GamepadAxisInfo> xray::ui::PlatformWindow::gamepad_axis_info() const noexcept { return {}; }

void xray::ui::PlatformWindow::message_loop() {
	_platform->platform_window = this;
	SetWindowLongPtr(_platform->_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(raw_ptr(_platform)));

	pod_zero<MSG> wnd_msg;

	for (; _platform->_quit_flag == false;) {
		core.events.poll_start(poll_start_event{});

		while (PeekMessage(&wnd_msg, nullptr, 0, 0, PM_NOREMOVE) && !_platform->_quit_flag) {
			const auto res = GetMessage(&wnd_msg, nullptr, 0, 0);
			if (res <= 0) {
				_platform->_quit_flag = true;
				break;
			}

			TranslateMessage(&wnd_msg);
			DispatchMessage(&wnd_msg);
		}

		if (!_platform->prev_reading) {
			if (const HRESULT res = _platform->input_interface->GetCurrentReading(
					GameInputKindGamepad, _platform->input_device.get(), _platform->prev_reading.put()
				);
				SUCCEEDED(res)) {
				_platform->prev_reading->GetDevice(_platform->input_device.put());

				GameInputGamepadState gamepad_state;
				_platform->prev_reading->GetGamepadState(&gamepad_state);
			}
		} else {
			winrt::com_ptr<IGameInputReading> next_reading;
			const HRESULT res = _platform->input_interface->GetNextReading(
				_platform->prev_reading.get(), GameInputKindGamepad, _platform->input_device.get(), next_reading.put()
			);
			if (SUCCEEDED(res)) {
				_platform->prev_reading = next_reading;
				//
				// process
			} else {
				if (res != GAMEINPUT_E_READING_NOT_FOUND) {
					_platform->input_device = nullptr;
					_platform->prev_reading = nullptr;
				}
			}
		}

		core.events.poll_end(poll_end_event{});

		//
		// user loop event
		core.events.loop(window_loop_event{_platform->_wnd_width, _platform->_wnd_height, this});
	}
}

int32_t xray::ui::PlatformWindow::width() const noexcept { return _platform->_wnd_width; }
int32_t xray::ui::PlatformWindow::height() const noexcept { return _platform->_wnd_height; }
void xray::ui::PlatformWindow::quit() noexcept { _platform->_quit_flag = true; }
