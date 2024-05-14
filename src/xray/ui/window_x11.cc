#include "opengl/opengl.hpp"

#include "xray/base/array_dimension.hpp"
#include "xray/base/containers/fixed_vector.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/window_x11.hpp"
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib-xcb.h> /* for XGetXCBConnection, link with libX11-xcb */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrender.h>
#include <X11/keysym.h>
#include <xcb/xcb.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon.h>

#include <algorithm>
#include <span>

using namespace xray::base;
using namespace xray::ui;
using namespace xray::ui::detail;
using namespace std;

xray::base::maybe<XineramaScreenInfo>
get_primary_screen_info(Display* dpy)
{
    int32_t num_screens{};

    unique_pointer<XineramaScreenInfo, decltype(&XFree)> screens{ XineramaQueryScreens(dpy, &num_screens), &XFree };

    if (!num_screens) {
        return nothing{};
    }

    const auto root_screen = DefaultScreen(dpy);
    auto itr_def_scr =
        std::find_if(raw_ptr(screens), raw_ptr(screens) + num_screens, [root_screen](const XineramaScreenInfo& si) {
            return root_screen == si.screen_number;
        });

    if (itr_def_scr == (raw_ptr(screens) + num_screens)) {
        return nothing{};
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

static xray::ui::KeySymbol
map_x11_key_symbol(const xkb_keysym_t key_sym) noexcept
{
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

xray::ui::window::window(const window_params_t& wparam)

{
    int32_t ver_major{ XkbMajorVersion };
    int32_t ver_minor{ XkbMinorVersion };
    int32_t err_code{};
    int32_t xkb_event_base{};
    int32_t xkb_error_base{};
    _display = x11_unique_display{ XkbOpenDisplay(
        nullptr, &xkb_event_base, &xkb_error_base, &ver_major, &ver_minor, &err_code) };
    if (!_display) {
        XR_LOG_CRITICAL("Failed to open display, error {}", err_code);
        return;
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

    detail::unique_xbk_context xkb_ctx{ xkb_context_new(XKB_CONTEXT_NO_FLAGS) };
    if (!xkb_ctx) {
        XR_LOG_ERR("Failed to get XKB context!");
    }

    const int32_t device_id = xkb_x11_get_core_keyboard_device_id(connection);
    XR_LOG_INFO("Core keyboard id {}", device_id);

    detail::unique_xkb_keyman keymap{ xkb_x11_keymap_new_from_device(
        raw_ptr(xkb_ctx), connection, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS) };
    if (!keymap) {
        XR_LOG_ERR("Failed to get XKB keymap!");
    }

    detail::unique_xkb_state state{ xkb_x11_state_new_from_device(raw_ptr(keymap), connection, device_id) };

    const auto main_screen_info = get_primary_screen_info(raw_ptr(_display));
    if (!main_screen_info) {
        XR_LOG_ERR("Failed to get primary screen info!");
        return;
    }

    _default_screen = XDefaultScreen(raw_ptr(_display));

    if (glXQueryExtension(raw_ptr(_display), nullptr, nullptr) != True) {
        XR_LOG_ERR("GLX extension not present!");
        return;
    }

    {
        int32_t glx_ver_major{};
        int32_t glx_ver_minor{};
        if (glXQueryVersion(raw_ptr(_display), &glx_ver_major, &glx_ver_minor) != True) {
            XR_LOG_ERR("Failed to get GLX version!");
            return;
        }

        if (glx_ver_major != 1 && glx_ver_minor != 4) {
            XR_LOG_ERR("GLX version mismatch : expected 1.4, have %d.%d", glx_ver_major, glx_ver_minor);
            return;
        }
    }

    fixed_vector<int32_t, 32> framebuffer_attributes;

    framebuffer_attributes.push_back(GLX_BUFFER_SIZE);
    framebuffer_attributes.push_back(wparam.color_bits);
    framebuffer_attributes.push_back(GLX_DOUBLEBUFFER);
    framebuffer_attributes.push_back(True);
    framebuffer_attributes.push_back(GLX_DEPTH_SIZE);
    framebuffer_attributes.push_back(wparam.depth_bits);
    framebuffer_attributes.push_back(GLX_STENCIL_SIZE);
    framebuffer_attributes.push_back(wparam.stencil_bits);
    framebuffer_attributes.push_back(GLX_RENDER_TYPE);
    framebuffer_attributes.push_back(GLX_RGBA_BIT);
    framebuffer_attributes.push_back(GLX_DRAWABLE_TYPE);
    framebuffer_attributes.push_back(GLX_WINDOW_BIT);
    framebuffer_attributes.push_back(GLX_CONFIG_CAVEAT);
    framebuffer_attributes.push_back(GLX_NONE);

    if (wparam.sample_count > 0) {
        framebuffer_attributes.push_back(GLX_SAMPLE_BUFFERS);
        framebuffer_attributes.push_back(1);
        framebuffer_attributes.push_back(GLX_SAMPLES);
        framebuffer_attributes.push_back(wparam.sample_count);
    }

    framebuffer_attributes.push_back(None);

    int32_t supported_cfg_count{};
    unique_pointer<GLXFBConfig, decltype(&XFree)> supported_cfgs{
        glXChooseFBConfig(raw_ptr(_display), _default_screen, &framebuffer_attributes[0], &supported_cfg_count), &XFree
    };

    if (supported_cfg_count == 0) {
        XR_LOG_ERR("No framebuffer were found corresponding to the required attributes!");
        return;
    }

    unique_pointer<XVisualInfo, decltype(&XFree)> visual_info{ nullptr, &XFree };
    int32_t cfg_idx{};

    do {
        visual_info = unique_pointer<XVisualInfo, decltype(&XFree)>{
            glXGetVisualFromFBConfig(raw_ptr(_display), *(raw_ptr(supported_cfgs) + cfg_idx)), &XFree
        };
    } while ((cfg_idx < supported_cfg_count) && !visual_info);

    if (!visual_info || (cfg_idx >= supported_cfg_count)) {
        XR_LOG_ERR("Failed to get xn XVisual from configs!");
        return;
    }

    auto root_window = RootWindow(raw_ptr(_display), _default_screen);

    pod_zero<XSetWindowAttributes> window_attribs;
    window_attribs.override_redirect = wparam.grab_input ? True : False;
    window_attribs.background_pixel = WhitePixel(raw_ptr(_display), _default_screen);
    window_attribs.colormap = XCreateColormap(raw_ptr(_display), root_window, visual_info->visual, AllocNone);
    window_attribs.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
                                PointerMotionMask | ExposureMask | StructureNotifyMask | LeaveWindowMask |
                                EnterWindowMask | SubstructureNotifyMask;

    const auto msi = main_screen_info.value();

    _window = x11_unique_window{ XCreateWindow(raw_ptr(_display),
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
                                               &window_attribs),
                                 x11_window_deleter{ raw_ptr(_display) } };

    if (!_window) {
        XR_LOG_ERR("Failed to create window !");
        return;
    }

    //
    //  set window properties
    {
        unique_pointer<XSizeHints, decltype(&XFree)> size_hints{ XAllocSizeHints(), &XFree };
        size_hints->flags = PMinSize | PBaseSize;
        size_hints->min_width = 1024;
        size_hints->min_height = 1024;
        size_hints->base_width = msi.width;
        size_hints->base_height = msi.height;

        unique_pointer<XWMHints, decltype(&XFree)> wm_hints{ XAllocWMHints(), &XFree };
        wm_hints->flags = StateHint | InputHint;
        wm_hints->initial_state = NormalState;
        wm_hints->input = True;

        XTextProperty wnd_name;
        if (!XStringListToTextProperty((char**)&wparam.title, 1, &wnd_name)) {
            XR_LOG_ERR("XStringListToTextProperty failed!");
            return;
        }

        XTextProperty icon_name;
        if (!XStringListToTextProperty((char**)&wparam.title, 1, &icon_name)) {
            XR_LOG_ERR("XStringListToTextProperty failed!");
            return;
        }

        XSetWMProperties(raw_ptr(_display),
                         raw_ptr(_window),
                         &wnd_name,
                         &icon_name,
                         nullptr,
                         0,
                         raw_ptr(size_hints),
                         raw_ptr(wm_hints),
                         nullptr);
    }

    _window_delete_atom = XInternAtom(raw_ptr(_display), "WM_DELETE_WINDOW", False);
    XSetWMProtocols(raw_ptr(_display), raw_ptr(_window), &_window_delete_atom, 1);

    const Atom window_atoms[] = {
        XInternAtom(raw_ptr(_display), "_NET_WM_STATE", False),
        XInternAtom(raw_ptr(_display), "_NET_WM_STATE_MAXIMIZED_VERT", False),
        XInternAtom(raw_ptr(_display), "_NET_WM_STATE_MAXIMIZED_HORZ", False),
        XInternAtom(raw_ptr(_display), "_NET_WM_STATE_SKIP_PAGER", False),
        XInternAtom(raw_ptr(_display), "_NET_WM_STATE_ABOVE", False),
        XInternAtom(raw_ptr(_display), "_NET_WM_STATE_FULLSCREEN", False),
    };

    XChangeProperty(raw_ptr(_display),
                    raw_ptr(_window),
                    window_atoms[0],
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char*)(&window_atoms[1]),
                    XR_I32_COUNTOF(window_atoms) - 1);

    const Atom name_atoms[] = {
        XInternAtom(raw_ptr(_display), "_NET_WM_NAME", False),
        XInternAtom(raw_ptr(_display), "UTF8_STRING", False),
    };

    //
    // load OpenGL
    const char* extension_list = glXQueryExtensionsString(raw_ptr(_display), _default_screen);

    if (!extension_list) {
        XR_LOG_ERR("GLX error : failed to get extensions list!");
        return;
    }

    XR_LOG_INFO("GLX vendor {}", glXGetClientString(raw_ptr(_display), GLX_VENDOR));
    XR_LOG_INFO("GLX extensions present:\n{}", extension_list);

    {
        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
            (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

        if (!glXCreateContextAttribsARB || !strstr(extension_list, "GLX_ARB_create_context")) {
            XR_LOG_INFO("Extension glXCreateContextAttribsARB missing!");
            return;
        }

        const int32_t opengl_context_attribs[] = { GLX_CONTEXT_MAJOR_VERSION_ARB,
                                                   wparam.ver_major,
                                                   GLX_CONTEXT_MINOR_VERSION_ARB,
                                                   wparam.ver_minor,
                                                   GLX_CONTEXT_FLAGS_ARB,
                                                   GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |
                                                       (wparam.debug_output_level == 0 ? 0 : GLX_CONTEXT_DEBUG_BIT_ARB),
                                                   GLX_CONTEXT_PROFILE_MASK_ARB,
                                                   GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                                                   GLX_RENDER_TYPE,
                                                   GLX_RGBA_TYPE,
                                                   None };

        _glx_context = glx_unique_context{
            glXCreateContextAttribsARB(
                raw_ptr(_display), *(raw_ptr(supported_cfgs) + cfg_idx), nullptr, True, opengl_context_attribs),
            glx_context_deleter{ raw_ptr(_display) }
        };

        if (!_glx_context) {
            XR_LOG_ERR("Failed to create OpenGL context!");
            return;
        }

        glXMakeContextCurrent(raw_ptr(_display), raw_ptr(_window), raw_ptr(_window), raw_ptr(_glx_context));

        if (!gl::sys::LoadFunctions()) {
            _glx_context = detail::glx_unique_context{};
            XR_LOG_ERR("Failed to load OpenGL!");
            return;
        }

        XR_LOG_INFO("Loaded OpenGL ({}.{})", gl::sys::GetMajorVersion(), gl::sys::GetMinorVersion());
    }

    XClearWindow(raw_ptr(_display), raw_ptr(_window));
    XMapRaised(raw_ptr(_display), raw_ptr(_window));
    XMoveWindow(raw_ptr(_display), raw_ptr(_window), msi.x_org, msi.y_org);

    if (wparam.grab_input) {
        _kb_grabbed =
            XGrabKeyboard(raw_ptr(_display), raw_ptr(_window), False, GrabModeAsync, GrabModeAsync, CurrentTime) ==
            GrabSuccess;

        if (!_kb_grabbed) {
            XR_LOG_WARN("Failed to grab keyboard!");
        }

        _pointer_grabbed = XGrabPointer(raw_ptr(_display),
                                        raw_ptr(_window),
                                        False,
                                        ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask |
                                            EnterWindowMask | LeaveWindowMask,
                                        GrabModeAsync,
                                        GrabModeAsync,
                                        raw_ptr(_window),
                                        None,
                                        CurrentTime) == GrabSuccess;

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

    XGetGeometry(raw_ptr(_display),
                 raw_ptr(_window),
                 &root_wnd,
                 &x_location,
                 &y_location,
                 &width,
                 &height,
                 &border_width,
                 &depth);

    _wnd_width = static_cast<int32_t>(width);
    _wnd_height = static_cast<int32_t>(height);

    const uint32_t mod_shift = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_SHIFT);
    const uint32_t mod_caps = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_CAPS);
    const uint32_t mod_alt = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_ALT);
    const uint32_t mod_ctrl = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_CTRL);
    const uint32_t mod_num = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_NUM);
    const uint32_t mod_logo = xkb_keymap_mod_get_index(raw_ptr(keymap), XKB_MOD_NAME_LOGO);

    _input_helper = XInputHelper{ connection,
                                  move(xkb_ctx),
                                  move(keymap),
                                  move(state),
                                  xkb_event_base,
                                  xkb_error_base,
                                  XInputHelper::KbMods{ .shift_mod = mod_shift,
                                                        .caps_mod = mod_caps,
                                                        .ctrl_mod = mod_ctrl,
                                                        .alt_mod = mod_alt,
                                                        .num_mod = mod_num,
                                                        .logo_mod = mod_logo } };

    XR_LOG_INFO("Window [{} x {}], border size {}, depth {}, screen position ({} x {})",
                _wnd_width,
                _wnd_height,
                border_width,
                depth,
                x_location,
                y_location);
}

xray::ui::window::~window()
{
    if (_kb_grabbed) {
        XUngrabKeyboard(raw_ptr(_display), CurrentTime);
    }

    if (_pointer_grabbed) {
        XUngrabPointer(raw_ptr(_display), CurrentTime);
    }

    glXMakeContextCurrent(raw_ptr(_display), None, None, nullptr);
}

void
xray::ui::window::swap_buffers() noexcept
{
    assert(valid());
    glXSwapBuffers(raw_ptr(_display), raw_ptr(_window));
}

void
xray::ui::window::message_loop()
{
    assert(valid());
    assert(events.loop != nullptr);

    while (_quit_flag == 0) {

        events.poll_start({});

        while ((_quit_flag == 0) && XEventsQueued(raw_ptr(_display), QueuedAfterFlush)) {

            XEvent window_event;
            XNextEvent(raw_ptr(_display), &window_event);

            if (window_event.type == _input_helper.xkb_event_base) {
                const XkbEvent* xkb_evt = reinterpret_cast<const XkbEvent*>(&window_event);
                if (xkb_evt->any.xkb_type == XkbStateNotify) {

                    // XR_LOG_INFO("XkbStateNotify: mods {:0x} base mods: {:0x} latched mods {:0x} locked mods {:0x}",
                    //             xkb_evt->state.mods,
                    //             xkb_evt->state.base_mods,
                    //             xkb_evt->state.latched_mods,
                    //             xkb_evt->state.locked_mods);

                    auto get_xkb_mod_mask_fn = [xi = &_input_helper](const uint32_t in) {
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

                    xkb_state_update_mask(raw_ptr(_input_helper.xkb_state),
                                          get_xkb_mod_mask_fn(xkb_evt->state.base_mods),
                                          get_xkb_mod_mask_fn(xkb_evt->state.latched_mods),
                                          get_xkb_mod_mask_fn(xkb_evt->state.locked_mods),
                                          0,
                                          0,
                                          xkb_evt->state.group);
                }
                continue;
            }

            if ((window_event.type == ButtonPress) || (window_event.type == ButtonRelease)) {
                event_mouse_button(&window_event.xbutton);
                continue;
            }

            if (window_event.type == MotionNotify) {
                event_motion_notify(&window_event.xmotion);
                continue;
            }

            if (window_event.type == ClientMessage) {
                event_client_message(&window_event.xclient);
                continue;
            }

            if ((window_event.type == KeyPress) || (window_event.type == KeyRelease)) {
                event_key(&window_event.xkey);
                continue;
            }
        }

        events.poll_end({});

        //
        // user loop
        events.loop({ _wnd_width, _wnd_height, this });
        glXSwapBuffers(raw_ptr(_display), raw_ptr(_window));
    }
}

void
xray::ui::window::event_motion_notify(const XMotionEvent* x11evt)
{
    mouse_motion_event mme;
    mme.wnd = this;
    mme.pointer_x = x11evt->x;
    mme.pointer_y = x11evt->y;
    mme.button1 = (x11evt->state & Button1Mask) != 0;
    mme.button2 = (x11evt->state & Button2Mask) != 0;
    mme.button3 = (x11evt->state & Button3Mask) != 0;
    mme.button4 = (x11evt->state & Button4Mask) != 0;
    mme.button5 = (x11evt->state & Button5Mask) != 0;
    mme.shift = (x11evt->state & ShiftMask) != 0;
    mme.control = (x11evt->state & ControlMask) != 0;

    window_event we;
    we.type = event_type::mouse_motion;
    we.event.motion = mme;

    events.window(we);
}

void
xray::ui::window::event_mouse_button(const XButtonEvent* x11evt)
{

    struct x11_button_mapping
    {
        uint32_t x11_btn;
        mouse_button xray_btn;
    };

    {
        static constexpr x11_button_mapping REGULAR_BUTTON_MAPPINGS[] = { { Button1, mouse_button::button1 },
                                                                          { Button2, mouse_button::button2 },
                                                                          { Button3, mouse_button::button3 } };

        auto mapped_button =
            find_if(begin(REGULAR_BUTTON_MAPPINGS),
                    end(REGULAR_BUTTON_MAPPINGS),
                    [btnid = x11evt->button](const auto& mapping) { return mapping.x11_btn == btnid; });

        if (mapped_button != end(REGULAR_BUTTON_MAPPINGS)) {
            //
            //
            mouse_button_event mbe;
            mbe.type = x11evt->type == ButtonPress ? event_action_type::press : event_action_type::release;
            mbe.wnd = this;
            mbe.pointer_x = x11evt->x;
            mbe.pointer_y = x11evt->y;
            mbe.button = mapped_button->xray_btn;
            mbe.button1 = (x11evt->state & Button1Mask) != 0;
            mbe.button2 = (x11evt->state & Button2Mask) != 0;
            mbe.button3 = (x11evt->state & Button3Mask) != 0;
            mbe.button4 = (x11evt->state & Button4Mask) != 0;
            mbe.button5 = (x11evt->state & Button5Mask) != 0;
            mbe.shift = (x11evt->state & ShiftMask) != 0;
            mbe.control = (x11evt->state & ControlMask) != 0;

            window_event we;
            we.type = event_type::mouse_button;
            we.event.button = mbe;

            events.window(we);
            return;
        }
    }

    mouse_wheel_event mwe;
    mwe.delta = x11evt->button == Button4 ? 1 : -1;
    mwe.wnd = this;
    mwe.pointer_x = x11evt->x;
    mwe.pointer_y = x11evt->y;
    mwe.button1 = (x11evt->state & Button1Mask) != 0;
    mwe.button2 = (x11evt->state & Button2Mask) != 0;
    mwe.button3 = (x11evt->state & Button3Mask) != 0;
    mwe.button4 = (x11evt->state & Button4Mask) != 0;
    mwe.button5 = (x11evt->state & Button5Mask) != 0;
    mwe.shift = (x11evt->state & ShiftMask) != 0;
    mwe.control = (x11evt->state & ControlMask) != 0;

    window_event we;
    we.type = event_type::mouse_wheel;
    we.event.wheel = mwe;

    events.window(we);
}

void
xray::ui::window::event_client_message(const XClientMessageEvent* x11evt)
{
    if ((uint32_t)x11evt->data.l[0] == _window_delete_atom) {
        XR_LOG_INFO("Quit received, exiting message loop...");
        quit();
    }
}

void
xray::ui::window::event_key(const XKeyEvent* x11evt)
{
    const xkb_keycode_t key_code{ x11evt->keycode };
    const xkb_keysym_t key_sym{ xkb_state_key_get_one_sym(raw_ptr(_input_helper.xkb_state), key_code) };

    key_event ke;
    if (const int bytes_len = xkb_keysym_get_name(key_sym, ke.name, sizeof(ke.name)); bytes_len >= 0)
        ke.name[bytes_len] = 0;

    if (const KeySymbol mapped_key = map_x11_key_symbol(key_sym); mapped_key != KeySymbol::unknown) {
        ke.wnd = this;
        ke.type = x11evt->type == KeyPress ? event_action_type::press : event_action_type::release;
        ke.keycode = mapped_key;
        ke.pointer_x = x11evt->x;
        ke.pointer_y = x11evt->y;

        ke.meta = (x11evt->state & Mod4Mask) != 0;
        ke.alt = (x11evt->state & Mod1Mask) != 0;
        ke.shift = (x11evt->state & ShiftMask) != 0;
        ke.control = (x11evt->state & ControlMask) != 0;

        window_event we;
        we.type = event_type::key;
        we.event.key = ke;

        events.window(we);
    }

    if (x11evt->type == KeyPress) {
        // https://gist.github.com/bluetech/6038239
        // https://stackoverflow.com/questions/64722105/without-creating-a-window-is-it-possible-to-detect-the-current-xcb-modifier-sta
        // https://www.x.org/releases/X11R7.7/doc/kbproto/xkbproto.html
        // https://github.com/xkbcommon/libxkbcommon/blob/master/test/state.c
        // https://github.com/xkbcommon/libxkbcommon/blob/master/doc/quick-guide.md
        // https://stackoverflow.com/questions/10157826/xkb-how-to-convert-a-keycode-to-keysym?rq=3
        char_input_event ch_input;
        ch_input.wnd = this;
        ch_input.key_code = ke.keycode;

        if (this->key_sym_handling == InputKeySymHandling::Unicode) {
            ch_input.unicode_point = xkb_state_key_get_utf32(raw_ptr(_input_helper.xkb_state), key_code);

            if (ch_input.unicode_point != 0) {
                window_event we;
                we.type = event_type::char_input;
                we.event.char_input = ch_input;
                events.window(we);
            }
        } else {
            KeySym x11_key_sym;
            uint32_t mod_return{};
            XkbLookupKeySym(raw_ptr(_display), x11evt->keycode, x11evt->state, &mod_return, &x11_key_sym);

            const int32_t num_bytes = XkbTranslateKeySym(
                raw_ptr(_display), &x11_key_sym, x11evt->state, ch_input.utf8, sizeof(ch_input.utf8), nullptr);
            ch_input.utf8[num_bytes > 0 ? num_bytes : 0] = 0;

            if (num_bytes > 0) {
                window_event we;
                we.type = event_type::char_input;
                we.event.char_input = ch_input;
                events.window(we);
            }
        }
    }
}

void
xray::ui::window::event_configure(const XConfigureEvent* x11evt)
{
    _wnd_width = x11evt->width;
    _wnd_height = x11evt->height;

    window_configure_event cfg_evt;
    cfg_evt.width = x11evt->width;
    cfg_evt.height = x11evt->height;
    cfg_evt.wnd = this;

    window_event we;
    we.type = event_type::configure;
    we.event.configure = cfg_evt;

    events.window(we);
}
