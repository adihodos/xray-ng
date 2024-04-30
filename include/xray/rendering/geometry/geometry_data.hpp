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

/// \file   geometry_data.hpp    Class with geometric data.

#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vertex_format/vertex_pntt.hpp"
#include "xray/xray.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace xray {
namespace rendering {

/// \addtogroup __GroupXrayRendering
/// @{

///     Stores geometry data (vertices and conectivity information).
struct geometry_data_t
{
    template<typename vector_type>
    using scoped_vector_array_t = xray::base::unique_pointer<vector_type[]>;

    using size_type = size_t;

    geometry_data_t(geometry_data_t&&) = default;

    geometry_data_t() noexcept = default;

    geometry_data_t(scoped_vector_array_t<vertex_pntt> vertices,
                    scoped_vector_array_t<uint32_t> idx,
                    const size_type vtx_count,
                    const size_type idx_count) noexcept
        : geometry{ std::move(vertices) }
        , indices{ std::move(idx) }
        , vertex_count{ vtx_count }
        , index_count{ idx_count }
    {
    }

    geometry_data_t(const size_type num_vertices, const size_type num_indices) { setup(num_vertices, num_indices); }

    void setup(const size_type num_vertices, const size_type num_indices)
    {
        vertex_count = num_vertices;
        index_count = num_indices;
        geometry = scoped_vector_array_t<vertex_pntt>{ new vertex_pntt[num_vertices] };
        indices = scoped_vector_array_t<uint32_t>{ new uint32_t[num_indices] };
    }

    explicit operator bool() const noexcept { return vertex_count != 0 && index_count != 0; }

    std::span<vertex_pntt> vertex_span() noexcept
    {
        assert(vertex_count != 0);
        return std::span{ xray::base::raw_ptr(geometry), static_cast<ptrdiff_t>(vertex_count) };
    }

    std::span<const vertex_pntt> vertex_span() const noexcept
    {
        assert(vertex_count != 0);
        return std::span{ xray::base::raw_ptr(geometry), static_cast<ptrdiff_t>(vertex_count) };
    }

    std::span<const uint32_t> index_span() const noexcept
    {
        assert(index_count != 0);
        return std::span{ xray::base::raw_ptr(indices), static_cast<ptrdiff_t>(index_count) };
    }

    vertex_pntt* vertex_data() noexcept
    {
        assert(vertex_count != 0);
        return xray::base::raw_ptr(geometry);
    }

    const vertex_pntt* vertex_data() const noexcept
    {
        assert(vertex_count != 0);
        return xray::base::raw_ptr(geometry);
    }

    uint32_t* index_data() noexcept
    {
        assert(index_count != 0);
        return xray::base::raw_ptr(indices);
    }

    const uint32_t* index_data() const noexcept
    {
        assert(index_count != 0);
        return xray::base::raw_ptr(indices);
    }

    size_type vertex_count{ 0 };
    size_type index_count{ 0 };
    scoped_vector_array_t<vertex_pntt> geometry;
    scoped_vector_array_t<uint32_t> indices;

  private:
    XRAY_NO_COPY(geometry_data_t);
};

/// @}

} // namespace rendering
} // namespace xray
