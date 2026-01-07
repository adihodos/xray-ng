#pragma once

#include <cstdint>
#include <swl/variant.hpp>

namespace xray::rendering {

#if defined(XRAY_OS_IS_POSIX_FAMILY)

struct WindowPlatformDataXcb {
	uintptr_t connection;
	uintptr_t window;
	uintptr_t visual;
	uint32_t width;
	uint32_t height;
};

struct WindowPlatformDataXlib {
	uintptr_t display;
	uintptr_t window;
	uintptr_t visual;
	uint32_t width;
	uint32_t height;
};

#elif defined(XRAY_OS_IS_WINDOWS)

struct WindowPlatformDataWin32 {
	uintptr_t module;
	uintptr_t window;
	uint32_t width;
	uint32_t height;
};

#endif

#if defined(XRAY_OS_IS_WINDOWS)
struct WindowPlatformDataWin32;
using WindowPlatformData = swl::variant<WindowPlatformDataWin32>;
#else
struct WindowPlatformDataXcb;
struct WindowPlatformDataXlib;
using WindowPlatformData = swl::variant<WindowPlatformDataXcb, WindowPlatformDataXlib>;
#endif

}  // namespace xray::rendering
