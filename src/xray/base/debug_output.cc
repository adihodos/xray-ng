#include "xray/base/debug_output.hpp"

#include <cstdarg>
#include <cstdio>
#include <fmt/format.h>
#include <stlsoft/memory/auto_buffer.hpp>

static void output_dbg_string_impl(const char* fmt_string, va_list args_ptr) {
  const auto req_chars_no_null_term =
    vsnprintf(nullptr, 0, fmt_string, args_ptr);

  if (req_chars_no_null_term <= 0)
    return;

  stlsoft::auto_buffer<char, 1024> msg_buff{
    static_cast<size_t>(req_chars_no_null_term + 1)};

  vsnprintf(msg_buff.data(), msg_buff.size(), fmt_string, args_ptr);
  fputs(msg_buff.data(), stderr);
  fflush(stderr);
}

void xray::base::output_debug_string(const char* format, ...) {
  fputc('\n', stderr);

  va_list args_ptr;
  va_start(args_ptr, format);
  output_dbg_string_impl(format, args_ptr);
  va_end(args_ptr);
}
