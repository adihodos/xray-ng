//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "xray/xray.hpp"
#include "xray/rendering/colors/color_cast.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/colors/rgb_variants.hpp"
#include <cmath>
#include <cstdint>

namespace xray {
namespace rendering {

template <>
struct color_caster<rgb_color, rgba_u32_color> {

  static rgb_color cast(const rgba_u32_color& rgba) noexcept {

    const auto red   = static_cast<uint8_t>((rgba.value >> 24) & 0xFF);
    const auto green = static_cast<uint8_t>((rgba.value >> 16) & 0xFF);
    const auto blue  = static_cast<uint8_t>((rgba.value >> 8) & 0xFF);
    const auto alpha = static_cast<uint8_t>(rgba.value & 0xFF);

    return {
        static_cast<float>(red) / 255.0f, static_cast<float>(green) / 255.0f,
        static_cast<float>(blue) / 255.0f, static_cast<float>(alpha) / 255.0f};
  }
};

template <>
struct color_caster<rgba_u32_color, rgb_color> {

  static rgba_u32_color cast(const rgb_color& rgb) noexcept {

    const auto red   = static_cast<uint32_t>(std::ceil(255.0f * rgb.r)) << 24;
    const auto green = static_cast<uint32_t>(std::ceil(255.0f * rgb.g)) << 16;
    const auto blue  = static_cast<uint32_t>(std::ceil(255.0f * rgb.b)) << 8;
    const auto alpha = static_cast<uint32_t>(std::ceil(255.0f * rgb.a));

    return rgba_u32_color{red | green | blue | alpha};
  }
};

} // namespace rendering
} // namespace xray
