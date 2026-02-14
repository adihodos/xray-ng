#pragma once

#include <fmt/format.h>
#include "au/au.hh"

namespace fmt {
template <typename U, typename R>
struct formatter<::au::Quantity<U, R>> : ::au::QuantityFormatter<U, R, ::fmt::formatter> {};

template <typename U, typename R>
struct formatter<::au::QuantityPoint<U, R>>
    : ::au::QuantityPointFormatter<U, R, ::fmt::formatter> {};
}  // namespace fmt
