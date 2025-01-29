#pragma once

#include "xray/math/scalar4.hpp"
#include <rfl.hpp>

namespace rfl {
template<typename T>
struct Reflector<xray::math::scalar4<T>>
{
    struct ReflType
    {
        T x;
        T y;
        T z;
        T w;
    };

    static xray::math::scalar4<T> to(const ReflType& v) noexcept { return { v.x, v.y, v.z, v.w }; }
    static ReflType from(const xray::math::scalar4<T>& v) { return { v.x, v.y, v.z, v.w }; }
};
}
