//
// Copyright (c) 2011, 2012, Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

/// \file debug_ext.hpp Debugging extensions and utilities.

#if defined(XRAY_COMPILER_IS_MSVC)
#include <intrin.h>
#endif /* XRAY_COMPILER_IS_MSVC */

#include "xray/xray.hpp"

namespace xray {
namespace base {
namespace debug {

constexpr auto msg_location_fmt_string = "\n[%s :: %d]";

void output_debug_string(const char* format, ...);

inline void debug_break() {
#if defined(XRAY_COMPILER_IS_MSVC)
  __debugbreak();
#else  /* XRAY_COMPILER_IS_MSVC */
  __asm__ __volatile__("int $3");
#endif /* !XRAY_COMPILER_IS_MSVC */
}

} // namespace debug
} // namespace base
} // namespace xray

#define XR_NOT_REACHED()                                                       \
  do {                                                                         \
    xray::base::debug::debug_break();                                          \
  } while (0)

#define XR_NOT_REACHED_MSG(msg, ...)                                           \
  do {                                                                         \
    xray::base::debug::output_debug_string(                                    \
        xray::base::debug::msg_location_fmt_string, __FILE__, __LINE__);       \
    xray::base::debug::output_debug_string(msg, ##__VA_ARGS__);                \
    xray::base::debug::debug_break();                                          \
  } while (0)

#if defined(XRAY_IS_DEBUG_BUILD)

#define OUTPUT_DBG_MSG(fmt, ...)                                               \
  do {                                                                         \
    xray::base::debug::output_debug_string(                                    \
        xray::base::debug::msg_location_fmt_string, __FILE__, __LINE__);       \
    xray::base::debug::output_debug_string(fmt, ##__VA_ARGS__);                \
  } while (0)

#define OUTPUT_DBG_MSG_NO_FILE_LINE(fmt_string, ...)                           \
  do {                                                                         \
    xray::base::debug::output_debug_string(fmt_string, ##__VA_ARGS__);         \
  } while (0)

#if defined(XRAY_OS_IS_WINDOWS)

#define WIN32_CHK_FNCALL(ret_code_ptr, func_and_args)                          \
  do {                                                                         \
    *(ret_code_ptr) = (func_and_args);                                         \
    if (*(ret_code_ptr) == FALSE) {                                            \
      xray::base::debug::output_debug_string(                                  \
          xray::base::debug::msg_location_fmt_string, __FILE__, __LINE__);     \
      xray::base::debug::output_debug_string(                                  \
          "Function %s failed, error %d", #func_and_args, ::GetLastError());   \
    }                                                                          \
  } while (0)

#endif /* XRAY_OS_IS_WINDOWS */

#else /* XRAY_IS_DEBUG_BUILD */

#define OUTPUT_DBG_MSG(fmt, ...) static_cast<void>(0)
#define OUTPUT_DBG_MSG_NO_FILE_LINE(fmt, ...) static_cast<void>(0)

#if defined(XRAY_OS_IS_WINDOWS)

#define WIN32_CHK_FNCALL(ret_code_ptr, func_and_args)                          \
  do {                                                                         \
    *(ret_code_ptr) = (func_and_args);                                         \
  } while (0)
#endif /* XRAY_OS_IS_WINDOWS */

#endif /* !XRAY_IS_DEBUG_BUILD */

#if defined(XRAY_OS_IS_WINDOWS)

#define WAIT_FOR_DEBUGGER_AND_BREAK()                                          \
  do {                                                                         \
    while (!IsDebuggerPresent()) {                                             \
      Sleep(100);                                                              \
    }                                                                          \
    xray::base::debug::debug_break();                                          \
  } while (0)

#endif /* XRAY_OS_IS_WINDOWS */
