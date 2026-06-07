#pragma once

#if defined(__has_feature)
#if __has_feature(address_sanitizer)  // for clang
// GCC and MSVC already set this
// https://learn.microsoft.com/en-us/cpp/sanitizers/asan-building?view=msvc-160
#ifndef __SANITIZE_ADDRESS__
#define __SANITIZE_ADDRESS__
#endif

#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#include <sanitizer/asan_interface.h>
#endif

#include "xray/xray.hpp"
#include "xray/base/xray.slice.hpp"

namespace xray::base {

struct xrSysInfo_t {
	ISIZE page_size{};
	ISIZE processor_count{};
};

extern const xrSysInfo_t* g_SysInfo;
void xray_init() noexcept;

//
// Reserve address space range but don’t commit any memory pages to back it up. Returns nullptr on failure.
// Free with os_free_mem.
[[nodiscard]] xrSlice_t<U8> xrSys_ReserveMemory(void* preferred_addr, const ISIZE bytes) noexcept;

//
// Commit memory pages for reserved address space.
[[nodiscard]] xrSlice_t<U8> xrSys_CommitMemory(const xrSlice_t<U8> mem_range) noexcept;

//
// Decommit the memory pages backing up the specified address space.
void xrSys_DecommitMemory(const xrSlice_t<U8> mem_range) noexcept;

//
// Allocate memory (equivalent of os_reserve_mem + os_commit_mem)
[[nodiscard]] xrSlice_t<U8> xrSys_AllocMemory(const ISIZE size) noexcept;

//
// Free memory allocated with os_alloc_mem.
void xrSys_FreeMemory(const xrSlice_t<U8> mem_range) noexcept;

#if defined(__SANITIZE_ADDRESS__)

inline void poison_memory_region(void* ptr, std::size_t n) { ASAN_POISON_MEMORY_REGION(ptr, n); }
inline void unpoison_memory_region(void* ptr, std::size_t n) { ASAN_UNPOISON_MEMORY_REGION(ptr, n); }

#else

inline void poison_memory_region(void* ptr, std::size_t n) {}
inline void unpoison_memory_region(void* ptr, std::size_t n) {}

#endif

}  // namespace xray::base
