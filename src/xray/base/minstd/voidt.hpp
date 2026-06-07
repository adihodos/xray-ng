#pragma once

namespace xray::minstd {

template <typename...>
using void_t = void;

template <typename T>
constexpr bool dependant_always_false_v = false;

}  // namespace xray::minstd
