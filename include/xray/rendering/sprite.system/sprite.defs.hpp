#pragma once

#include <cstdint>
#include <strong_type/strong_type.hpp>
#include <strong_type/bitarithmetic.hpp>
#include <strong_type/convertible_to.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/formattable.hpp>
#include <strong_type/hashable.hpp>

#include "xray/math/scalar2.hpp"

namespace xray::rendering {

using SpriteHandleType =
    strong::type<uint64_t, struct SpriteHandleTypeTag, strong::equality, strong::formattable, strong::hashable>;

struct SpriteAtlasEntry
{
    uint32_t layer;
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    SpriteHandleType hashed_name;
    xray::math::vec2f bottom_left;
    xray::math::vec2f top_left;
    xray::math::vec2f top_right;
    xray::math::vec2f bottom_right;
};

}
