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

/// \file   mesh.hpp    Simple class to represent a mesh.

#include "xray/xray.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "xray/base/maybe.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <mio/mmap.hpp>
#include <tl/optional.hpp>

namespace xray {
namespace rendering {

/// \brief Describes the data in a binary mesh file.
struct mesh_header
{
    uint16_t ver_major;
    uint16_t ver_minor;
    ///< Number of vertices in the mesh.
    uint32_t vertex_count;
    ///< Size in bytes of a vertex.
    uint32_t vertex_size;
    ///< Number of indices.
    uint32_t index_count;
    ///< Size in bytes of an index element.
    uint32_t index_size;
    ///< Offset in bytes, from the start of file, where vertex data begins.
    uint32_t vertex_offset;
    ///< Offset in bytes, from the start of file, where index data begins.
    uint32_t index_offset;
};

struct mesh_bounding
{
    xray::math::aabb3f axis_aligned_bbox;
    xray::math::sphere3f bounding_sphere;
};

/// \brief  Loading options for binary meshes. These can be OR'ed together.
struct mesh_load_option
{
    enum
    {
        ///< Remove any points or line entities from the mesh.
        remove_points_lines = 1u << 1,
        ///< Convert faces to clockwise winding.
        convert_left_handed = 1u << 2
    };
};

/// \brief  Loads a mesh in binary format. The mesh file consists of a header
/// \see mesh_header structure plus vertex and index data. The vertices are
/// of type vertex_pnt. The indices are of uint32_t type.
class mesh_loader
{
  public:
    mesh_loader() = delete;
    mesh_loader(mesh_loader&&) = default;
    mesh_loader& operator=(mesh_loader&&) = default;

    explicit mesh_loader(mio::mmap_source mmfile) noexcept
        : _mfile{ std::move(mmfile) }
    {
    }

    static tl::optional<mesh_loader> load(const char* file_path,
                                          const uint32_t load_opts = mesh_load_option::remove_points_lines);

    ///   \brief  Returns the header data of the mesh file.
    const mesh_header& header() const noexcept { return *reinterpret_cast<const mesh_header*>(_mfile.data()); }

    const mesh_bounding& bounding() const noexcept
    {
        return *reinterpret_cast<const mesh_bounding*>(reinterpret_cast<const uint8_t*>(_mfile.data()) +
                                                       sizeof(mesh_header));
    }

    ///   \brief  Returns a pointer to the mesh data.
    const void* get_data() const noexcept { return _mfile.data(); }

    ///   \brief  Returns a span that can be used to iterate over the vertices.
    std::span<const vertex_pnt> vertex_span() const noexcept;

    ///   \brief  Returns a span that can be used to iterate over the indices.
    std::span<const uint32_t> index_span() const noexcept;

    ///   \brief  Returns a pointer to the vertex data.
    const vertex_pnt* vertex_data() const noexcept;

    /// \brief  Returns a pointer to the indices data.
    const uint32_t* index_data() const noexcept;

    ///   \brief  Returns the number of bytes with vertex data.
    size_t vertex_bytes() const noexcept { return header().vertex_count * header().vertex_size; }

    ///   \brief  Returns the number of bytes with indices data.
    size_t index_bytes() const noexcept { return header().index_count * header().index_size; }

    ///   \brief  Reads the header information from a binary mesh file.
    static tl::optional<mesh_header> read_header(const char* file_path);

  private:
    mio::mmap_source _mfile;

    XRAY_NO_COPY(mesh_loader);
};

/// @}

} // namespace rendering
} // namespace xray
