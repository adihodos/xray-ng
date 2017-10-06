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
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrender.h>
#include <X11/keysym.h>
#include <algorithm>

using namespace xray::base;
using namespace xray::ui;
using namespace xray::ui::detail;
using namespace std;

xray::base::maybe<XineramaScreenInfo> get_primary_screen_info(Display* dpy) {
  int32_t num_screens{};

  unique_pointer<XineramaScreenInfo, decltype(&XFree)> screens{
    XineramaQueryScreens(dpy, &num_screens), &XFree};

  if (!num_screens) {
    return nothing{};
  }

  const auto root_screen = DefaultScreen(dpy);
  auto       itr_def_scr = std::find_if(raw_ptr(screens),
                                  raw_ptr(screens) + num_screens,
                                  [root_screen](const XineramaScreenInfo& si) {
                                    return root_screen == si.screen_number;
                                  });

  if (itr_def_scr == (raw_ptr(screens) + num_screens)) {
    return nothing{};
  }

  return *itr_def_scr;
}

static constexpr const xray::ui::key_sym::e X11_MISC_KEYS_MAPPING_TABLE[] = {
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::backspace,
  key_sym::e::tab,
  key_sym::e::unknown,
  key_sym::e::clear,
  key_sym::e::unknown,
  key_sym::e::enter,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::pause,
  key_sym::e::scrol_lock,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::escape,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::home,
  key_sym::e::left,
  key_sym::e::up,
  key_sym::e::right,
  key_sym::e::down,
  key_sym::e::page_up,
  key_sym::e::page_down,
  key_sym::e::end,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::select,
  key_sym::e::print_screen,
  key_sym::e::unknown,
  key_sym::e::insert,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::kp_multiply,
  key_sym::e::kp_add,
  key_sym::e::unknown,
  key_sym::e::kp_minus,
  key_sym::e::unknown,
  key_sym::e::kp_divide,
  key_sym::e::kp0,
  key_sym::e::kp1,
  key_sym::e::kp2,
  key_sym::e::kp3,
  key_sym::e::kp4,
  key_sym::e::kp5,
  key_sym::e::kp6,
  key_sym::e::kp7,
  key_sym::e::kp8,
  key_sym::e::kp9,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::f1,
  key_sym::e::f2,
  key_sym::e::f3,
  key_sym::e::f4,
  key_sym::e::f5,
  key_sym::e::f6,
  key_sym::e::f7,
  key_sym::e::f8,
  key_sym::e::f9,
  key_sym::e::f10,
  key_sym::e::f11,
  key_sym::e::f12,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::left_shift,
  key_sym::e::right_shift,
  key_sym::e::left_control,
  key_sym::e::right_control,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::left_menu,
  key_sym::e::right_menu,
  key_sym::e::left_alt,
  key_sym::e::right_alt,
  key_sym::e::left_win,
  key_sym::e::right_win,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::unknown,
  key_sym::e::del};

static constexpr const xray::ui::key_sym::e X11_LATIN1_KEYS_MAPPING_TABLE[] = {
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::space,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::key_0,   key_sym::e::key_1,   key_sym::e::key_2,
  key_sym::e::key_3,   key_sym::e::key_4,   key_sym::e::key_5,
  key_sym::e::key_6,   key_sym::e::key_7,   key_sym::e::key_8,
  key_sym::e::key_9,   key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::key_a,
  key_sym::e::key_b,   key_sym::e::key_c,   key_sym::e::key_d,
  key_sym::e::key_e,   key_sym::e::key_f,   key_sym::e::key_g,
  key_sym::e::key_h,   key_sym::e::key_i,   key_sym::e::key_j,
  key_sym::e::key_k,   key_sym::e::key_l,   key_sym::e::key_m,
  key_sym::e::key_n,   key_sym::e::key_o,   key_sym::e::key_p,
  key_sym::e::key_q,   key_sym::e::key_r,   key_sym::e::key_s,
  key_sym::e::key_t,   key_sym::e::key_u,   key_sym::e::key_v,
  key_sym::e::key_w,   key_sym::e::key_x,   key_sym::e::key_y,
  key_sym::e::key_z,   key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::key_a,   key_sym::e::key_b,
  key_sym::e::key_c,   key_sym::e::key_d,   key_sym::e::key_e,
  key_sym::e::key_f,   key_sym::e::key_g,   key_sym::e::key_h,
  key_sym::e::key_i,   key_sym::e::key_j,   key_sym::e::key_k,
  key_sym::e::key_l,   key_sym::e::key_m,   key_sym::e::key_n,
  key_sym::e::key_o,   key_sym::e::key_p,   key_sym::e::key_q,
  key_sym::e::key_r,   key_sym::e::key_s,   key_sym::e::key_t,
  key_sym::e::key_u,   key_sym::e::key_v,   key_sym::e::key_w,
  key_sym::e::key_x,   key_sym::e::key_y,   key_sym::e::key_z,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown, key_sym::e::unknown, key_sym::e::unknown,
  key_sym::e::unknown};

static xray::ui::key_sym::e map_x11_key_symbol(const KeySym x11_key) noexcept {
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
    assert(sym_idx < XR_COUNTOF(X11_MISC_KEYS_MAPPING_TABLE));
    return X11_MISC_KEYS_MAPPING_TABLE[sym_idx];
  } else {
    return xray::ui::key_sym::e::unknown;
  }
}

xray::ui::window::window(const window_params_t& wparam)
  : _display{XOpenDisplay(nullptr)} {

  if (!_display) {
    XR_LOG_ERR("Failed to open display!");
    return;
  }

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
    if (glXQueryVersion(raw_ptr(_display), &glx_ver_major, &glx_ver_minor) !=
        True) {
      XR_LOG_ERR("Failed to get GLX version!");
      return;
    }

    if (glx_ver_major != 1 && glx_ver_minor != 4) {
      XR_LOG_ERR("GLX version mismatch : expected 1.4, have %d.%d",
                 glx_ver_major,
                 glx_ver_minor);
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

  int32_t                                       supported_cfg_count{};
  unique_pointer<GLXFBConfig, decltype(&XFree)> supported_cfgs{
    glXChooseFBConfig(raw_ptr(_display),
                      _default_screen,
                      &framebuffer_attributes[0],
                      &supported_cfg_count),
    &XFree};

  if (supported_cfg_count == 0) {
    XR_LOG_ERR(
      "No framebuffer were found corresponding to the required attributes!");
    return;
  }

  unique_pointer<XVisualInfo, decltype(&XFree)> visual_info{nullptr, &XFree};
  int32_t                                       cfg_idx{};

  do {
    visual_info = unique_pointer<XVisualInfo, decltype(&XFree)>{
      glXGetVisualFromFBConfig(raw_ptr(_display),
                               *(raw_ptr(supported_cfgs) + cfg_idx)),
      &XFree};
  } while ((cfg_idx < supported_cfg_count) && !visual_info);

  if (!visual_info || (cfg_idx >= supported_cfg_count)) {
    XR_LOG_ERR("Failed to get xn XVisual from configs!");
    return;
  }

  auto root_window = RootWindow(raw_ptr(_display), _default_screen);

  pod_zero<XSetWindowAttributes> window_attribs;
  window_attribs.override_redirect = wparam.grab_input ? True : False;
  window_attribs.background_pixel =
    WhitePixel(raw_ptr(_display), _default_screen);
  window_attribs.colormap = XCreateColormap(
    raw_ptr(_display), root_window, visual_info->visual, AllocNone);
  window_attribs.event_mask =
    KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
    PointerMotionMask | ExposureMask | StructureNotifyMask | LeaveWindowMask |
    EnterWindowMask | SubstructureNotifyMask;

  const auto msi = main_screen_info.value();

  _window = x11_unique_window{
    XCreateWindow(raw_ptr(_display),
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
    x11_window_deleter{raw_ptr(_display)}};

  if (!_window) {
    XR_LOG_ERR("Failed to create window !");
    return;
  }

  //
  //  set window properties
  {
    unique_pointer<XSizeHints, decltype(&XFree)> size_hints{XAllocSizeHints(),
                                                            &XFree};
    size_hints->flags       = PSize | PMinSize | PBaseSize;
    size_hints->min_width   = 1024;
    size_hints->min_height  = 1024;
    size_hints->base_width  = msi.width;
    size_hints->base_height = msi.height;

    unique_pointer<XWMHints, decltype(&XFree)> wm_hints{XAllocWMHints(),
                                                        &XFree};
    wm_hints->flags         = StateHint | InputHint;
    wm_hints->initial_state = NormalState;
    wm_hints->input         = True;

    XTextProperty wnd_name;
    if (!XStringListToTextProperty((char**) &wparam.title, 1, &wnd_name)) {
      XR_LOG_ERR("XStringListToTextProperty failed!");
      return;
    }

    XTextProperty icon_name;
    if (!XStringListToTextProperty((char**) &wparam.title, 1, &icon_name)) {
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

  _window_delete_atom =
    XInternAtom(raw_ptr(_display), "WM_DELETE_WINDOW", False);
  XSetWMProtocols(raw_ptr(_display), raw_ptr(_window), &_window_delete_atom, 1);

  //
  // remove window manager decorations
  _motif_hints = XInternAtom(raw_ptr(_display), "_MOTIF_WM_HINTS", False);

  struct {
    unsigned long flags{};
    unsigned long functions{};
    unsigned long decorations{};
    long          input_mode{};
    unsigned long status{};
  } motif_wm_hints{};

  motif_wm_hints.flags       = 2;
  motif_wm_hints.decorations = 0;

  XChangeProperty(raw_ptr(_display),
                  raw_ptr(_window),
                  _motif_hints,
                  _motif_hints,
                  32,
                  PropModeReplace,
                  reinterpret_cast<unsigned char*>(&motif_wm_hints),
                  5);

  //
  // load OpenGL
  const char* extension_list =
    glXQueryExtensionsString(raw_ptr(_display), _default_screen);

  if (!extension_list) {
    XR_LOG_ERR("GLX error : failed to get extensions list!");
    return;
  }

  XR_LOG_INFO("GLX vendor {}",
              glXGetClientString(raw_ptr(_display), GLX_VENDOR));
  XR_LOG_INFO("GLX extensions present:\n{}", extension_list);

  {
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB =
      (PFNGLXCREATECONTEXTATTRIBSARBPROC) glXGetProcAddress(
        (const GLubyte*) "glXCreateContextAttribsARB");

    if (!glXCreateContextAttribsARB ||
        !strstr(extension_list, "GLX_ARB_create_context")) {
      XR_LOG_INFO("Extension glXCreateContextAttribsARB missing!");
      return;
    }

    const int32_t opengl_context_attribs[] = {
      GLX_CONTEXT_MAJOR_VERSION_ARB,
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
      None};

    _glx_context = glx_unique_context{
      glXCreateContextAttribsARB(raw_ptr(_display),
                                 *(raw_ptr(supported_cfgs) + cfg_idx),
                                 nullptr,
                                 True,
                                 opengl_context_attribs),
      glx_context_deleter{raw_ptr(_display)}};

    if (!_glx_context) {
      XR_LOG_ERR("Failed to create OpenGL context!");
      return;
    }

    glXMakeContextCurrent(raw_ptr(_display),
                          raw_ptr(_window),
                          raw_ptr(_window),
                          raw_ptr(_glx_context));

    if (!gl::sys::LoadFunctions()) {
      _glx_context = detail::glx_unique_context{};
      XR_LOG_ERR("Failed to load OpenGL!");
      return;
    }

    XR_LOG_INFO("Loaded OpenGL ({}.{})",
                gl::sys::GetMajorVersion(),
                gl::sys::GetMinorVersion());
  }

  XClearWindow(raw_ptr(_display), raw_ptr(_window));
  XMapRaised(raw_ptr(_display), raw_ptr(_window));
  XMoveWindow(raw_ptr(_display), raw_ptr(_window), msi.x_org, msi.y_org);

  if (wparam.grab_input) {
    _kb_grabbed = XGrabKeyboard(raw_ptr(_display),
                                raw_ptr(_window),
                                False,
                                GrabModeAsync,
                                GrabModeAsync,
                                CurrentTime) == GrabSuccess;

    if (!_kb_grabbed) {
      XR_LOG_WARN("Failed to grab keyboard!");
    }

    _pointer_grabbed =
      XGrabPointer(raw_ptr(_display),
                   raw_ptr(_window),
                   False,
                   ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                     FocusChangeMask | EnterWindowMask | LeaveWindowMask,
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

  Window   root_wnd{};
  int32_t  x_location{};
  int32_t  y_location{};
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

  _wnd_width  = static_cast<int32_t>(width);
  _wnd_height = static_cast<int32_t>(height);

  XR_LOG_INFO(
    "Window [{} x {}], border size {}, depth {}, screen position ({} x {})",
    _wnd_width,
    _wnd_height,
    border_width,
    depth,
    x_location,
    y_location);
}

xray::ui::window::~window() {
  if (_kb_grabbed) {
    XUngrabKeyboard(raw_ptr(_display), CurrentTime);
  }

  if (_pointer_grabbed) {
    XUngrabPointer(raw_ptr(_display), CurrentTime);
  }
}

void xray::ui::window::swap_buffers() noexcept {
  assert(valid());
  glXSwapBuffers(raw_ptr(_display), raw_ptr(_window));
}

void xray::ui::window::message_loop() {
  assert(valid());
  assert(events.loop != nullptr);

  while (_quit_flag == 0) {

    while ((_quit_flag == 0) &&
           XEventsQueued(raw_ptr(_display), QueuedAfterFlush)) {

      XEvent window_event;
      XNextEvent(raw_ptr(_display), &window_event);

      if ((window_event.type == ButtonPress) ||
          (window_event.type == ButtonRelease)) {
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

      if ((window_event.type == KeyPress) ||
          (window_event.type == KeyRelease)) {
        event_key(&window_event.xkey);
        continue;
      }

      if (window_event.type == ConfigureNotify) {
        event_configure(&window_event.xconfigure);
        continue;
      }
    }

    //
    // user loop
    events.loop({_wnd_width, _wnd_height, this});
    glXSwapBuffers(raw_ptr(_display), raw_ptr(_window));
  }
}

void xray::ui::window::event_motion_notify(const XMotionEvent* x11evt) {
  mouse_motion_event mme;
  mme.wnd       = this;
  mme.pointer_x = x11evt->x;
  mme.pointer_y = x11evt->y;
  mme.button1   = (x11evt->state & Button1Mask) != 0;
  mme.button2   = (x11evt->state & Button2Mask) != 0;
  mme.button3   = (x11evt->state & Button3Mask) != 0;
  mme.button4   = (x11evt->state & Button4Mask) != 0;
  mme.button5   = (x11evt->state & Button5Mask) != 0;
  mme.shift     = (x11evt->state & ShiftMask) != 0;
  mme.control   = (x11evt->state & ControlMask) != 0;

  window_event we;
  we.type         = event_type::mouse_motion;
  we.event.motion = mme;

  events.window(we);
}

void xray::ui::window::event_mouse_button(const XButtonEvent* x11evt) {

  struct x11_button_mapping {
    uint32_t     x11_btn;
    mouse_button xray_btn;
  };

  {
    static constexpr x11_button_mapping REGULAR_BUTTON_MAPPINGS[] = {
      {Button1, mouse_button::button1},
      {Button2, mouse_button::button2},
      {Button3, mouse_button::button3}};

    auto mapped_button = find_if(begin(REGULAR_BUTTON_MAPPINGS),
                                 end(REGULAR_BUTTON_MAPPINGS),
                                 [btnid = x11evt->button](const auto& mapping) {
                                   return mapping.x11_btn == btnid;
                                 });

    if (mapped_button != end(REGULAR_BUTTON_MAPPINGS)) {
      XR_LOG_INFO("Button press/release!");
      //
      //
      mouse_button_event mbe;
      mbe.type = x11evt->type == ButtonPress ? event_action_type::press
                                             : event_action_type::release;
      mbe.wnd       = this;
      mbe.pointer_x = x11evt->x;
      mbe.pointer_y = x11evt->y;
      mbe.button    = mapped_button->xray_btn;
      mbe.button1   = (x11evt->state & Button1Mask) != 0;
      mbe.button2   = (x11evt->state & Button2Mask) != 0;
      mbe.button3   = (x11evt->state & Button3Mask) != 0;
      mbe.button4   = (x11evt->state & Button4Mask) != 0;
      mbe.button5   = (x11evt->state & Button5Mask) != 0;
      mbe.shift     = (x11evt->state & ShiftMask) != 0;
      mbe.control   = (x11evt->state & ControlMask) != 0;

      window_event we;
      we.type         = event_type::mouse_button;
      we.event.button = mbe;

      events.window(we);
      return;
    }
  }

  XR_LOG_INFO("Wheel event!");
  mouse_wheel_event mwe;
  mwe.delta     = x11evt->button == Button4 ? -1 : 1;
  mwe.wnd       = this;
  mwe.pointer_x = x11evt->x;
  mwe.pointer_y = x11evt->y;
  mwe.button1   = (x11evt->state & Button1Mask) != 0;
  mwe.button2   = (x11evt->state & Button2Mask) != 0;
  mwe.button3   = (x11evt->state & Button3Mask) != 0;
  mwe.button4   = (x11evt->state & Button4Mask) != 0;
  mwe.button5   = (x11evt->state & Button5Mask) != 0;
  mwe.shift     = (x11evt->state & ShiftMask) != 0;
  mwe.control   = (x11evt->state & ControlMask) != 0;

  window_event we;
  we.type        = event_type::mouse_wheel;
  we.event.wheel = mwe;

  events.window(we);
}

void xray::ui::window::event_client_message(const XClientMessageEvent* x11evt) {
  if ((uint32_t) x11evt->data.l[0] == _window_delete_atom) {
    XR_LOG_INFO("Quit received, exiting message loop...");
    quit();
  }
}

void xray::ui::window::event_key(const XKeyEvent* x11evt) {
  char          tmp[256];
  KeySym        key_sym{NoSymbol};
  const int32_t buff_len =
    XLookupString((XKeyEvent*) x11evt, tmp, 256, &key_sym, nullptr);
  tmp[buff_len] = 0;

  const auto mapped_key = map_x11_key_symbol(key_sym);
  if (mapped_key == key_sym::e::unknown) {
    return;
  }

  key_event ke;
  ke.wnd  = this;
  ke.type = x11evt->type == KeyPress ? event_action_type::press
                                     : event_action_type::release;
  ke.keycode   = mapped_key;
  ke.pointer_x = x11evt->x;
  ke.pointer_y = x11evt->y;
  ke.button1   = (x11evt->state & Button1Mask) != 0;
  ke.button2   = (x11evt->state & Button2Mask) != 0;
  ke.button3   = (x11evt->state & Button3Mask) != 0;
  ke.button4   = (x11evt->state & Button4Mask) != 0;
  ke.button5   = (x11evt->state & Button5Mask) != 0;
  ke.shift     = (x11evt->state & ShiftMask) != 0;
  ke.control   = (x11evt->state & ControlMask) != 0;

  window_event we;
  we.type      = event_type::key;
  we.event.key = ke;

  events.window(we);
}

void xray::ui::window::event_configure(const XConfigureEvent* x11evt) {
  _wnd_width  = x11evt->width;
  _wnd_height = x11evt->height;

  window_configure_event cfg_evt;
  cfg_evt.width  = x11evt->width;
  cfg_evt.height = x11evt->height;
  cfg_evt.wnd    = this;

  window_event we;
  we.type            = event_type::configure;
  we.event.configure = cfg_evt;

  events.window(we);
}
