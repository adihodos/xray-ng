#pragma once

#include "xray/base/minstd/add.hpp"

namespace xray::minstd {

template <typename T>
auto declval() noexcept -> add_rvalue_reference_t<T>;

}  // namespace xray::minstd
