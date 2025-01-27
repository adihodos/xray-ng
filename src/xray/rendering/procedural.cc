//
// Copyright Adrian Hodos
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

#include "xray/rendering/procedural.hpp"

#include <cassert>

#include "xray/rendering/colors/hsv_color.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/colors/rgb_variants.hpp"
#include "xray/rendering/colors/color_cast_rgb_hsv.hpp"
#include "xray/rendering/colors/color_cast_rgb_variants.hpp"

std::vector<xray::math::vec4ui8>
xray::rendering::xor_pattern(const uint32_t width, const uint32_t height, const XorPatternType pattern)
{
    std::vector<math::vec4ui8> pixels{ width * height };

    // for (uint32_t y = 0; y < height; ++y) {
    //     for (uint32_t x = 0; x < width; ++x) {
    //         const uint8_t c = x ^ y;
    //         if (pattern == XorPatternType::BnW) {
    //             pixels[y * width + x] = math::vec4ui8{ c, c, c, 255 };
    //         } else if (pattern == XorPatternType::ColoredHSV) {
    //             const hsv_color hsv{ static_cast<float>(c) / 255.0f, 1.0f, 1.0f };
    //             const rgb_color rgb{ color_cast<rgb_color>(hsv) };
    //             const rgba_u32_color final{ color_cast<rgba_u32_color>(rgb) };
    //             pixels[y * width + x] = math::vec4ui8{ final.r, final.g, final.b, 255 };
    //         } else {
    //             pixels[y * width + x] = math::vec4ui8{ uint8_t(255 - c), c, uint8_t(c % 128), 255 };
    //         }
    //     }
    // }

    xor_fill(width,
             height,
             pattern,
             std::span{ reinterpret_cast<uint8_t*>(pixels.data()), pixels.size() * sizeof(pixels[0]) });
    return pixels;
}

void
xray::rendering::xor_fill(const uint32_t width,
                          const uint32_t height,
                          const XorPatternType pattern,
                          std::span<uint8_t> region)
{
    assert(width * height * 4 == region.size());

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t c = x ^ y;
            if (pattern == XorPatternType::BnW) {
                region[y * width * 4 + x * 4 + 0] = c;
                region[y * width * 4 + x * 4 + 1] = c;
                region[y * width * 4 + x * 4 + 2] = c;
                region[y * width * 4 + x * 4 + 3] = 255;
            } else if (pattern == XorPatternType::ColoredHSV) {
                const hsv_color hsv{ static_cast<float>(c) / 255.0f, 1.0f, 1.0f };
                const rgb_color rgb{ color_cast<rgb_color>(hsv) };
                const rgba_u32_color final{ color_cast<rgba_u32_color>(rgb) };
                region[y * width * 4 + x * 4 + 0] = final.r;
                region[y * width * 4 + x * 4 + 1] = final.g;
                region[y * width * 4 + x * 4 + 2] = final.b;
                region[y * width * 4 + x * 4 + 3] = 255;
            } else {
                region[y * width * 4 + x * 4 + 0] = uint8_t(255 - c);
                region[y * width * 4 + x * 4 + 1] = c;
                region[y * width * 4 + x * 4 + 2] = uint8_t(c % 128);
                region[y * width * 4 + x * 4 + 3] = 255;
            }
        }
    }
}
