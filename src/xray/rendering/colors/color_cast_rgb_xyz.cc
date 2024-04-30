#include "xray/rendering/colors/color_cast_rgb_xyz.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include <cassert>
#include <cmath>

xray::rendering::xyz_color
xray::rendering::rgb_to_xyz(const rgb_color& rgb) noexcept
{

    auto correct_color = [](const float input_val) {
        if (input_val <= 0.04045f) {
            return input_val / 12.92f;
        }

        const float constant_val = 0.055f;
        return std::pow((input_val + constant_val) / (1.0f + constant_val), 2.4f);
    };

    const float red_val = correct_color(rgb.r);
    const float green_val = correct_color(rgb.g);
    const float blue_val = correct_color(rgb.b);

    return { 0.4124f * red_val + 0.3576f * green_val + 0.1805f * blue_val,
             0.2126f * red_val + 0.7152f * green_val + 0.0722f * blue_val,
             0.0193f * red_val + 0.1192f * green_val + 0.9505f * blue_val };
}

xray::rendering::rgb_color
xray::rendering::xyz_to_rgb(const xyz_color& xyz) noexcept
{
    const float x = xyz.x;
    const float y = xyz.y;
    const float z = xyz.z;

    const float r_linear = math::clamp(3.2406f * x - 1.5372f * y - 0.4986f * z, 0.0f, 1.0f);
    const float g_linear = math::clamp(-0.9689f * x + 1.8758f * y + 0.0415f * z, 0.0f, 1.0f);
    const float b_linear = math::clamp(0.0557f * x - 0.2040f * y + 1.0570f * z, 0.0f, 1.0f);

    auto correct = [](const float cl) {
        const float a = 0.055f;

        if (cl <= 0.0031308f) {
            return 12.92f * cl;
        }

        return (1.0f + a) * std::pow(cl, 1.0f / 2.4f) - a;
    };

    return { correct(r_linear), correct(g_linear), correct(b_linear), 1.0f };
}
