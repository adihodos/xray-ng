#include "xray/rendering/colors/color_cast_rgb_hsv.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include <cmath>
#include <limits>

xray::rendering::hsv_color
xray::rendering::rgb_to_hsv(const rgb_color& rgb) noexcept
{
    const float max = math::max(math::max(rgb.r, rgb.g), rgb.b);
    const float min = math::min(math::min(rgb.r, rgb.g), rgb.b);

    hsv_color hsv;
    hsv.v = max;

    if (max) {
        hsv.s = (max - min) / max;
    } else {
        hsv.s = 0.0f;
        hsv.h = std::numeric_limits<float>::max();
        return hsv;
    }

    const float delta = max - min;

    if (rgb.r == max) {
        hsv.h = (rgb.g - rgb.b) / delta;
    } else if (rgb.g == max) {
        hsv.h = 2.0f + (rgb.b - rgb.r) / delta;
    } else {
        hsv.h = 4.0f + (rgb.r - rgb.g) / delta;
    }

    hsv.h *= 60.0f;

    if (hsv.h < 0.0f) {
        hsv.h += 360.0f;
    }

    return hsv;
}

xray::rendering::rgb_color
xray::rendering::hsv_to_rgb(const hsv_color& hsv) noexcept
{

    if (math::is_zero(hsv.s)) {
        return { hsv.v, hsv.v, hsv.v, 1.0f };
    }

    //
    // Make hue to be in the [0, 6) range.
    const float hue = math::is_equal(hsv.h, 360.0f) ? 0.0f : (hsv.h / 60.0f);

    //
    // Get integer and fractional part of hue.
    const int32_t int_part = static_cast<int32_t>(floor(hue));
    const float frac_part = hue - static_cast<float>(int_part);

    const float p = hsv.v * (1.0f - hsv.s);

    const float q = hsv.v * (1.0f - (hsv.s * frac_part));

    const float t = hsv.v * (1.0f - (hsv.s * (1.0f - frac_part)));

    const float color_table[6 * 3] = { //
                                       // Case 0
                                       hsv.v,
                                       t,
                                       p,
                                       //
                                       // Case 1
                                       q,
                                       hsv.v,
                                       p,
                                       //
                                       // Case 2
                                       p,
                                       hsv.v,
                                       t,
                                       //
                                       // Case 3
                                       p,
                                       q,
                                       hsv.v,
                                       //
                                       // Case 4
                                       t,
                                       p,
                                       hsv.v,
                                       //
                                       // Case 5
                                       hsv.v,
                                       p,
                                       q
    };

    rgb_color rgb;

    rgb.r = color_table[int_part * 3 + 0];
    rgb.g = color_table[int_part * 3 + 1];
    rgb.b = color_table[int_part * 3 + 2];
    rgb.a = 1.0f;

    return rgb;
}
