#include "xray/base/xray.misc.hpp"
#include "xray/base/logger.hpp"

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <sys/mman.h>
#elif defined(XRAY_OS_IS_WINDOWS)
#include <windows.h>
#else
#error "unsupported OS"
#endif

namespace xray::base {

std::span<std::byte>
os_virtual_alloc(const size_t block_size) noexcept
{
    std::byte* memptr =
#if defined(XRAY_OS_IS_POSIX_FAMILY)
        static_cast<std::byte*>(
            mmap(nullptr, static_cast<int>(block_size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
#elif defined(XRAY_OS_IS_WINDOWS)
        static_cast<std::byte*>(VirtualAlloc(nullptr, block_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
#else
#error "unsupported OS"
#endif

    if (!memptr) {
        XR_LOG_INFO("VirtualAlloc/mmap failure");
        return {};
    }

    return std::span{ static_cast<std::byte*>(memptr), block_size };
}

void
os_virtual_free(std::span<std::byte> block) noexcept
{
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

} // namespace xray::base
