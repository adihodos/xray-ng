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

#include "xray/math/scalar3.hpp"
#include "xray/xray.hpp"

#if defined(XRAY_MATH_ENABLE_FMT_SUPPORT)
#include <fmt/core.h>
#include <fmt/format.h>
#endif

#if defined(XRAY_MATH_ENABLE_FMT_SUPPORT)

namespace fmt {

template<typename T>
struct formatter<xray::math::scalar3<T>> : nested_formatter<T>
{

    // Formats value using the parsed format specification stored in this
    // formatter and writes the output to ctx.out().
    auto format(const xray::math::scalar3<T>& value, format_context& ctx) const
    {
        return this->write_padded(ctx, [this, value](auto out) {
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
