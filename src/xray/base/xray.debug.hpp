#pragma once

#include "xray/xray.hpp"

namespace xray {

inline void xrSys_DebugBreak() noexcept {
#if defined(XRAY_OS_IS_POSIX_FAMILY)
	__asm__ volatile("int $0x03");
#else
#endif
}

XRAY_ATTRIBUTE_FORMAT(3, 4)
void xrSys_DebugPrintFileLine(const char* file, const I32 line, const char* fmt_spec, ...);

XRAY_ATTRIBUTE_FORMAT(1, 2)
void xrSys_DebugPrint(const char* fmt_spec, ...);

bool xrSys_IsDebuggerPresent() noexcept;

}  // namespace xray

#define xrSys_DebugBreakMsg(msg, ...)               \
	do {                                            \
		xray::xrSys_DebugPrint(msg, ##__VA_ARGS__); \
		xray::xrSys_DebugBreak();                   \
	} while (0)

#define XRAY_ASSERT_NOMSG(condition)                                                      \
	do {                                                                                  \
		if (!(condition)) {                                                               \
			xray::xrSys_DebugPrintFileLine(__FILE__, __LINE__, "ASSERT: %s", #condition); \
			xray::xrSys_DebugBreak();                                                     \
		}                                                                                 \
	} while (0)

#define XRAY_ASSERT(condition, msg, ...)                                                                       \
	do {                                                                                                       \
		if (!(condition)) {                                                                                    \
			xray::xrSys_DebugPrintFileLine(__FILE__, __LINE__, "ASSERT: %s\n" msg, #condition, ##__VA_ARGS__); \
			xray::xrSys_DebugBreak();                                                                          \
		}                                                                                                      \
	} while (0)

#define XRAY_BREAK_IF_DEBUGGER_ATTACHED()      \
	do {                                       \
		if (xray::xrSys_IsDebuggerPresent()) { \
			xray::xrSys_DebugBreak();          \
		}                                      \
	} while (0)
