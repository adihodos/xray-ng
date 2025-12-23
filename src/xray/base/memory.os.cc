#include "xray/base/memory.os.hpp"

#include <cassert>
#include <cstdint>

#if defined(XRAY_OS_IS_POSIX_FAMILY)

#include <sys/mman.h>
#include <unistd.h>

namespace xray::base {
[[nodiscard]] size_t os_get_page_size() noexcept {
	static const size_t page_size = static_cast<size_t>(sysconf(_SC_PAGESIZE));
	return page_size;
}

[[nodiscard]] void* os_reserve_mem(const size_t size) noexcept {
	void* addr = mmap(nullptr, size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (addr == MAP_FAILED) {
		return nullptr;
	}

	return addr;
}

[[nodiscard]] void* os_commit_mem(void* addr, const size_t size) noexcept {
	const uintptr_t addr_val = reinterpret_cast<uintptr_t>(addr);
	assert(addr_val % os_get_page_size() == 0);

	const int result = mprotect(addr, size, PROT_READ | PROT_WRITE);
	return result == 0 ? addr : nullptr;
}

void os_decommit_mem(void* addr, const size_t size) noexcept {
	const uintptr_t addr_val = reinterpret_cast<uintptr_t>(addr);
	assert(addr_val % os_get_page_size() == 0);
	const int32_t result = mprotect(addr, size, PROT_NONE);
	assert(result == 0);
}

[[nodiscard]] void* os_alloc_mem(const size_t size) noexcept {
	void* addr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (addr == MAP_FAILED) {
		return nullptr;
	}

	return addr;
}

void os_free_mem(void* addr, const size_t size) noexcept {
	const int32_t result = munmap(addr, size);
	assert(result == 0);
}
}  // namespace xray::base

#elif defined(XRAY_OS_IS_WINDOWS)
#else
#error Unsupported system!
#endif
