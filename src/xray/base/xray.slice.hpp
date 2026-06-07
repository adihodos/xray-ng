#pragma once

#include "xray/xray.hpp"

namespace xray::base {

struct slice_from {
	ISIZE start{};
};
struct slice_to {
	ISIZE end{};
};

struct slice_between {
	ISIZE start{};
	ISIZE end{};
};

//
// No constructors to make it an aggregate and use the aggregate init syntax
template <typename T>
struct xrSlice_t {
	T* s_ptr{nullptr};
	ISIZE s_len{};

	using value_type	 = T;
	using iterator		 = T*;
	using const_iterator = const T*;

	constexpr iterator begin() noexcept { return s_ptr; }
	constexpr iterator end() noexcept { return s_ptr + s_len; }
	constexpr const_iterator cbegin() const noexcept { return s_ptr; }
	constexpr const_iterator cend() const noexcept { return s_ptr + s_len; }

	T& operator[](const ISIZE idx) noexcept {
		//
		// TODO: bounds check
		return s_ptr[idx];
	}

	const T& operator[](const ISIZE idx) const noexcept { return s_ptr[idx]; }

	xrSlice_t<T> operator[](const slice_from from) const noexcept {
		// assert from.start >= 0 && <= s_len
		return xrSlice_t{
			.s_ptr = s_ptr + from.start,
			.s_len = s_len - from.start,
		};
	}

	xrSlice_t<T> operator[](const slice_to to) const noexcept {
		// assert to.end >= 0 && to.end <= s_len
		return xrSlice_t{
			.s_ptr = s_ptr,
			.s_len = to.end,
		};
	}

	xrSlice_t<T> operator[](const slice_between r) const noexcept {
		// assert(r.start <= r.end)
		// assert(r.start >= 00 && r.start <= s_len);

		return xrSlice_t{
			.s_ptr = s_ptr + r.start,
			.s_len = r.end - r.start,
		};
	}

	constexpr explicit operator bool() const noexcept { return s_ptr != nullptr && s_len != 0; }
	constexpr ISIZE size() const noexcept { return s_len; }
	constexpr T* data() noexcept { return s_ptr; }
	constexpr const T* data() const noexcept { return s_ptr; }
	constexpr const T* cdata() const noexcept { return s_ptr; }
	constexpr ISIZE size_bytes() const noexcept { return s_len * xr_size_of(T); }
};

template <typename T>
inline constexpr xrSlice_t<T> slice_from_ptr_and_len(T* pts, const ISIZE len) noexcept {
	return xrSlice_t<T>{
		.s_ptr{pts},
		.s_len{len},
	};
}

template <typename T>
inline constexpr xrSlice_t<const T> slice_from_ptr_and_len(const T* pts, const ISIZE len) noexcept {
	return xrSlice_t<const T>{
		.s_ptr{pts},
		.s_len{len},
	};
}

template <typename T>
inline constexpr xrSlice_t<T> slice_from_ptr_range(T* beg, T* end) noexcept {
	return xrSlice_t{
		.s_ptr{beg},
		.s_len{end - beg},
	};
}

template <typename T>
inline constexpr xrSlice_t<const T> slice_from_ptr_range(const T* beg, const T* end) noexcept {
	return xrSlice_t{
		.s_ptr{beg},
		.s_len{end - beg},
	};
}

template <typename SeqContainer>
	requires requires(SeqContainer c) {
		typename SeqContainer::value_type;
		c[0];
		c.data();
		c.size();
	}
inline constexpr xrSlice_t<const typename SeqContainer::value_type> slice_from_seq_container(
	const SeqContainer& cont
) noexcept {
	return xrSlice_t{
		.s_ptr = static_cast<const typename SeqContainer::value_type*>(cont.data()),
		.s_len = static_cast<ISIZE>(cont.size()),
	};
}

template <ISIZE N, typename T>
inline constexpr xrSlice_t<T> slice_from_array(T (&arr)[N]) noexcept {
	return xrSlice_t<T>{
		.s_ptr{&arr[0]},
		.s_len{N},
	};
}

template <ISIZE N, typename T>
inline constexpr xrSlice_t<const T> slice_from_array(const T (&arr)[N]) noexcept {
	return xrSlice_t<const T>{
		.s_ptr{&arr[0]},
		.s_len{N},
	};
}

template <typename T>
inline constexpr bool operator==(const xrSlice_t<T> lhs, const xrSlice_t<T> rhs) noexcept {
	return lhs.s_ptr == rhs.s_ptr && lhs.s_len == rhs.s_len;
}

template <typename T>
inline constexpr bool operator!=(const xrSlice_t<T> lhs, const xrSlice_t<T> rhs) noexcept {
	return !(lhs == rhs);
}

template <typename T>
inline constexpr ISIZE slice_bytes_size(const xrSlice_t<T> s) noexcept {
	return static_cast<ISIZE>(sizeof(T)) * s.s_len;
}

}  // namespace xray::base
