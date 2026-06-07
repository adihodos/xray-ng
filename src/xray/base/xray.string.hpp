#pragma once

#include <cstring>

#include "xray/xray.hpp"
#include "xray/base/memory.arena.hpp"

namespace xray::base {

struct xrString_t {
	C8* s_ptr{};
	ISIZE s_len{};
	ISIZE s_cap{};

	using value_type	 = C8;
	using iterator		 = C8*;
	using const_iterator = const C8*;

	constexpr iterator begin() noexcept { return s_ptr; }
	constexpr iterator end() noexcept { return s_ptr + s_len; }
	constexpr const_iterator cbegin() const noexcept { return s_ptr; }
	constexpr const_iterator cend() const noexcept { return s_ptr + s_len; }

	explicit constexpr operator bool() const noexcept { return s_ptr != nullptr; }

	const C8* data() const noexcept { return s_ptr; }
	C8* data() noexcept { return s_ptr; }
	const C8* cdata() const noexcept { return s_ptr; }
	ISIZE size() const noexcept { return s_len; }
	ISIZE capacity() const noexcept { return s_cap; }
};

inline xrString_t string_from_c_str_with_len(MemoryArena& arena, const char* c_str, const ISIZE len) noexcept {
	xrSlice_t<C8> s = arena.alloc_align<C8>(len);
	if (s) {
		memcpy(s.s_ptr, c_str, static_cast<size_t>(len));
		return xrString_t{
			.s_ptr = s.s_ptr,
			.s_len = s.s_len,
			.s_cap = len,
		};
	}

	return xrString_t{};
}

inline xrString_t string_from_c_str(MemoryArena& arena, const char* s_ptr) noexcept {
	return string_from_c_str_with_len(arena, s_ptr, static_cast<ISIZE>(strlen(s_ptr)));
}

inline xrString_t string_clone(MemoryArena& arena, const xrString_t src) noexcept {
	return string_from_c_str_with_len(arena, src.s_ptr, src.s_len);
}

}  // namespace xray::base
