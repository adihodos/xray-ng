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
#include "xray/math/objects/aabb3.hpp"
#include "xray/math/objects/sphere.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/vertex_format/vertex_format.hpp"
#include <cassert>
#include <cstdint>
#include <string>

namespace xray {
namespace rendering {

struct geometry_data_t;

struct mesh_load_option {
  enum { remove_points_lines = 1u << 1, convert_left_handed = 1u << 2 };
};

class simple_mesh {
public:
  simple_mesh() noexcept = default;

  simple_mesh(const vertex_format fmt, const geometry_data_t& geometry);

  simple_mesh(
      const vertex_format fmt, const char* mesh_file,
      const uint32_t load_options = mesh_load_option::remove_points_lines);

  simple_mesh(
      const vertex_format fmt, const std::string& mesh_file,
      const uint32_t load_options = mesh_load_option::remove_points_lines)
      : simple_mesh{fmt, mesh_file.c_str(), load_options} {}

  XRAY_DEFAULT_MOVE(simple_mesh);

  explicit operator bool() const noexcept { return valid(); }
  bool valid() const noexcept { return _valid; }

  xray::math::aabb3f aabb() const noexcept {
    assert(valid());
    return _boundingbox;
  }

  void draw();

private:
  bool load_model_impl(const char* model_data, const size_t data_size,
                       const uint32_t mesh_process_opts,
                       const uint32_t mesh_import_opts);

  void create_vertexarray();

private:
  xray::rendering::scoped_buffer       _vertexbuffer;
  xray::rendering::scoped_buffer       _indexbuffer;
  xray::rendering::scoped_vertex_array _vertexarray;
  vertex_format                        _vertexformat{vertex_format::undefined};
  index_format                         _indexformat{index_format::u16};
  uint32_t                             _indexcount{};
  xray::math::aabb3f                   _boundingbox;
  bool                                 _valid{false};

private:
  XRAY_NO_COPY(simple_mesh);
};

/// @}

} // namespace rendering
} // namespace xray
