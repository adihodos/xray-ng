#include "xray/base/xray.os.hpp"

#if defined(XRAY_OS_IS_LINUX)
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

#include "xray/base/xray.debug.hpp"

namespace xray::base {
xrSysInfo_t g_SysInfoData	 = {};
const xrSysInfo_t* g_SysInfo = nullptr;
}  // namespace xray::base

void xray::base::xray_init() noexcept {
	XRAY_ASSERT(g_SysInfo == nullptr, "This should only be called once!");

#if defined(XRAY_OS_IS_LINUX)
	g_SysInfoData.page_size		  = sysconf(_SC_PAGESIZE);
	g_SysInfoData.processor_count = get_nprocs();
#else
#error TODO: implement for this OS!
#endif

	g_SysInfo = &g_SysInfoData;
}

//
// Reserve address space range but don’t commit any memory pages to back it up. Returns nullptr on failure.
// Free with os_free_mem.
[[nodiscard]] xray::base::xrSlice_t<xray::U8> xray::base::xrSys_ReserveMemory(
	void* preferred_addr, const ISIZE bytes
) noexcept {
	//
	// TODO: failure handling
	void* reserved_addr = mmap(preferred_addr, bytes, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	if (reserved_addr == MAP_FAILED) {
		return xrSlice_t<U8>{};
	}

	return xrSlice_t<U8>{
		.s_ptr = static_cast<U8*>(reserved_addr),
		.s_len = bytes,
	};
}

//
// Commit memory pages for reserved address space.
[[nodiscard]] xray::base::xrSlice_t<xray::U8> xray::base::xrSys_CommitMemory(const xrSlice_t<U8> mem_range) noexcept {
	//
	// round down to page size, if not aligned
	ISIZE addr_start = reinterpret_cast<ISIZE>(mem_range.s_ptr);
	if ((addr_start & (g_SysInfo->page_size - 1)) != 0) {
		addr_start = xr_round_down(addr_start, g_SysInfo->page_size);
	}

	U8* ptr				= reinterpret_cast<U8*>(addr_start);
	const I32 op_result = mprotect(ptr, mem_range.s_len, PROT_READ | PROT_WRITE);
	if (op_result != 0) {
		return xrSlice_t<U8>{};
	}

	const I32 op_result_madv = madvise(ptr, mem_range.s_len, MADV_WILLNEED | MADV_SEQUENTIAL);
	if (op_result != 0) {
		return xrSlice_t<U8>{};
	}

	return xrSlice_t<U8>{
		.s_ptr = ptr,
		.s_len = mem_range.s_len,
	};
}

//
// Decommit the memory pages backing up the specified address space.
void xray::base::xrSys_DecommitMemory(const xrSlice_t<U8> mem_range) noexcept {
	//
	// round down to page size, if not aligned
	ISIZE addr_start = reinterpret_cast<ISIZE>(mem_range.s_ptr);
	if ((addr_start & (g_SysInfo->page_size - 1)) != 0) {
		addr_start = xr_round_down(addr_start, g_SysInfo->page_size);
	}

	U8* ptr = reinterpret_cast<U8*>(addr_start);
	madvise(static_cast<void*>(ptr), mem_range.s_len, MADV_DONTNEED | MADV_FREE | MADV_REMOVE);
}

//
// Allocate memory (equivalent of os_reserve_mem + os_commit_mem)
[[nodiscard]] xray::base::xrSlice_t<xray::U8> xray::base::xrSys_AllocMemory(const ISIZE size) noexcept {
	//
	// TODO: implement this
	const ISIZE page_aligned_size = xr_round_up(size, g_SysInfo->page_size);
	xrSlice_t<U8> reserved_mem	  = xrSys_ReserveMemory(nullptr, size);
	if (reserved_mem) {
		reserved_mem = xrSys_CommitMemory(reserved_mem);
	}
	return reserved_mem;
}

//
// Free memory allocated with os_alloc_mem.
void xray::base::xrSys_FreeMemory(const xrSlice_t<U8> mem_range) noexcept {
	if (mem_range) {
		xrSys_DecommitMemory(mem_range);
		munmap(mem_range.s_ptr, mem_range.s_len);
	}
}
