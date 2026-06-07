#pragma once

namespace xray::minstd {
	
template <typename T>
constexpr inline T min_of(const T& a, const T& b) noexcept {
	return a < b ? a : b;
}

template <typename T>
constexpr inline T max_of(const T& a, const T& b) noexcept {
	return a > b ? a : b;
}

template <typename T>
constexpr inline T clamp(const T& val, const T& min_val, const T& max_val) noexcept {
	return val < min_val ? min_val : val > max_val ? max_val : val;
}

}  // namespace xray::minstd
