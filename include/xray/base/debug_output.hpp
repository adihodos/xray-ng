#ifndef xray_base_debug_output_hpp__
#define xray_base_debug_output_hpp__

#include "xray/xray.hpp"

#if defined(XRAY_COMPILER_IS_MSVC)
#include <intrin.h>
#endif /* XRAY_COMPILER_IS_MSVC */

namespace xray {
namespace base {
namespace detail {

inline void
debug_break()
{
#if defined(XRAY_COMPILER_IS_MSVC)
    __debugbreak();
#else  /* XRAY_COMPILER_IS_MSVC */
    __asm__ __volatile__("int $3");
#endif /* !XRAY_COMPILER_IS_MSVC */
}

} // namespace detail

void
setup_logging();

} // namespace base
} // namespace xray

#define XR_NOT_REACHED()                                                                                               \
    do {                                                                                                               \
        xray::base::detail::debug_break();                                                                             \
    } while (0)

#endif /* !defined(xray_base_debug_output_hpp__) */
