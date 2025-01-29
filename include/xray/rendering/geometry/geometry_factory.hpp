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

#include <cassert>
#include <cmath>
#include <cstdint>
#include <span>

#include "xray/math/constants.hpp"

namespace xray {
namespace rendering {

struct geometry_data_t;

struct vertex_ripple_parameters
{
    float amplitude;
    float period;
    float width;
    float height;
};

struct vertex_effect
{
    template<typename vertex_type>
    static void ripple(std::span<vertex_type> vertices,
                       std::span<const uint32_t> indices,
                       const vertex_ripple_parameters& rp) noexcept;
};

template<typename vertex_type>
void
xray::rendering::vertex_effect::ripple(std::span<vertex_type> vertices,
                                       std::span<const uint32_t> indices,
                                       const vertex_ripple_parameters& rp) noexcept
{

    const auto scale_x = rp.width * 0.5f;
    const auto scale_z = rp.height * 0.5f;

    for (auto& vertex : vertices) {
        vertex.normal = decltype(vertex.normal){ 0 };
        const auto x_pos = vertex.position.x / scale_x;
        const auto z_pos = vertex.position.z / scale_z;

        vertex.position.y = rp.amplitude * std::sin(rp.period * (x_pos * x_pos + z_pos * z_pos));
    }

    assert((indices.size() % 3) == 0);

    for (uint32_t i = 0, idx_count = (uint32_t)indices.size(); i < idx_count; i += 3) {
        auto& v0 = vertices[indices[i + 0]];
        auto& v1 = vertices[indices[i + 1]];
        auto& v2 = vertices[indices[i + 2]];

        const auto normal = cross(v2.position - v0.position, v1.position - v0.position);

        v0.normal += normal;
        v1.normal += normal;
        v2.normal += normal;
    }

    for (auto& vertex : vertices) {
        vertex.normal = normalize(vertex.normal);
    }
}

struct RingGeometryParams
{
    float inner_radius{ 0.5f };
    float outer_radius{ 1.0f };
    uint32_t theta_segments{ 32 };
    uint32_t phi_segments{ 1 };
    float theta_start{ 0.0f };
    float theta_lengt{ math::F32::TwoPi };
};

struct TorusParams
{
    float outer_radius;
    float tube_radius;
    uint32_t radial_segments;
    uint32_t tubular_segments;
    float arc;
};

struct SphereParams
{
    float radius;
    uint32_t subdivisions;
};

struct GridParams
{
    uint32_t cellsx;
    uint32_t cellsy;
    float width;
    float height;
};

struct ConeParams
{
    float upper_radius;
    float lower_radius;
    float height;
    uint32_t tess_vert;
    uint32_t tess_horz;
};

struct BoxParams
{
    float width;
    float height;
    float depth;
};

struct TorusKnotParams
{
    float radius;
    float tube;
    uint32_t tubular_segments;
    uint32_t radial_segments;
    uint32_t p;
    uint32_t q;
};

class geometry_factory
{
  public:
    static geometry_data_t tetrahedron();
    static geometry_data_t hexahedron();
    static geometry_data_t octahedron();
    static geometry_data_t dodecahedron();
    static geometry_data_t icosahedron();

    static geometry_data_t cylinder(const uint32_t ring_tesselation_factor,
                                    const uint32_t rings_count,
                                    const float height,
                                    const float radius);

    /// Creates a box centered around the origin, with the specified
    /// dimensions.
    /// \param  width   Dimension along the X axis.
    /// \param  height  Dimension along the Y axis.
    /// \param  depth   Dimension along the Z axis.
    static geometry_data_t box(const BoxParams boxparams);

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
    static geometry_data_t grid(const GridParams& params);

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
    static geometry_data_t conical_section(const ConeParams& coneparams);

    static geometry_data_t geosphere(const SphereParams);
    static geometry_data_t torus(const TorusParams& params);
    static geometry_data_t torus_knot(const TorusKnotParams& params);

    /// Creates a quad spanning the whole screen. Vertices will be in NDC
    /// space.
    static void fullscreen_quad(geometry_data_t* grid_geometry);

    /// Creates a planar ring (XY plane)
    static geometry_data_t ring_geometry(const RingGeometryParams& params);
};

/// @}

} // namespace rendering
} // namespace xray
