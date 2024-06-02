#pragma once

#include <vulkan/vulkan.h>

#if defined(XRAY_OS_IS_POSIX_FAMILY)

#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>

#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>

namespace xray::rendering {

struct WindowPlatformDataXcb
{
    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_visualid_t visual;
    uint32_t width;
    uint32_t height;
};

struct WindowPlatformDataXlib
{
    Display* display;
    Window window;
    VisualID visual;
    uint32_t width;
    uint32_t height;
};

}

#endif
