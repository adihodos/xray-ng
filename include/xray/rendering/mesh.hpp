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
#include <vector>

#include <platformstl/filesystem/memory_mapped_file.hpp>

#include "xray/base/maybe.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/mesh_loader.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"

namespace xray {
namespace rendering {

enum class mesh_type { readonly, writable };

struct instance_descriptor {
  using buffer_handle_type = GLuint;

  buffer_handle_type buffer_handle{};
  uint32_t           element_stride{};
  uint32_t           instance_divisor{};
  uint32_t           buffer_offset{0};
};

struct vertex_attribute_descriptor {
  int32_t                    component_count{};
  int32_t                    component_type{};
  ptrdiff_t                  component_offset{};
  int32_t                    component_index{};
  bool                       normalized{false};
  const instance_descriptor* instance_desc{};
};

// TODO: this class has a terrible desing and needs to be revisited
class basic_mesh {
public:
  using vertex_buffer_handle       = GLuint;
  using index_buffer_handle        = GLuint;
  using vertex_array_object_handle = GLuint;

  basic_mesh() noexcept = default;
  XRAY_DEFAULT_MOVE(basic_mesh);

  basic_mesh(const char* path, const mesh_type type = mesh_type::readonly);

  basic_mesh(const xray::rendering::vertex_pnt* vertices,
             const size_t                       num_vertices,
             const uint32_t*                    indices,
             const size_t                       num_indices,
             const mesh_type                    mtype = mesh_type::readonly);

  basic_mesh(std::span<const vertex_pnt> vertices,
             std::span<const uint32_t>   indices) {
    create(vertices, indices);
	compute_bounding();
  }

  basic_mesh(const mesh_loader& ml) {
    const mesh_loader* mloader = &ml; // mlady monkaS
    create(mloader->vertex_span(), mloader->index_span());
    _aabb    = mloader->bounding().axis_aligned_bbox;
    _bsphere = mloader->bounding().bounding_sphere;
  }

  bool valid() const noexcept { return _vertexbuffer && _indexbuffer; }

  explicit operator bool() const noexcept { return valid(); }

  vertex_buffer_handle vertex_buffer() const noexcept {
    assert(valid());
    return xray::base::raw_handle(_vertexbuffer);
  }

  index_buffer_handle index_buffer() const noexcept {
    assert(valid());
    return xray::base::raw_handle(_indexbuffer);
  }

  vertex_array_object_handle vertex_array() const noexcept {
    assert(valid());
    return xray::base::raw_handle(_vertexarray);
  }

  uint32_t index_count() const noexcept { return (uint32_t) _indices.size(); }
  uint32_t vertex_count() const noexcept { return (uint32_t) _vertices.size(); }

  std::span<vertex_pnt> vertices() noexcept {
    assert(valid());
    return {_vertices};
  }

  std::span<const vertex_pnt> vertices() const noexcept {
    assert(valid());
    return {_vertices};
  }

  std::span<uint32_t> indices() noexcept {
    assert(valid());
    return {_indices};
  }

  std::span<const uint32_t> indices() const noexcept {
    assert(valid());
    return {_indices};
  }

  const xray::math::aabb3f& aabb() const noexcept {
    assert(valid());
    return _aabb;
  }

  const xray::math::sphere3f& bounding_sphere() const noexcept {
    assert(valid());
    return _bsphere;
  }

  void update_vertices() noexcept;
  void update_indices() noexcept;

  void set_instance_data(const instance_descriptor*         instances,
                         const size_t                       instance_count,
                         const vertex_attribute_descriptor* vertex_attributes,
                         const size_t vertex_attributes_count);

private:
  void create(const std::span<const vertex_pnt> vertices,
              const std::span<const uint32_t>   indices);

  void setup_buffers();

  void compute_bounding();

  mesh_type                                _mtype;
  std::vector<xray::rendering::vertex_pnt> _vertices;
  std::vector<uint32_t>                    _indices;
  xray::rendering::scoped_buffer           _vertexbuffer;
  xray::rendering::scoped_buffer           _indexbuffer;
  xray::rendering::scoped_vertex_array     _vertexarray;
  xray::math::aabb3f   _aabb{xray::math::aabb3f::stdc::identity};
  xray::math::sphere3f _bsphere{};

private:
  XRAY_NO_COPY(basic_mesh);
};

} // namespace rendering
} // namespace xray
