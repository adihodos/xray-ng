#pragma once

#include <cstring>

#include "xray/xray.hpp"
#include "xray/base/xray.slice.hpp"

#define PRI_xrStringView_t ".*s"
#define FMT_xrStringView_t(sv) (static_cast<xray::I32>(sv.size())), sv.data()

namespace xray::base {

using xrStringView_t = xrSlice_t<const C8>;

inline xrStringView_t string_view_from_c_str(const char* c_str) noexcept {
	return xrStringView_t{
		.s_ptr = c_str,
		.s_len = static_cast<ISIZE>(strlen(c_str)),
	};
}

inline xrStringView_t string_view_from_c_str_with_len(const char* c_str, const ISIZE len) noexcept {
	return slice_from_ptr_and_len(c_str, len);
}

}  // namespace xray::base
