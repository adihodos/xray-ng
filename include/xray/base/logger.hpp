//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
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

#include "xray/xray.hpp"
#include <fmt/core.h>

namespace xray::base {

enum class LogLevel : uint8_t
{
    Trace,
    Debug,
    Info,
    Warn,
    Err,
    Critical,

};

void
log_file_line(const LogLevel level,
              const char* file,
              const int32_t line,
              fmt::string_view format,
              fmt::format_args args);

template<typename... Ts>
void
log_fwd_file_line(const LogLevel level,
                  const char* file,
                  const int32_t line,
                  fmt::format_string<Ts...> format,
                  Ts&&... args)
{
    return log_file_line(level, file, line, format, fmt::make_format_args(args...));
}

void
log(const LogLevel level, fmt::string_view format, fmt::format_args args);

template<typename... Ts>
void
log_fwd(const LogLevel level, fmt::format_string<Ts...> format, Ts&&... args)
{
    return log(level, format, fmt::make_format_args(args...));
}

void
setup_logging();
}

#define XR_LOG_ERR(msg, ...)                                                                                           \
    do {                                                                                                               \
        xray::base::log_fwd(xray::base::LogLevel::Err, msg, ##__VA_ARGS__);                                            \
    } while (0)

#define XR_LOG_INFO(msg, ...)                                                                                          \
    do {                                                                                                               \
        xray::base::log_fwd(xray::base::LogLevel::Info, msg, ##__VA_ARGS__);                                           \
    } while (0)

#define XR_LOG_TRACE(msg, ...)                                                                                         \
    do {                                                                                                               \
        xray::base::log_fwd(xray::base::LogLevel::Trace, msg, ##__VA_ARGS__);                                          \
    } while (0)

#define XR_LOG_DEBUG(msg, ...)                                                                                         \
    do {                                                                                                               \
        xray::base::log_fwd(xray::base::LogLevel::Debug, msg, ##__VA_ARGS__);                                          \
    } while (0)

#define XR_LOG_WARN(msg, ...)                                                                                          \
    do {                                                                                                               \
        xray::base::log_fwd(xray::base::LogLevel::Warn, msg, ##__VA_ARGS__);                                           \
    } while (0)

#define XR_LOG_CRITICAL(msg, ...)                                                                                      \
    do {                                                                                                               \
        xray::base::log_fwd(xray::base::LogLevel::Critical, msg, ##__VA_ARGS__);                                       \
    } while (0)

#define XR_LOG_ERR_FILE_LINE(msg, ...)                                                                                 \
    do {                                                                                                               \
        xray::base::log_fwd_file_line(xray::base::LogLevel::Err, __FILE__, __LINE__, msg, ##__VA_ARGS__);              \
    } while (0)

#define XR_LOG_INFO_FILE_LINE(msg, ...)                                                                                \
    do {                                                                                                               \
        xray::base::log_fwd_file_line(xray::base::LogLevel::Info, __FILE__, __LINE__, msg, ##__VA_ARGS__);             \
    } while (0)

#define XR_LOG_TRACE_FILE_LINE(msg, ...)                                                                               \
    do {                                                                                                               \
        xray::base::log_fwd_file_line(xray::base::LogLevel::Trace, __FILE__, __LINE__, msg, ##__VA_ARGS__);            \
    } while (0)

#define XR_LOG_DEBUG_FILE_LINE(msg, ...)                                                                               \
    do {                                                                                                               \
        xray::base::log_fwd_file_line(xray::base::LogLevel::Debug, __FILE__, __LINE__, msg, ##__VA_ARGS__);            \
    } while (0)

#define XR_LOG_WARN_FILE_LINE(msg, ...)                                                                                \
    do {                                                                                                               \
        xray::base::log_fwd_file_line(xray::base::LogLevel::Warn, __FILE__, __LINE__, msg, ##__VA_ARGS__);             \
    } while (0)

#define XR_LOG_CRITICAL_FILE_LINE(msg, ...)                                                                            \
    do {                                                                                                               \
        xray::base::log_fwd_file_line(xray::base::LogLevel::Critical, __FILE__, __LINE__, msg, ##__VA_ARGS__);         \
    } while (0)
