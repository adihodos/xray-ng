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
#include "xray/base/debug_output.hpp"
#include <fmt/format.h>

#define XR_LOG_ERR(msg, ...)                                                   \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      fmt::format("[{} : {}]", __FILE__, __LINE__).c_str());                   \
    xray::base::output_debug_string(fmt::format(msg, ##__VA_ARGS__).c_str());  \
  } while (0)

#define XR_LOG_INFO(msg, ...)                                                  \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      fmt::format("[{} : {}]", __FILE__, __LINE__).c_str());                   \
    xray::base::output_debug_string(fmt::format(msg, ##__VA_ARGS__).c_str());  \
  } while (0)

#define XR_LOG_TRACE(msg, ...)                                                 \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      fmt::format("[{} : {}]", __FILE__, __LINE__).c_str());                   \
    xray::base::output_debug_string(fmt::format(msg, ##__VA_ARGS__).c_str());  \
  } while (0)

#define XR_LOG_DEBUG(msg, ...)                                                 \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      fmt::format("[{} : {}]", __FILE__, __LINE__).c_str());                   \
    xray::base::output_debug_string(fmt::format(msg, ##__VA_ARGS__).c_str());  \
  } while (0)

#define XR_LOG_WARN(msg, ...)                                                  \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      fmt::format("[{} : {}]", __FILE__, __LINE__).c_str());                   \
    xray::base::output_debug_string(fmt::format(msg, ##__VA_ARGS__).c_str());  \
  } while (0)

#define XR_LOG_CRITICAL(msg, ...)                                              \
  do {                                                                         \
    xray::base::output_debug_string(                                           \
      fmt::format("[{} : {}]", __FILE__, __LINE__).c_str());                   \
    xray::base::output_debug_string(fmt::format(msg, ##__VA_ARGS__).c_str());  \
  } while (0)
