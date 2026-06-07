#include <xray/base/xray.fmt.hpp>

#include <stb/stb_sprintf.h>

XRAY_ATTRIBUTE_FORMAT(2, 3)
xray::base::xrFmtResult_t xray::base::format_to_n(std::span<char> dest, const char* fmtspec, ...) {
	va_list args_ptr;
	va_start(args_ptr, fmtspec);
	const auto result = vformat_to_n(dest, fmtspec, args_ptr);
	va_end(args_ptr);
	return result;
}

xray::base::xrFmtResult_t xray::base::vformat_to_n(std::span<char> dest, const char* fmtspec, va_list fmt_args) {
	const I32 cch_out = stbsp_vsnprintf(dest.data(), static_cast<I32>(dest.size()), fmtspec, fmt_args);
	const I32 end_ptr = cch_out > 0 ? cch_out : 0;
	dest[end_ptr]	  = 0;

	return xrFmtResult_t{
		.out = dest.data() + end_ptr,
		.len = end_ptr,
	};
}
