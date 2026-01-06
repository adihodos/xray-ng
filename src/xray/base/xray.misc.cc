#include "xray/base/xray.misc.hpp"
#include "xray/base/xray.types.hpp"
#include "xray/base/logger.hpp"

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#elif defined(XRAY_OS_IS_WINDOWS)
#include <windows.h>
#else
#error "unsupported OS"
#endif

namespace xray::base {

void os_output_debug_string(const char* str) noexcept {
#if defined(XRAY_OS_IS_POSIX_FAMILY)
	static_cast<void>(::write(STDERR_FILENO, str, strlen(str)));
#elif defined(XRAY_OS_IS_WINDOWS)
	OutputDebugString(str);
#else
#error "unsupported OS"
#endif
}

//
// TODO: maybe move the OS specific implementation into separate files
bool os_is_debugger_present() noexcept {
#if defined(XRAY_OS_IS_WINDOWS)
	return IsDebuggerPresent() == TRUE;
#elif defined(XRAY_OS_IS_POSIX_FAMILY)
	// https://stackoverflow.com/questions/3596781/how-to-detect-if-the-current-process-is-being-run-by-gdb
	const I32 proc_fd = open("/proc/self/status", O_RDONLY);
	if (proc_fd == -1) {
		return false;
	}

	char scratch_buffer[4096];
	const auto bytes_read = read(proc_fd, scratch_buffer, sizeof(scratch_buffer) - 1);
	if (bytes_read <= 0) {
		return false;
	}
	scratch_buffer[bytes_read] = 0;

	if (const char* ptr = strstr(scratch_buffer, "TracerPid:"); ptr != nullptr) {
		const char* tracer_id = ptr + strlen("TracerPid:");
		while (*tracer_id && tracer_id <= scratch_buffer + bytes_read) {
			if (isspace(*tracer_id)) {
				++tracer_id;
				continue;
			}

			return isdigit(*tracer_id) && *tracer_id != '0';
		}
	}

	return false;

#else
#error Not implemented!
#endif
}

void os_debug_break() noexcept {
#if defined(XRAY_OS_IS_WINDOWS)
	__debugbreak();
#elif defined(XRAY_OS_IS_POSIX_FAMILY)
	__asm__("int3");
#else
#error Not implemented!
#endif
}

}  // namespace xray::base
