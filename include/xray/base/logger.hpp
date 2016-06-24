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
#define ELPP_PERFORMANCE_MICROSECONDS
#include <easyloggingpp/easylogging++.h>
#include <fmt/format.h>

#define XR_LOGGER_START(argc, argv)                                            \
  do {                                                                         \
    START_EASYLOGGINGPP(argc, argv);                                           \
  } while (0)

#define XR_LOGGER_CONFIG_FILE(path)                                            \
  do {                                                                         \
    el::Loggers::reconfigureAllLoggers(el::Configurations{path});              \
  } while (0)

#define XR_LOG_ERR(msg, ...)                                                   \
  do {                                                                         \
    LOG(ERROR) << fmt::format(msg, ##__VA_ARGS__);                             \
  } while (0)

#define XR_LOG_INFO(msg, ...)                                                  \
  do {                                                                         \
    LOG(INFO) << fmt::format(msg, ##__VA_ARGS__);                              \
  } while (0)

#define XR_LOG_TRACE(msg, ...)                                                 \
  do {                                                                         \
    LOG(TRACE) << fmt::format(msg, ##__VA_ARGS__);                             \
  } while (0)

#define XR_LOG_DEBUG(msg, ...)                                                 \
  do {                                                                         \
    LOG(DEBUG) << fmt::format(msg, ##__VA_ARGS__);                             \
  } while (0)

#define XR_LOG_WARN(msg, ...)                                                  \
  do {                                                                         \
    LOG(WARNING) << fmt::format(msg, ##__VA_ARGS__);                           \
  } while (0)

#define XR_LOG_CRITICAL(msg, ...)                                              \
  do {                                                                         \
    LOG(FATAL) << fmt::format(msg, ##__VA_ARGS__);                             \
  } while (0)

#define XRAY_TIMED_FUNC() TIMED_FUNC(XRAY_CONCATENATE(__FUNCTION__, __LINE__))

#define XRAY_TIMED_SCOPE(scope_name)                                           \
  TIMED_SCOPE(XRAY_CONCATENATE(__FUNCTION__, __LINE__), scope_name)
