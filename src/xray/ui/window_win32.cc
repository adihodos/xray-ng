#if defined(XRAY_RENDERER_OPENGL)
#include "opengl/opengl.hpp"
#include "opengl/wglext.h"
#endif

#include "xray/base/array_dimension.hpp"
#include "xray/base/containers/fixed_vector.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/window_win32.hpp"
#include <windowsx.h>

using namespace xray::base;
using namespace xray::ui;
using namespace xray::ui::detail;
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

static KeySymbol
map_key(const int32_t key_code)
{
    if (key_code == VK_MENU) {
        return KeySymbol ::unknown;
    }

    if (key_code < XR_I32_COUNTOF(WIN32_KEYS_MAPPING_TABLE)) {
        return WIN32_KEYS_MAPPING_TABLE[key_code];
    }

    XR_LOG_INFO("Unmaped key code {}", key_code);
    return KeySymbol ::unknown;
}

static POINT
get_primary_monitor_resolution()
{
    auto primaryMonitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    if (!primaryMonitor)
        return { 1024, 1024 };

    MONITORINFO monInfo;
    monInfo.cbSize = sizeof(monInfo);
    if (!GetMonitorInfo(primaryMonitor, &monInfo)) {
        return { 1024, 1024 };
    }

    return { std::abs(monInfo.rcWork.right - monInfo.rcWork.left),
             std::abs(monInfo.rcWork.bottom - monInfo.rcWork.top) };
}

#if defined(XRAY_RENDERER_OPENGL)
///
/// Pointers to WGL extensions
struct wgl
{
    static PFNWGLCHOOSEPIXELFORMATARBPROC ChoosePixelFormatARB;
    static PFNWGLCREATECONTEXTATTRIBSARBPROC CreateContextAttribsARB;
    static PFNWGLSWAPINTERVALEXTPROC SwapIntervalEXT;
};

PFNWGLCHOOSEPIXELFORMATARBPROC wgl::ChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wgl::CreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC wgl::SwapIntervalEXT;

#endif /* defined XRAY_RENDERER_OPENGL */

xray::ui::window::window(const window_params_t& wparam)
{
    initialize(&wparam);
}

xray::ui::window::~window() {}

xray::ui::window::window(window&& other) noexcept
    : core{ std::move(other.core) }
    , _window{ other._window }
#if defined(XRAY_RENDERER_OPENGL)
    , _pixelformat_id{ other._pixelformat_id }
    , _window_dc{ std::move(other._window_dc) }
    , _glcontext{ std::move(other._glcontext) }
#endif
    , _wnd_width{ other._wnd_width }
    , _wnd_height{ other._wnd_height }
    , _quit_flag{ other._quit_flag.load() }
{
    other._window = nullptr;
    SetWindowLongPtr(_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void
xray::ui::window::initialize(const window_params_t* wp)
{
#if defined(XRAY_RENDERER_OPENGL)
    if (_pixelformat_id == 0) {
        constexpr const char* const kTempWindowClassName = "__##@@_TempOpenGLWindow";

        const WNDCLASSEX wndClass = {
            sizeof(wndClass),
            CS_OWNDC,
            DefWindowProc,
            0,
            0,
            GetModuleHandle(nullptr),
            LoadIcon(nullptr, IDI_APPLICATION),
            LoadCursor(nullptr, IDC_ARROW),
            (HBRUSH)GetStockObject(COLOR_WINDOW + 1),
            nullptr,
            kTempWindowClassName,
            nullptr,
        };

        //
        // First call, so just create a dummy window and a dummy context to
        // get a suitable pixel format
        RegisterClassEx(&wndClass);
        HWND tempWindow = CreateWindowEx(0L,
                                         kTempWindowClassName,
                                         "TempOpenGL Window",
                                         WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         CW_USEDEFAULT,
                                         nullptr,
                                         nullptr,
                                         GetModuleHandle(nullptr),
                                         this);

        if (!tempWindow) {
            XR_LOG_CRITICAL("Failed to create temporary window!");
            return;
        }

        {
            scoped_window_dc devContext{ GetDC(tempWindow), hdc_deleter{ tempWindow } };

            const auto pfd = [wp]() {
                PIXELFORMATDESCRIPTOR pfd = { 0 };
                pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
                pfd.nVersion = 1;
                pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
                pfd.iPixelType = PFD_TYPE_RGBA;
                pfd.cColorBits = (BYTE)wp->color_bits;
                pfd.cDepthBits = (BYTE)wp->depth_bits;
                pfd.cStencilBits = (BYTE)wp->stencil_bits;
                pfd.iLayerType = PFD_MAIN_PLANE;

                return pfd;
            }();

            const auto pixelFormat = ChoosePixelFormat(raw_ptr(devContext), &pfd);
            if (pixelFormat == 0) {
                XR_LOG_CRITICAL("Failed to find suitable pixel format!");
                return;
            }

            if (!SetPixelFormat(raw_ptr(devContext), pixelFormat, &pfd)) {
                XR_LOG_CRITICAL("Failed to set new pixel format for temporary window!");
                return;
            }

            scoped_opengl_context tempGLContext{ wglCreateContext(raw_ptr(devContext)) };

            if (!tempGLContext) {
                XR_LOG_CRITICAL("Failed to create initial OpenGL context!");
                return;
            }

            wglMakeCurrent(raw_ptr(devContext), raw_ptr(tempGLContext));

            wgl::ChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

            if (!wgl::ChoosePixelFormatARB) {
                XR_LOG_CRITICAL("wglChoosePixelFormatARB not present!");
                return;
            }

            const int32_t desiredPixelFmtProperties[] = { WGL_DRAW_TO_WINDOW_ARB,
                                                          TRUE,
                                                          /* Full acceleration */
                                                          WGL_ACCELERATION_ARB,
                                                          WGL_FULL_ACCELERATION_ARB,
                                                          /* OpenGL support */
                                                          WGL_SUPPORT_OPENGL_ARB,
                                                          TRUE,
                                                          /* Double buffered */
                                                          WGL_DOUBLE_BUFFER_ARB,
                                                          TRUE,
                                                          WGL_PIXEL_TYPE_ARB,
                                                          WGL_TYPE_RGBA_ARB,
                                                          WGL_COLOR_BITS_ARB,
                                                          wp->color_bits,
                                                          WGL_RED_BITS_ARB,
                                                          8,
                                                          WGL_GREEN_BITS_ARB,
                                                          8,
                                                          WGL_BLUE_BITS_ARB,
                                                          8,
                                                          WGL_ALPHA_BITS_ARB,
                                                          8,
                                                          WGL_DEPTH_BITS_ARB,
                                                          wp->depth_bits,
                                                          WGL_STENCIL_BITS_ARB,
                                                          wp->stencil_bits,
                                                          0 };

            uint32_t numFormats{};
            wgl::ChoosePixelFormatARB(
                raw_ptr(devContext), desiredPixelFmtProperties, nullptr, 1, &_pixelformat_id, &numFormats);

            if (numFormats == 0) {
                XR_LOG_CRITICAL("wglChoosePixelFormat found no suitable pixel format!");
                return;
            }
        }

        DestroyWindow(tempWindow);
        //
        //  Found a suitable pixel format, call again
        initialize(wp);
        return;
    }

    assert(_pixelformat_id != 0);

#endif /* defined XRAY_RENDERER_OPENGL */

    //
    // Since we have found a suitable pixel format, we will now create
    // the real window, where we will draw stuff.

    const WNDCLASSEX wndClass = {
        sizeof(wndClass),
        CS_OWNDC,
        &window::window_proc_stub,
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
        return;
    }

    constexpr auto kWindowStyle = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    const auto monSize = get_primary_monitor_resolution();
    RECT windowGeom{ 0, 0, monSize.x, monSize.y };
    AdjustWindowRectEx(&windowGeom, kWindowStyle, false, 0L);

    _window = CreateWindowEx(0,
                             kWindowClassName,
                             wp->title,
                             kWindowStyle,
                             windowGeom.left,
                             windowGeom.top,
                             windowGeom.right - windowGeom.left,
                             windowGeom.bottom - windowGeom.top,
                             nullptr,
                             nullptr,
                             GetModuleHandle(nullptr),
                             this);
    if (!_window) {
        XR_LOG_CRITICAL("Failed to create main window !");
        return;
    }

#if defined(XRAY_RENDERER_OPENGL)
    _window_dc = scoped_window_dc{ GetDC(_window), hdc_deleter{ _window } };

    PIXELFORMATDESCRIPTOR selectedPixelFormat;
    if (!DescribePixelFormat(
            raw_ptr(_window_dc), _pixelformat_id, (UINT)sizeof(selectedPixelFormat), &selectedPixelFormat)) {
        XR_LOG_CRITICAL("Failed to get description of pixel format {}", _pixelformat_id);
        return;
    }

    //
    //  set the new pixel format
    if (!SetPixelFormat(raw_ptr(_window_dc), _pixelformat_id, &selectedPixelFormat)) {
        XR_LOG_CRITICAL("Failed to set chosen pixel format!");
        return;
    }

    //
    //  Create another temp context
    {
        scoped_opengl_context tempGLContext{ wglCreateContext(raw_ptr(_window_dc)) };
        if (!tempGLContext) {
            XR_LOG_CRITICAL("Failed to create temp OpenGL context (2)");
            return;
        }

        wglMakeCurrent(raw_ptr(_window_dc), raw_ptr(tempGLContext));

        //
        // load wglCreateContextAttribsARB
        wgl::CreateContextAttribsARB =
            (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        if (!wgl::CreateContextAttribsARB) {
            XR_LOG_CRITICAL("wglCreateContextAttribsARB not found!");
            return;
        }

        fixed_vector<int32_t, 16u> contextAttribs;

        contextAttribs.push_back(WGL_CONTEXT_MAJOR_VERSION_ARB);
        contextAttribs.push_back(wp->ver_major);

        contextAttribs.push_back(WGL_CONTEXT_MINOR_VERSION_ARB);
        contextAttribs.push_back(wp->ver_minor);

        contextAttribs.push_back(WGL_CONTEXT_PROFILE_MASK_ARB);
        contextAttribs.push_back(WGL_CONTEXT_CORE_PROFILE_BIT_ARB);

        if (wp->debug_output_level != 0) {
            contextAttribs.push_back(WGL_CONTEXT_FLAGS_ARB);
            contextAttribs.push_back(WGL_CONTEXT_DEBUG_BIT_ARB);
        }

        if (wp->sample_count != 0) {
        }

        //
        // terminate attribute list
        contextAttribs.push_back(0);

        _glcontext =
            scoped_opengl_context{ wgl::CreateContextAttribsARB(raw_ptr(_window_dc), nullptr, contextAttribs.data()),
                                   hglrc_deleter{ raw_ptr(_window_dc) } };

        if (!_glcontext) {
            XR_LOG_CRITICAL("Failed to create REAL OPENGL context!");
            return;
        }
    }

    //
    //  Make new context current
    if (!wglMakeCurrent(raw_ptr(_window_dc), raw_ptr(_glcontext))) {
        XR_LOG_CRITICAL("Failed to set context as current OpenGL context !");
        return;
    }

    //
    // Load OpenGL functions
    if (!gl::sys::LoadFunctions()) {
        XR_LOG_CRITICAL("Failed to load OpenGL functions");
        _glcontext = {};
        return;
    }

#endif /* defined XRAY_RENDERER_OPENGL */

    ShowWindow(_window, SW_SHOWNORMAL);
    UpdateWindow(_window);

    {
        RECT rc;
        GetClientRect(_window, &rc);
        _wnd_width = rc.right - rc.left;
        _wnd_height = rc.bottom - rc.top;
    }
}

LRESULT WINAPI
xray::ui::window::window_proc_stub(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

    if (msg == WM_CREATE) {
        const auto ptr = (const CREATESTRUCT*)lparam;
        SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)ptr->lpCreateParams);
        return true;
    }

    auto obj = (window*)GetWindowLongPtr(wnd, GWLP_USERDATA);
    return obj ? obj->window_proc(msg, wparam, lparam) : DefWindowProc(wnd, msg, wparam, lparam);
}

LRESULT
xray::ui::window::window_proc(UINT message, WPARAM wparam, LPARAM lparam)
{
    maybe<LRESULT> msg_result{ 0L };

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
            msg_result = maybe<LRESULT>{ nothing{} };
            break;
    }

    if (msg_result) {
        return msg_result.value();
    }

    return DefWindowProc(_window, message, wparam, lparam);
}

void
xray::ui::window::disable_cursor() noexcept
{
}
void
xray::ui::window::enable_cursor() noexcept
{
}

void
xray::ui::window::message_loop()
{
    pod_zero<MSG> wnd_msg;

    for (; _quit_flag == false;) {

        core.events.poll_start(poll_start_event{});

        while (PeekMessage(&wnd_msg, nullptr, 0, 0, PM_NOREMOVE) && !_quit_flag) {
            const auto res = GetMessage(&wnd_msg, nullptr, 0, 0);
            if (res <= 0) {
                _quit_flag = true;
                break;
            }

            TranslateMessage(&wnd_msg);
            DispatchMessage(&wnd_msg);
        }

        core.events.poll_end(poll_end_event{});

        //
        // user loop event
        core.events.loop(window_loop_event{ _wnd_width, _wnd_height, this });

#if defined(XRAY_RENDERER_OPENGL)
        SwapBuffers(raw_ptr(_window_dc));
#endif
    }
}

void
xray::ui::window::event_mouse_button(const uint32_t type, const WPARAM wp, const LPARAM lp)
{

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

    mbe.wnd = this;
    mbe.pointer_x = GET_X_LPARAM(lp);
    mbe.pointer_y = GET_Y_LPARAM(lp);
    mbe.button = fn_translate_button(type, wp);
    mbe.button1 = (wp & MK_LBUTTON) != 0;
    mbe.button2 = (wp & MK_MBUTTON) != 0;
    mbe.button3 = (wp & MK_RBUTTON) != 0;
    mbe.button4 = (wp & MK_XBUTTON1) != 0;
    mbe.button5 = (wp & MK_XBUTTON2) != 0;
    mbe.shift = (wp & MK_SHIFT) != 0;
    mbe.control = (wp & MK_CONTROL) != 0;

    window_event we;
    we.type = event_type::mouse_button;
    we.event.button = mbe;

    core.events.window(we);
}

void
xray::ui::window::event_mouse_wheel(const WPARAM wparam, const LPARAM lparam)
{
    XR_LOG_INFO("Wheel event!");
    mouse_wheel_event mwe;
    mwe.delta = GET_WHEEL_DELTA_WPARAM(wparam) < 0 ? +1 : -1;
    mwe.fdelta = GET_WHEEL_DELTA_WPARAM(wparam)  / 120.0f;
    mwe.wnd = this;
    mwe.pointer_x = GET_X_LPARAM(lparam);
    mwe.pointer_y = GET_Y_LPARAM(lparam);
    mwe.button1 = (wparam & MK_LBUTTON) != 0;
    mwe.button2 = (wparam & MK_MBUTTON) != 0;
    mwe.button3 = (wparam & MK_RBUTTON) != 0;
    mwe.button4 = (wparam & MK_XBUTTON1) != 0;
    mwe.button5 = (wparam & MK_XBUTTON2) != 0;
    mwe.shift = (wparam & MK_SHIFT) != 0;
    mwe.control = (wparam & MK_CONTROL) != 0;

    window_event we;
    we.type = event_type::mouse_wheel;
    we.event.wheel = mwe;

    core.events.window(we);
}

void
xray::ui::window::event_key(const uint32_t type, const WPARAM wp, const LPARAM lp)
{
    key_event ke;
    ke.wnd = this;
    ke.type = (type == WM_KEYDOWN ? event_action_type::press : event_action_type::release);
    memset(ke.name, 0, sizeof(ke.name));

    const auto cursor_pos = [w = _window]() {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(w, &pt);
        return pt;
    }();

    ke.keycode = map_key(wp);
    ke.pointer_x = cursor_pos.x;
    ke.pointer_y = cursor_pos.y;
    // ke.button1 = (GetAsyncKeyState(VK_LBUTTON) & (1 << 15)) != 0;
    // ke.button2 = (GetAsyncKeyState(VK_MBUTTON) & (1 << 15)) != 0;
    // ke.button3 = (GetAsyncKeyState(VK_RBUTTON) & (1 << 15)) != 0;
    // ke.button4 = (GetAsyncKeyState(VK_XBUTTON1) & (1 << 15)) != 0;
    // ke.button5 = (GetAsyncKeyState(VK_XBUTTON2) & (1 << 15)) != 0;
    ke.shift = (GetAsyncKeyState(VK_SHIFT) & (1 << 15)) != 0;
    ke.control = (GetAsyncKeyState(VK_CONTROL) & (1 << 15)) != 0;

    GetKeyNameText(lp, ke.name, XR_I32_COUNTOF(ke.name));

    window_event we;
    we.type = event_type::key;
    we.event.key = ke;

    core.events.window(we);
}

void
xray::ui::window::event_motion_notify(const WPARAM wparam, const LPARAM lparam)
{
    mouse_motion_event mme;
    mme.wnd = this;
    mme.pointer_x = GET_X_LPARAM(lparam);
    mme.pointer_y = GET_Y_LPARAM(lparam);
    mme.button1 = (wparam & MK_LBUTTON) != 0;
    mme.button2 = (wparam & MK_MBUTTON) != 0;
    mme.button3 = (wparam & MK_RBUTTON) != 0;
    mme.button4 = (wparam & MK_XBUTTON1) != 0;
    mme.button5 = (wparam & MK_XBUTTON2) != 0;
    mme.shift = (wparam & MK_SHIFT) != 0;
    mme.control = (wparam & MK_CONTROL) != 0;

    window_event we;
    we.type = event_type::mouse_motion;
    we.event.motion = mme;

    core.events.window(we);
}

void
xray::ui::window::event_configure(const WPARAM wparam, const LPARAM lparam)
{
    if ((wparam != SIZE_MAXIMIZED) || (wparam != SIZE_RESTORED)) {
        return;
    }

    _wnd_width = GET_X_LPARAM(lparam);
    _wnd_height = GET_Y_LPARAM(lparam);

    window_configure_event cfg_evt;
    cfg_evt.width = _wnd_width;
    cfg_evt.height = _wnd_height;
    cfg_evt.wnd = this;

    window_event we;
    we.type = event_type::configure;
    we.event.configure = cfg_evt;

    core.events.window(we);
}
