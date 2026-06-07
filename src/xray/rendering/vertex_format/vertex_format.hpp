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
#include <cstdint>
#include <concepts>
#include <type_traits>

namespace xray {
namespace rendering {

enum class vertex_format
{
    undefined,
    p,
    pn,
    pt,
    pnt,
    pntt
};

enum class index_format
{
    u16,
    u32
};

struct component_type
{
    enum
    {
        signed_byte,
        unsigned_byte,
        signed_short,
        unsigned_short,
        signed_int,
        unsigned_int,
        float_,
        double_
    };
};

inline constexpr uint32_t
component_type_size(const uint32_t c) noexcept
{
    switch (c) {
        case component_type::signed_byte:
        case component_type::unsigned_byte:
            return 1;

        case component_type::signed_short:
        case component_type::unsigned_short:
            return 2;

        case component_type::signed_int:
        case component_type::unsigned_int:
        case component_type::float_:
            return 4;

        case component_type::double_:
            return 8;

        default:
            return 1;
    }
}

struct VertexInputAttributeDescriptor
{
    uint32_t component_count;
    uint32_t component_type;
    uint32_t component_offset;
    uint32_t location;
    uint32_t binding;
};

template<xray::rendering::vertex_format fmt>
struct vertex_format_traits;

// template<typename T>
// concept GraphicsPipelineVertex = requires(T) {
//     std::is_bounded_array_v<typename T::description>&&
//         std::is_same_v<decltype(T::description[0]), VertexInputAttributeDescriptor>;
// };
//
} // namespace rendering
} // namespace rendering
