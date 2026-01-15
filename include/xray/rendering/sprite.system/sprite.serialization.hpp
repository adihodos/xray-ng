#pragma once

#include "xray/rendering/sprite.system/sprite.defs.hpp"
#include <rfl.hpp>

namespace rfl {
template<>
struct Reflector<xray::rendering::SpriteHandleType>
{
    using ReflType = uint64_t;

    static xray::rendering::SpriteHandleType to(const ReflType& v) noexcept
    {
        return xray::rendering::SpriteHandleType{ v };
    }
    static ReflType from(const xray::rendering::SpriteHandleType& v) { return v.value_of(); }
};

}
