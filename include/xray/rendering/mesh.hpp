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

/// \file   geometry_factory.hpp    Helper classes/functions to create
///         geometrical shapes.

#include "xray/xray.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace xray {
namespace rendering {

struct mesh_load_option {
  enum { remove_points_lines = 1u << 1, convert_left_handed = 1u << 2 };
};

class basic_mesh {
public:
  basic_mesh() noexcept = default;
  XRAY_DEFAULT_MOVE(basic_mesh);

  explicit basic_mesh(const char* path);

  basic_mesh(const xray::rendering::vertex_pnt* vertices,
             const size_t                       num_vertices,
             const uint32_t*                    indices,
             const size_t                       num_indices);

  bool valid() const noexcept { return _vertexbuffer && _indexbuffer; }

  explicit operator bool() const noexcept { return valid(); }

  GLuint vertex_buffer() const noexcept {
    assert(valid());
    return xray::base::raw_handle(_vertexbuffer);
  }

  GLuint index_buffer() const noexcept {
    assert(valid());
    return xray::base::raw_handle(_indexbuffer);
  }

  GLuint vertex_array() const noexcept {
    assert(valid());
    return xray::base::raw_handle(_vertexarray);
  }

  uint32_t index_count() const noexcept { return (uint32_t) _indices.size(); }

  const xray::rendering::vertex_pnt* vertices() const noexcept {
    assert(valid());
    return _vertices.data();
  }

  const xray::math::aabb3f& aabb() const noexcept {
    assert(valid());
    return _aabb;
  }

private:
  void compute_aabb();
  void setup_buffers();

  std::vector<xray::rendering::vertex_pnt> _vertices;
  std::vector<uint32_t>                    _indices;
  xray::rendering::scoped_buffer           _vertexbuffer;
  xray::rendering::scoped_buffer           _indexbuffer;
  xray::rendering::scoped_vertex_array     _vertexarray;
  xray::math::aabb3f _aabb{xray::math::aabb3f::stdc::identity};

private:
  XRAY_NO_COPY(basic_mesh);
};

/// @}

} // namespace rendering
} // namespace xray
