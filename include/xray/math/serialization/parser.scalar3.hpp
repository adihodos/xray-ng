
#pragma once

#include "xray/math/scalar3.hpp"
#include <rfl.hpp>

namespace rfl {
template<typename T>
struct Reflector<xray::math::scalar3<T>>
{
    struct ReflType
    {
        T x;
        T y;
        T z;
    };

    static xray::math::scalar3<T> to(const ReflType& v) noexcept { return { v.x, v.y, v.z }; }
    static ReflType from(const xray::math::scalar3<T>& v) { return { v.x, v.y, v.z }; }
};
}
