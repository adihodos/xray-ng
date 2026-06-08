#pragma once

//
// https://www.foonathan.net/2020/09/move-forward/

#include "xray/base/minstd/remove.hpp"

//
// static_cast to rvalue reference
#define XRAY_MOVE(...) static_cast<xray::minstd::remove_reference_t<decltype(__VA_ARGS__)>&&>(__VA_ARGS__)

//
// static_cast to identity
// The extra && aren't necessary as discussed above, but make it more robust in case it's used with a non-reference.
#define XRAY_FWD(...) static_cast<decltype(__VA_ARGS__)&&>(__VA_ARGS__)

namespace xray::minstd {
template <typename T, typename U = T>
inline constexpr T exchange(T& original, U&& updated) noexcept {
	T prev_val = XRAY_MOVE(original);
	original   = XRAY_FWD(updated);
	return prev_val;
}
	
}  // namespace xray::minstd
