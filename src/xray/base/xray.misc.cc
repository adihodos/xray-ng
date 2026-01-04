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

std::span<std::byte> os_virtual_alloc(const size_t block_size) noexcept {
	std::byte* memptr =
#if defined(XRAY_OS_IS_POSIX_FAMILY)
		static_cast<std::byte*>(
			mmap(nullptr, static_cast<int>(block_size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
		);
#elif defined(XRAY_OS_IS_WINDOWS)
		static_cast<std::byte*>(VirtualAlloc(nullptr, block_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
#error "unsupported OS"
#endif

	if (!memptr) {
		XR_LOG_INFO("VirtualAlloc/mmap failure");
		return {};
	}

	return std::span{static_cast<std::byte*>(memptr), block_size};
}

void os_virtual_free(std::span<std::byte> block) noexcept {
#if defined(XRAY_OS_IS_POSIX_FAMILY)
	if (!block.empty()) {
		munmap(block.data(), block.size());
	}
#elif defined(XRAY_OS_IS_WINDOWS)
	if (!block.empty()) {
		::VirtualFree(static_cast<void*>(block.data()), block.size_bytes(), MEM_DECOMMIT);
	}
#else
#error "unsupported OS"
#endif
}

void os_output_debug_string(const char* str) noexcept {
#if defined(XRAY_OS_IS_POSIX_FAMILY)
	::write(STDERR_FILENO, str, strlen(str));
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
