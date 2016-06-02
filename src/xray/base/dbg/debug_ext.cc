#include <cstdarg>
#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/syscall_wrapper.hpp"
#include <stlsoft/memory/auto_buffer.hpp>

void xray::base::debug::output_debug_string(const char* format, ...) {

  int32_t req_chars_no_null_term = 0;

  {
    va_list args_ptr;
    va_start(args_ptr, format);
    req_chars_no_null_term = vsnprintf(nullptr, 0, format, args_ptr);
    va_end(args_ptr);
  }

  if (req_chars_no_null_term <= 0)
    return;

  stlsoft::auto_buffer<char, 1024> msg_buff{
      static_cast<size_t>(req_chars_no_null_term + 1)};

  {
    va_list args_ptr;
    va_start(args_ptr, format);
    vsnprintf(msg_buff.data(), msg_buff.size(), format, args_ptr);
    va_end(args_ptr);
  }

#if !defined(_WIN32)
  syscall_wrapper(write, STDERR_FILENO, "\n", 1);
  syscall_wrapper(write, STDERR_FILENO, msg_buff.data(), msg_buff.size());
#else
  OutputDebugStringA("\n");
  OutputDebugStringA(msg_buff.data());
#endif
}
