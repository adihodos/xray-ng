#pragma once

#include <cstdint>
#include "xray/math/scalar3.hpp"

namespace xray::rendering {

enum class ShapeKind : uint8_t
{
    Disc,
    Square,
    Triangle,
    Diamond,
    Chevron,
    Ring,
    Tag,
    Cross,
    Asterisk,
    Infinity,
    BlockArrow,
};

struct alignas(16) ShapeSetup
{
    xray::math::vec3f32 position;
    float size;
    float cos_theta;
    float sin_theta;
    float line_width;
    float antialias;
    uint32_t fg_color;
    uint32_t bg_color;
    uint32_t shape_kind;
};

}
