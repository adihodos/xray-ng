#pragma once

#include <xray/xray.hpp>

#include <cstdarg>
#include <span>

namespace xray::base {

struct xrFmtResult_t {
	char* out;
	I32 len;
};

XRAY_ATTRIBUTE_FORMAT(2, 3)
xrFmtResult_t format_to_n(std::span<char> dest, const char* fmtspec, ...);

xrFmtResult_t vformat_to_n(std::span<char> dest, const char* fmtspec, va_list fmt_args);

}  // namespace xray::base
