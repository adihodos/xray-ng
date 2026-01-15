#pragma once

#include <cstddef>

namespace xray::base {

//
// Returns the size of a page of memory.
[[nodiscard]] size_t os_get_page_size() noexcept;

//
// Reserve address space range but don’t commit any memory pages to back it up. Returns nullptr on failure.
// Free with os_free_mem.
[[nodiscard]] void* os_reserve_mem(const size_t size) noexcept;

//
// Commit memory pages for reserved address space.
[[nodiscard]] void* os_commit_mem(void* addr, const size_t size) noexcept;

//
// Decommit the memory pages backing up the specified address space.
void os_decommit_mem(void* addr, const size_t size) noexcept;

//
// Allocate memory (equivalent of os_reserve_mem + os_commit_mem)
[[nodiscard]] void* os_alloc_mem(const size_t size) noexcept;

//
// Free memory allocated with os_alloc_mem.
void os_free_mem(void* addr, const size_t size) noexcept;

}  // namespace xray::base
