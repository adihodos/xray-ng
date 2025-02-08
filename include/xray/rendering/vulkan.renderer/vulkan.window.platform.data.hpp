#pragma once

#include <cstdint>

namespace xray::rendering {

#if defined(XRAY_OS_IS_POSIX_FAMILY)

struct WindowPlatformDataXcb
{
    uintptr_t connection;
    uintptr_t window;
    uintptr_t visual;
    uint32_t width;
    uint32_t height;
};

struct WindowPlatformDataXlib
{
    uintptr_t display;
    uintptr_t window;
    uintptr_t visual;
    uint32_t width;
    uint32_t height;
};

#elif defined(XRAY_OS_IS_WINDOWS)

// TODO: replace all OS specific types with cstdint ones
struct WindowPlatformDataWin32
{
    HMODULE module;
    HWND window;
    uint32_t width;
    uint32_t height;
};

#endif

} // namespace xray::rendering
