#include "xray/base/xray.debug.hpp"

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <cctype>
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

#include "xray/base/scoped_guard.hpp"
#include "xray/base/array_dimension.hpp"

//
// TODO: maybe move the OS specific implementation into separate files
bool xray::xrSys_IsDebuggerPresent() noexcept {
#if defined(XRAY_OS_IS_WINDOWS)
	return IsDebuggerPresent() == TRUE;
#elif defined(XRAY_OS_IS_POSIX_FAMILY)
	// https://stackoverflow.com/questions/3596781/how-to-detect-if-the-current-process-is-being-run-by-gdb
	const I32 proc_fd = open("/proc/self/status", O_RDONLY);
	if (proc_fd == -1) {
		return false;
	}
	XRAY_SCOPE_EXIT_NOEXCEPT { close(proc_fd); };

	C8 scratch_buffer[4096];
	const auto bytes_read = read(proc_fd, scratch_buffer, static_cast<size_t>(base::xr_array_size(scratch_buffer) - 1));
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
