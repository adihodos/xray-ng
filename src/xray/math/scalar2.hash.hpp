#pragma once

#include <string_view>
#include "xray/base/fnv_hash.hpp"
#include "xray/math/scalar2.hpp"

namespace std {
template<typename T>
struct hash<xray::math::scalar2<T>>
{
    std::size_t operator()(const xray::math::scalar2<T>& s) const noexcept
    {
        if constexpr (std::is_integral_v<T>) {
            return FNV::fnv1a(static_cast<const void*>(&s), sizeof(s));
        } else {
            static_assert(false, "Do something man");
        }
    };
};
}
