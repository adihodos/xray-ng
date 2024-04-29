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
#include "xray/math/scalar3.hpp"
#include <cstdio>
#include <stlsoft/memory/auto_buffer.hpp>
#include <string>

#define XRAY_MATH_ENABLE_FMT_SUPPORT
#if defined(XRAY_MATH_ENABLE_FMT_SUPPORT)
#include <fmt/core.h>
#include <fmt/format.h>
#endif

namespace xray {
namespace math {

template <typename T>
std::string string_cast(const scalar3<T>& s) {
  stlsoft::auto_buffer<char, 256u> tmp_buff{256u};
  snprintf(
    tmp_buff.data(), tmp_buff.size(), "scalar3 [%f, %f, %f]", s.x, s.y, s.z);

  return static_cast<const char*>(tmp_buff.data());
}

} // namespace math
} // namespace xray

#if defined(XRAY_MATH_ENABLE_FMT_SUPPORT)

namespace fmt {

// template <typename T>
// struct formatter<xray::math::scalar3<T>> {
//   template <typename ParseContext>
//   constexpr auto parse(ParseContext& ctx);
//
//   template <typename FormatContext>
//   auto format(const xray::math::scalar3<T>& val, FormatContext& ctx);
// };
//
// template <typename T>
// template <typename ParseContext>
// constexpr auto formatter<xray::math::scalar3<T>>::parse(ParseContext& ctx) {
//   return ctx.begin();
// }
//
// template <typename T>
// template <typename FormatContext>
// auto formatter<xray::math::scalar3<T>>::format(const xray::math::scalar3<T>&
// val,
//                                         FormatContext&    ctx) {
//   return fmt::format_to(
//     ctx.begin(), "scalar3 = {.x = {}, .y = {}, .z = {}}", val.x, val.y,
//     val.z);
// }

template <typename T>
struct formatter<xray::math::scalar3<T>> : nested_formatter<T> {

  // Formats value using the parsed format specification stored in this
  // formatter and writes the output to ctx.out().
  auto format(const xray::math::scalar3<T>& value, format_context& ctx) const {
    return this->write_padded(ctx, [=](auto out) {
      return fmt::format_to(out,
                            "scalar3 [ .x = {}, .y = {}, .z = {} ]",
                            this->nested(value.x),
                            this->nested(value.y),
                            this->nested(value.z));
    });
  }
};

} // namespace fmt

#endif
