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
#include "xray/math/objects/sphere.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/vertex_format/vertex_format.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>

namespace xray {
namespace rendering {

struct geometry_data_t;

struct mesh_load_option {
  enum { remove_points_lines = 1u << 1, convert_left_handed = 1u << 2 };
};

struct vertex_format_info {
  uint32_t                        components;
  size_t                          element_size;
  const vertex_format_entry_desc* description;
};

enum class primitive_topology {
  undefined,
  point,
  line,
  line_strip,
  triangle,
  triangle_strip
};

class simple_mesh {
public:
  simple_mesh() noexcept = default;

  simple_mesh(const vertex_format fmt, const geometry_data_t& geometry,
              const primitive_topology topology = primitive_topology::triangle);

  simple_mesh(
      const vertex_format fmt, const char* mesh_file,
      const uint32_t load_options = mesh_load_option::remove_points_lines);

  simple_mesh(
      const vertex_format fmt, const std::string& mesh_file,
      const uint32_t load_options = mesh_load_option::remove_points_lines)
      : simple_mesh{fmt, mesh_file.c_str(), load_options} {}

  XRAY_DEFAULT_MOVE(simple_mesh);

public:
  explicit operator bool() const noexcept { return valid(); }

  bool valid() const noexcept { return _vertices != nullptr; }

  const xray::math::aabb3f& aabb() const noexcept {
    assert(valid());
    return _boundingbox;
  }

  const vertex_format_info* vertex_fmt_description() const noexcept {
    return &_vertex_format_info;
  }

  vertex_format vertex_fmt() const noexcept { return _vertexformat; }
  uint32_t      vertex_count() const noexcept { return _vertexcount; }

  index_format index_fmt() const noexcept { return _indexformat; }
  uint32_t     index_count() const noexcept { return _indexcount; }

  void* vertices() const noexcept { return base::raw_ptr(_vertices); }
  void* indices() const noexcept { return base::raw_ptr(_indices); }

  bool indexed() const noexcept { return _indexcount != 0; }
  void draw() const noexcept;

  size_t id() const noexcept { return _id; }

  size_t byte_size_vertices() const noexcept {
    return _vertexcount * _vertex_format_info.element_size;
  }

  size_t byte_size_indices() const noexcept {
    return _indexcount * (_indexformat == index_format::u32 ? 4 : 2);
  }

  primitive_topology topology() const noexcept { return _topology; }

private:
  bool load_model_impl(const char* model_data, const size_t data_size,
                       const uint32_t mesh_process_opts,
                       const uint32_t mesh_import_opts);

  struct create_buffers_args {
    const void*               vertexbuffer_data;
    size_t                    vertexbuffer_bytes;
    size_t                    vertexcount;
    const void*               indexbuffer_data;
    size_t                    indexbuffer_size;
    const vertex_format_info* fmt_into;
  };

  void create_buffers(const create_buffers_args* args);

  struct malloc_deleter {
    void operator()(void* ptr) const noexcept { free(ptr); }
  };

private:
  xray::base::unique_pointer<void, malloc_deleter> _vertices;
  xray::base::unique_pointer<void, malloc_deleter> _indices;
  uint32_t                             _vertexcount{};
  uint32_t                             _indexcount{};
  xray::rendering::scoped_buffer       _vertexbuffer;
  xray::rendering::scoped_buffer       _indexbuffer;
  xray::rendering::scoped_vertex_array _vertexarray;
  vertex_format_info                   _vertex_format_info;
  xray::math::aabb3f                   _boundingbox;
  size_t                               _id{};
  vertex_format                        _vertexformat{vertex_format::undefined};
  index_format                         _indexformat{index_format::u32};
  primitive_topology                   _topology{primitive_topology::undefined};
  bool                                 _valid{false};

private:
  XRAY_NO_COPY(simple_mesh);
};

struct mesh_graphics_rep {
public:
  using vertex_buffer_handle = scoped_buffer::handle_type;
  using index_buffer_handle  = scoped_buffer::handle_type;
  using vertex_array_handle  = scoped_vertex_array::handle_type;

public:
  mesh_graphics_rep() noexcept = default;
  XRAY_DEFAULT_MOVE(mesh_graphics_rep);

  explicit mesh_graphics_rep(const simple_mesh& mesh_geometry);

  bool valid() const noexcept {
    return _vertexbuffer && _indexbuffer && _vertexarray;
  }

  explicit operator bool() const noexcept { return valid(); }

  vertex_buffer_handle vertex_buffer() const noexcept {
    assert(valid());
    return base::raw_handle(_vertexbuffer);
  }

  index_buffer_handle index_buffer() const noexcept {
    assert(valid());
    return base::raw_handle(_indexbuffer);
  }

  vertex_array_handle vertex_array() const noexcept {
    assert(valid());
    return base::raw_handle(_vertexarray);
  }

  const simple_mesh* geometry() const noexcept { return _geometry; }

  void draw() noexcept;

private:
  const simple_mesh*                   _geometry{nullptr};
  xray::rendering::scoped_buffer       _vertexbuffer;
  xray::rendering::scoped_buffer       _indexbuffer;
  xray::rendering::scoped_vertex_array _vertexarray;

private:
  XRAY_NO_COPY(mesh_graphics_rep);
};

/// @}

} // namespace rendering
} // namespace xray
