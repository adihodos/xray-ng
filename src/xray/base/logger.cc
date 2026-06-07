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

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#else
#endif

#include <stb/stb_sprintf.h>

#include "xray/base/array_dimension.hpp"
#include "xray/base/minstd/algo.hpp"
#include "xray/base/syscall_wrapper.hpp"
#include "xray/base/xray.debug.hpp"

namespace {

struct xrEscapeCodeRange_t {
	const char* start;
	xray::ISIZE len;
} constexpr const ColorsByLogLevel[] = {
#define XR_COLOR_ENTRY(color_esc_code) \
	xrEscapeCodeRange_t { .start = color_esc_code, .len = xray::base::xr_array_size(color_esc_code) }
	//
	// trace
	XR_COLOR_ENTRY("\033[1;34m"),
	XR_COLOR_ENTRY("\033[1;33m"),
	XR_COLOR_ENTRY("\033[1;32m"),
	XR_COLOR_ENTRY("\033[1;36m"),
	XR_COLOR_ENTRY("\033[1;35m"),
	XR_COLOR_ENTRY("\033[1;31m"),
	XR_COLOR_ENTRY("\033[1;37m"),
};

constexpr xrEscapeCodeRange_t ColorsEnd = {
	XR_COLOR_ENTRY("\033[0m"),
};
#undef XR_COLOR_ENTRY

static_assert(
	(static_cast<xray::ISIZE>(xray::base::LogLevel::Off) + 1) == xray::base::xr_array_size(ColorsByLogLevel),
	"Mismatch between LogLevel enum and colors array"
);

struct xrHdrTagByLevel_t {
	const char* hdr;
	xray::ISIZE len;
} constexpr const HeaderTagsByLogLevel[] = {
#define HDR_TAG_ENTRY(tag) \
	xrHdrTagByLevel_t { .hdr = tag, .len = xray::base::xr_array_size(tag), }
	HDR_TAG_ENTRY("[-trace-]"),
	HDR_TAG_ENTRY("[-debug-]"),
	HDR_TAG_ENTRY("[-info-]"),
	HDR_TAG_ENTRY("[-warn-]"),
	HDR_TAG_ENTRY("[-error-]"),
	HDR_TAG_ENTRY("[-critical-]"),
	HDR_TAG_ENTRY(""),
};

#undef HDR_TAG_ENTRY

static_assert(
	(static_cast<xray::ISIZE>(xray::base::LogLevel::Off) + 1) == xray::base::xr_array_size(HeaderTagsByLogLevel),
	"Mismatch between LogLevel enum and colors array"
);

void xrSys_DebugPrintFileLineVAArgs(
	const xray::base::LogLevel level, const char* file, const xray::I32 line, const char* format, va_list args_ptr
) {
	char scratch_buffer[4096];
	const xray::ISIZE hdr_len = stbsp_snprintf(scratch_buffer, XR_I32_COUNTOF(scratch_buffer), "[%s:%d] ", file, line);

	const xray::ISIZE body_len =
		stbsp_vsnprintf(&scratch_buffer[0] + hdr_len, XR_I32_COUNTOF(scratch_buffer) - hdr_len, format, args_ptr);
	va_end(args_ptr);

	if (body_len > 0) {
		xray::base::xrSys_DebugPrintRaw(level, scratch_buffer, hdr_len + body_len);
	}
}

void xrSys_DebugPrintVAArgs(const xray::base::LogLevel level, const char* format, va_list args_ptr) {
	char scratch_buffer[4096];

	const xray::ISIZE cch = stbsp_vsnprintf(scratch_buffer, XR_I32_COUNTOF(scratch_buffer), format, args_ptr);
	va_end(args_ptr);
	if (cch > 0) {
		xray::base::xrSys_DebugPrintRaw(level, scratch_buffer, cch);
	}
}

}  // namespace

void xray::base::xrSys_DebugPrintRawMultipleArgs(
	const LogLevel lvl, const xrDebugPrintRawArgs_t* args, const ISIZE args_len
) {
	iovec writes_array[16];
	for (ISIZE idx = 0; idx < args_len; idx += 16) {
		ISIZE batch_writes = 0;
		for (; batch_writes < minstd::min_of(args_len, xr_array_size(writes_array)); ++batch_writes) {
			writes_array[batch_writes] = iovec{
				.iov_base = (void*)args[idx + batch_writes].txt_args,
				.iov_len  = (size_t)args[idx + batch_writes].txt_args_len,
			};
		}
		syscall_wrapper(writev, STDERR_FILENO, writes_array, static_cast<I32>(batch_writes));
	}
}

void xray::base::xrSys_DebugPrintRaw(const LogLevel log_lvl, const char* txt, const ISIZE len) {
	const ISIZE lvl_select					   = static_cast<ISIZE>(log_lvl);
	const xrDebugPrintRawArgs_t writes_array[] = {
		xrDebugPrintRawArgs_t{
			.txt_args	  = ColorsByLogLevel[lvl_select].start,
			.txt_args_len = ColorsByLogLevel[lvl_select].len,
		},
		xrDebugPrintRawArgs_t{
			.txt_args	  = HeaderTagsByLogLevel[lvl_select].hdr,
			.txt_args_len = HeaderTagsByLogLevel[lvl_select].len,
		},
		xrDebugPrintRawArgs_t{
			.txt_args	  = ColorsEnd.start,
			.txt_args_len = ColorsEnd.len,
		},
		xrDebugPrintRawArgs_t{
			.txt_args	  = txt,
			.txt_args_len = len,
		},
		xrDebugPrintRawArgs_t{
			.txt_args	  = "\n",
			.txt_args_len = 1,
		},
	};

	xrSys_DebugPrintRawMultipleArgs(log_lvl, writes_array, xr_array_size(writes_array));
}

XRAY_ATTRIBUTE_FORMAT(3, 4)
void xray::xrSys_DebugPrintFileLine(const char* file, const xray::I32 line, const char* fmtspec, ...) {
	va_list args_ptr;
	va_start(args_ptr, fmtspec);
	xrSys_DebugPrintFileLineVAArgs(xray::base::LogLevel::Debug, file, line, fmtspec, args_ptr);
}

XRAY_ATTRIBUTE_FORMAT(1, 2)
void xray::xrSys_DebugPrint(const char* fmt_spec, ...) {
	va_list args_ptr;
	va_start(args_ptr, fmt_spec);
	xrSys_DebugPrintVAArgs(xray::base::LogLevel::Debug, fmt_spec, args_ptr);
}

void xray::base::log(const LogLevel level, const char* format, ...) {
	va_list args_ptr;
	va_start(args_ptr, format);
	xrSys_DebugPrintVAArgs(level, format, args_ptr);
}

void xray::base::log_file_line(const LogLevel level, const char* file, const I32 line, const char* format, ...) {
	va_list args_ptr;
	va_start(args_ptr, format);
	xrSys_DebugPrintFileLineVAArgs(level, file, line, format, args_ptr);
}

void xray::base::setup_logging(const LogLevel log_lvl) {
	// spdlog::init_thread_pool(8192, 1);
	// const std::string log_pattern{"[%H:%M:%S %z] [%n] [%^-%L-%$] [thread %t] %v"};
	// auto stdout_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
	// stdout_sink->set_pattern(log_pattern);
	//
	// const auto local_time = std::chrono::system_clock::now();
	// char temp_buffer[1024];
	// format_to_n(temp_buffer, "xray.{:%d-%m-%Y-%H-%M-%S}.log", local_time);
	//
	// const auto log_file_path = ConfigSystem::instance()->FileSys.RootPathAbsolute / temp_buffer;
	//
	// auto rotating_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_path.generic_string(), true);
	// rotating_sink->set_pattern(log_pattern);
	// std::vector<spdlog::sink_ptr> sinks{stdout_sink, rotating_sink};
	//
	// // auto logger = std::make_shared<spdlog::async_logger>(
	// //     "xray-logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	// auto logger = std::make_shared<spdlog::logger>("xray-logger", sinks.begin(), sinks.end());
	// spdlog::set_default_logger(logger);
	// spdlog::set_level(static_cast<spdlog::level::level_enum>(log_lvl));
}
