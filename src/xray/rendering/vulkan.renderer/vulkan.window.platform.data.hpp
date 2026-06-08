#pragma once

#include "xray/xray.hpp"

namespace xray::rendering {

enum class WindowPlatformType : U8 {
#if defined(XRAY_OS_IS_WINDOWS)
	Windows,
#else
	XLib,
	Xcb,
	Wayland,
#endif
};

#if defined(XRAY_OS_IS_POSIX_FAMILY)

struct WindowPlatformDataXcb {
	UPTR connection;
	UPTR window;
	UPTR visual;
	U32 width;
	U32 height;
};

struct WindowPlatformDataXlib {
	UPTR display;
	UPTR window;
	UPTR visual;
	U32 width;
	U32 height;
};

#elif defined(XRAY_OS_IS_WINDOWS)

struct WindowPlatformDataWin32 {
	UPTR module_handle;
	UPTR window;
	U32 width;
	U32 height;
};

#endif

struct WindowPlatformData {
	WindowPlatformType type;
	union {
#if defined(XRAY_OS_IS_WINDOWS)
		WindowPlatformDataWin32;
#else
		WindowPlatformDataXcb xcb;
		WindowPlatformDataXlib xlib;
#endif
	};
};

}  // namespace xray::rendering
