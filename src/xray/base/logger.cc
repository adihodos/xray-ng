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

#include "xray/base/logger.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

void
xray::base::log(const LogLevel level, fmt::string_view format, fmt::format_args args)
{
    static thread_local char scratch_buffer[4096];
    const auto [itr, cch] = fmt::vformat_to_n(std::begin(scratch_buffer), std::size(scratch_buffer) - 1, format, args);
    *itr = 0;

    constexpr const spdlog::level::level_enum log_levels[] = {
        spdlog::level::trace, spdlog::level::debug, spdlog::level::info,
        spdlog::level::warn,  spdlog::level::err,   spdlog::level::critical,
    };

    spdlog::log(log_levels[static_cast<size_t>(level)], scratch_buffer);
}

void
xray::base::log_file_line(const LogLevel level,
                          const char* file,
                          const int32_t line,
                          fmt::string_view format,
                          fmt::format_args args)
{
    static thread_local char scratch_buffer[4096];
    const auto [itr, cch] =
        fmt::format_to_n(std::begin(scratch_buffer), std::size(scratch_buffer) - 1, "{}:{}\n", file, line);

    const auto [itr1, cch1] = fmt::vformat_to_n(itr, std::cend(scratch_buffer) - itr - 1, format, args);
    *itr1 = 0;

    constexpr const spdlog::level::level_enum log_levels[] = {
        spdlog::level::trace, spdlog::level::debug, spdlog::level::info,
        spdlog::level::warn,  spdlog::level::err,   spdlog::level::critical,
    };

    spdlog::log(log_levels[static_cast<size_t>(level)], scratch_buffer);
}

void
xray::base::setup_logging()
{
    spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    auto rotating_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("app.log", true);
    std::vector<spdlog::sink_ptr> sinks{ stdout_sink, rotating_sink };

    // auto logger = std::make_shared<spdlog::async_logger>(
    //     "xray-logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    auto logger = std::make_shared<spdlog::logger>("xray-logger", sinks.begin(), sinks.end());
    spdlog::set_default_logger(logger);
}
