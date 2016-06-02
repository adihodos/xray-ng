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
#include <cstdint>
#include <string>

namespace xray {
namespace rendering {

struct geometry_data_t;

/// \brief      Import options for meshes. Can be combined via operator |.
enum class mesh_import_options : uint16_t {
  none = 0U,

  ///< Convert indices and normals to use a left handed coordinate system
  convert_left_handed = 1U << 0,

  ///< Remove any points and lines from the mesh (leaving only triangles/quads)
  remove_points_lines = 1U << 1
};

inline constexpr mesh_import_options
operator|(const mesh_import_options opt_a,
          const mesh_import_options opt_b) noexcept {

  return static_cast<mesh_import_options>(static_cast<uint16_t>(opt_a) |
                                          static_cast<uint16_t>(opt_b));
}

inline constexpr bool operator&(const mesh_import_options opt_a,
                                const mesh_import_options opt_b) noexcept {
  return (static_cast<uint16_t>(opt_a) & static_cast<uint16_t>(opt_b)) != 0;
}

class geometry_factory {
public:
  static void tetrahedron(geometry_data_t* mesh);

  static void hexahedron(geometry_data_t* mesh);

  static void octahedron(geometry_data_t* mesh);

  static void dodecahedron(geometry_data_t* mesh);

  static void icosahedron(geometry_data_t* mesh);

  /// Creates a circle (XZ plane), centered around the origin.
  /// \param  radius  Circle's radius.
  /// \param  num_slices  Number of vertices used approximate the circle.
  ///         A higher number leads to a smoother circle.
  // static void create_circle(const float radius, const uint32_t num_slices,
  //                           geometry_data_t* circle_geometry);

  static void cylinder(const uint32_t ring_tesselation_factor,
                       const uint32_t rings_count, const float height,
                       const float radius, geometry_data_t* cylinder);

  /// Creates a box centered around the origin, with the specified
  /// dimensions.
  /// \param  width   Dimension along the X axis.
  /// \param  height  Dimension along the Y axis.
  /// \param  depth   Dimension along the Z axis.
  static void box(const float width, const float height, const float depth,
                  geometry_data_t* box_geometry);

  ///
  /// Creates a cone, centered at the origin.
  /// \param  height  Cone height, must be greater than 0.
  /// \param  slices  Tesselation factor. Passing 3 will result in a
  /// tetrahedron, 4 will result in a pyramid and so on. The mininum
  /// number is 3.
  // static void create_conical_shape(const float height, const float radius,
  //                                  const uint32_t slices, const uint32_t
  //                                  stacks,
  //                                  geometry_data_t* mesh);

  /// Creates a grid in the XZ plane, centered around the origin.
  /// \param  grid_width  Length along the X axis.
  /// \param  grid_depth  Length along the Z axis.
  static void grid(const float grid_width, const float grid_depth,
                   const size_t row_count, const size_t column_count,
                   geometry_data_t* grid_geometry);

  /// Creates a sphere, centered at the origin.
  /// \param  tess_factor_horz    Horizontal tesselation factor. Must be
  ///         at least 3. A higher number results in a smoother sphere.
  /// \param  tess_factor_vert    Vertical tesselation factor. Must be at
  ///         least 1. A higher number results in a smoother sphere.
  // static void create_spherical_shape(const float      radius,
  //                                    const uint32_t   tess_factor_horz,
  //                                    const uint32_t   tess_factor_vert,
  //                                    geometry_data_t* mesh_data);

  /// Creates a conical section, centered around the origin.
  /// \param  tess_factor_horz    Horizontal tesselation factor. Must be
  ///         at least 3.
  /// \param  tess_factor_vert    Vertical tesselation factor. Valid values
  ///         are from 0.
  // static void create_conical_section(const float      upper_radius,
  //                                    const float      lower_radius,
  //                                    const float      height,
  //                                    const uint32_t   tess_factor_horz,
  //                                    const uint32_t   tess_factor_vert,
  //                                    geometry_data_t* mesh);

  static void geosphere(const float radius, const uint32_t max_subdivisions,
                        geometry_data_t* mesh);

  static void torus(const float outer_radius, const float inner_radius,
                    const uint32_t sides, const uint32_t rings,
                    geometry_data_t* mesh);
  /// Creates a quad spanning the whole screen. Vertices will be in NDC
  /// space.
  static void fullscreen_quad(geometry_data_t* grid_geometry);

  static bool
  load_model(geometry_data_t* mesh, const char* file_path,
             const mesh_import_options import_opts = mesh_import_options::none);

  static bool load_model(
      geometry_data_t* mesh, const std::string& mode_path,
      const mesh_import_options import_opts = mesh_import_options::none) {
    return load_model(mesh, mode_path.c_str(), import_opts);
  }
};

/// @}

} // namespace rendering
} // namespace xray
