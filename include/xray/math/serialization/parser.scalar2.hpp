#pragma once

#include "xray/math/scalar2.hpp"
#include <rfl.hpp>

namespace rfl {
template<typename T>
struct Reflector<xray::math::scalar2<T>>
{
    struct ReflType
    {
        T x;
        T y;
    };

    static xray::math::scalar2<T> to(const ReflType& v) noexcept { return { v.x, v.y }; }
    static ReflType from(const xray::math::scalar2<T>& v) { return { v.x, v.y }; }
};
}
