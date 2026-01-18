#pragma once

#if defined(XRAY_OS_IS_WINDOWS)

#include <windows.h>

namespace xray::crash_handler {
struct CrashData {
	EXCEPTION_POINTERS* except_ptrs;
	DWORD thread_id;
	DWORD proc_id;
};

}  // namespace xray::crash_handler

#endif
