#include "xray/base/xray.misc.hpp"
#include "xray/base/logger.hpp"

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <sys/mman.h>
#endif

namespace xray::base {

#if defined(XRAY_OS_IS_POSIX_FAMILY)
std::span<std::byte>
os_virtual_alloc(const size_t block_size) noexcept
{
    void* mem = mmap(nullptr, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!mem) {
        XR_LOG_INFO("mmap failure {}", errno);
        return {};
    }

    return std::span{ static_cast<std::byte*>(mem), block_size };
}

void
os_virtual_free(std::span<std::byte> block) noexcept
{
    if (!block.empty()) {
        munmap(block.data(), block.size());
    }
}

#endif

}
