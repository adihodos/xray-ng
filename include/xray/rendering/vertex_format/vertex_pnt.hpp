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

#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/vertex_format/vertex_format.hpp"
#include "xray/xray.hpp"

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayMath_Rendering
/// @{

///
/// A vertex format, consisting of position and texture coordinates.
struct vertex_pnt
{

    vertex_pnt() noexcept {}

    vertex_pnt(const float px,
               const float py,
               const float pz,
               const float nx,
               const float ny,
               const float nz,
               const float tu,
               const float tv) noexcept
        : position{ px, py, pz }
        , normal{ nx, ny, nz }
        , texcoord{ tu, tv }
    {
    }

    vertex_pnt(const math::vec3f& pos, const math::vec3f& normal_vec, const math::vec2f& texc) noexcept
        : position{ pos }
        , normal{ normal_vec }
        , texcoord{ texc }
    {
    }

    static const char* name() noexcept { return "vertex_pnt"; }

    ///< Position in space.
    math::vec3f position;

    ///< Normal at vertex.
    math::vec3f normal;

    ///< Texture coordinates.
    math::vec2f texcoord;
};

template<>
struct vertex_format_traits<xray::rendering::vertex_format::pnt>
{
    using vertex_type = xray::rendering::vertex_pnt;
    static constexpr size_t bytes_size{ sizeof(vertex_pnt) };
    static constexpr uint32_t components{ 3 };

    static const VertexInputAttributeDescriptor* description()
    {
        static constexpr VertexInputAttributeDescriptor vdesc[] = {
            { 3, component_type::float_, XR_U32_OFFSETOF(vertex_pnt, position) },
            { 3, component_type::float_, XR_U32_OFFSETOF(vertex_pnt, normal) },
            { 2, component_type::float_, XR_U32_OFFSETOF(vertex_pnt, texcoord) }
        };

        return vdesc;
    }
};

/// @}

} // namespace rendering
} // namespace xray
