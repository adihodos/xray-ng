#include "xray/rendering/colors/color_cast_rgb_hsl.hpp"
#include "xray/math/math_base.hpp"
#include "xray/math/math_std.hpp"
#include <cassert>
#include <cmath>

xray::rendering::rgb_color
xray::rendering::hsl_to_rgb(const hsl_color& hsl) noexcept {
  assert((hsl.h >= 0.0f) && (hsl.h <= 360.0f) &&
         "Invalid hue value, must be in [0, 360] range");

  assert((hsl.s >= 0.0f) && (hsl.s <= 1.0f) &&
         "Invalid saturation value, must be in [0, 1] range");

  assert((hsl.l >= 0.0f) && (hsl.l <= 1.0f) &&
         "Invalid luminance value, must be in [0, 1] range");

  const auto c_val = (1.0f - std::fabs(2.0f * hsl.l - 1.0f)) * hsl.s;
  const auto x_val =
      c_val * (1.0f - std::fabs(std::fmod((hsl.h / 60.0f), 2.0f) - 1.0f));
  const auto m_val = hsl.l - c_val * 0.5f;

  const auto selector = static_cast<int32_t>(hsl.h / 60.0f);

  const float clr_values[] = {c_val, x_val, 0.0f,  x_val, c_val, 0.0f,
                              0.0f,  c_val, x_val, 0.0f,  x_val, c_val,
                              x_val, 0.0f,  c_val, c_val, 0.0f,  x_val};

  assert(selector < 6);

  return {clr_values[selector * 3 + 0] + m_val,
          clr_values[selector * 3 + 1] + m_val,
          clr_values[selector * 3 + 2] + m_val};
}

xray::rendering::hsl_color
xray::rendering::rgb_to_hsl(const rgb_color& rgb) noexcept {
  const auto c_max = math::max(rgb.r, math::max(rgb.g, rgb.b));
  const auto c_min = math::min(rgb.r, math::min(rgb.g, rgb.b));
  const auto delta = c_max - c_min;

  using namespace math;

  hsl_color hsl;
  if (math::is_zero(delta)) {
    hsl.h = 0.0f;
  } else if (math::is_equal(c_max, rgb.r)) {
    hsl.h = 60.0f * std::fmod((rgb.g - rgb.b) / delta, 6.0f);
  } else if (math::is_equal(c_max, rgb.g)) {
    hsl.h = 60.0f * ((rgb.b - rgb.r) / delta + 2);
  } else if (math::is_equal(c_max, rgb.b)) {
    hsl.h = 60.0f * ((rgb.r - rgb.g) / delta + 4);
  }

  if (hsl.h < 0.0f)
    hsl.h += 360.0f;

  hsl.l = (c_max + c_min) * 0.5f;
  hsl.s = math::is_zero(delta)
              ? 0.0f
              : delta / (1.0f - std::fabs(2.0f * (hsl.l) - 1.0f));

  assert((hsl.s >= 0.0f) && (hsl.s <= 1.0f) && "Invalid saturation computed!");
  assert((hsl.l >= 0.0f) && (hsl.l <= 1.0f) && "Invalid luminance computed");
  assert((hsl.h >= 0.0f) && (hsl.h <= 360.0f) && "Invalid hue computed");

  return hsl;
}
