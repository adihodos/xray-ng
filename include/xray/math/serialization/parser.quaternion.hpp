#pragma once

#include "xray/math/quaternion.hpp"
#include <rfl.hpp>

namespace rfl {
template<typename T>
struct Reflector<xray::math::quaternion<T>>
{
    struct ReflType
    {
        T x;
        T y;
        T z;
        T w;
    };

    static xray::math::quaternion<T> to(const ReflType& v) noexcept { return { v.x, v.y, v.z, v.w }; }
    static ReflType from(const xray::math::quaternion<T>& v) { return { v.x, v.y, v.z, v.w }; }
};
}
