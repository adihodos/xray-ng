#include "xray/base/xray.fmt.arena.hpp"

#include <cstdarg>
#include <stb/stb_sprintf.h>

#include "xray/base/xray.slice.hpp"

enum class xrStringContainerType_t {
	StdString,
	ArenaString,
};

struct xrStbSprintfContext_t {
	xrStringContainerType_t cont_type;
	union {
		xray::base::containers::string* dst_ar;
		std::string* dst_std;
	};
	xray::base::xrSlice_t<xray::C8> scratch_buf;
};

char* xrStbSprint_Callback(char const* buf, void* user, int len) {
	xrStbSprintfContext_t* ctx = static_cast<xrStbSprintfContext_t*>(user);
	if (ctx->cont_type == xrStringContainerType_t::ArenaString) {
		ctx->dst_ar->reserve(static_cast<size_t>(len));
		ctx->dst_ar->append(buf, static_cast<size_t>(len));
	} else {
		ctx->dst_std->reserve(static_cast<size_t>(len));
		ctx->dst_std->append(buf, static_cast<size_t>(len));
	}
	return ctx->scratch_buf.data();
}

XRAY_ATTRIBUTE_FORMAT(2, 3)
void xray::base::format_to(xray::base::containers::string& str, const char* fmt_spec, ...) {
	char temp_scratch[STB_SPRINTF_MIN];
	xrStbSprintfContext_t ctx{
		.cont_type	 = xrStringContainerType_t::ArenaString,
		.dst_ar		 = &str,
		.scratch_buf = xray::base::slice_from_array(temp_scratch),
	};

	va_list args_ptr;
	va_start(args_ptr, fmt_spec);
	stbsp_vsprintfcb(xrStbSprint_Callback, &ctx, temp_scratch, fmt_spec, args_ptr);
	va_end(args_ptr);
}

XRAY_ATTRIBUTE_FORMAT(1, 2)
std::string xray::base::format_to_string(const char* fmt_spec, ...) {
	std::string str;
	str.reserve(32);

	char temp_scratch[STB_SPRINTF_MIN];
	xrStbSprintfContext_t ctx{
		.cont_type	 = xrStringContainerType_t::StdString,
		.dst_std	 = &str,
		.scratch_buf = xray::base::slice_from_array(temp_scratch),
	};

	va_list args_ptr;
	va_start(args_ptr, fmt_spec);
	stbsp_vsprintfcb(xrStbSprint_Callback, &ctx, temp_scratch, fmt_spec, args_ptr);
	va_end(args_ptr);

	return str;
}

XRAY_ATTRIBUTE_FORMAT(2, 3)
xray::base::containers::string xray::base::format_to_string(xray::base::MemoryArena& arena, const char* fmt_spec, ...) {
	containers::string str{arena};
	str.reserve(32);

	char temp_scratch[STB_SPRINTF_MIN];
	xrStbSprintfContext_t ctx{
		.cont_type	 = xrStringContainerType_t::ArenaString,
		.dst_ar		 = &str,
		.scratch_buf = xray::base::slice_from_array(temp_scratch),
	};

	va_list args_ptr;
	va_start(args_ptr, fmt_spec);
	stbsp_vsprintfcb(xrStbSprint_Callback, &ctx, temp_scratch, fmt_spec, args_ptr);
	va_end(args_ptr);

	return str;
}
