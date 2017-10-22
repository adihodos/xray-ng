#ifndef xray_base_debug_output_hpp__
#define xray_base_debug_output_hpp__

#include "xray/xray.hpp"
#include <cstdint>
#include <fmt/format.h>

#if defined(XRAY_COMPILER_IS_MSVC)
#include <intrin.h>
#endif /* XRAY_COMPILER_IS_MSVC */

namespace xray {
namespace base {
namespace detail {

inline void debug_break() {
#if defined(XRAY_COMPILER_IS_MSVC)
  __debugbreak();
#else  /* XRAY_COMPILER_IS_MSVC */
  __asm__ __volatile__("int $3");
#endif /* !XRAY_COMPILER_IS_MSVC */
}

} // namespace detail

void output_debug_string(const char* format, ...);
} // namespace base
} // namespace xray

#define OUTPUT_DBG_MSG_NO_FILE_LINE(fmt_string, ...)                           \
  do {                                                                         \
    xray::base::output_debug_string(fmt_string, ##__VA_ARGS__);                \
  } while (0)

#define OUTPUT_DBG_MSG(fmt_string, ...)                                        \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      "\nFile [%s], line [%d] : \n", __FILE__, __LINE__);                      \
    xray::base::output_debug_string(fmt_string, ##__VA_ARGS__);                \
  } while (0)

#define OUTPUT_DBG_MSG_WINAPI_FAIL(function_name)                              \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      "File [%s], line [%d] : \n", __FILE__, __LINE__);                        \
    xray::base::output_debug_string(                                           \
      "Function [%s] failed, error code %d", #function_name, GetLastError());  \
  } while (0)

#define XR_DBG_MSG(msg, ...)                                                   \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      (fmt::format(msg, ##__VA_ARGS__)).c_str());                              \
  } while (0)

#define XR_NOT_REACHED()                                                       \
  do {                                                                         \
    xray::base::detail::debug_break();                                         \
  } while (0)

#endif /* !defined(xray_base_debug_output_hpp__) */
