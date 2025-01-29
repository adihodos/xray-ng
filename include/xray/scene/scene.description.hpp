//
// Copyright (c) Adrian Hodos
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

#include <cstdint>
#include <string>
#include <filesystem>
#include <vector>
#include <rfl.hpp>

#include <tl/expected.hpp>

#include "xray/math/scalar4.hpp"
#include "xray/math/orientation.hpp"
#include "xray/scene/light.types.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"

namespace xray::scene {

struct GltfGeometryDescription
{
    std::string name;
    std::filesystem::path path;
};

struct ProceduralGeometryDescription
{
    std::string name;
    rfl::Variant<rfl::Field<"grid", rendering::GridParams>,
                 rfl::Field<"cone", rendering::ConeParams>,
                 rfl::Field<"torus", rendering::TorusParams>,
                 rfl::Field<"sphere", rendering::SphereParams>,
                 rfl::Field<"ring", rendering::RingGeometryParams>,
                 rfl::Field<"torus_knot", rendering::TorusKnotParams>>
        gen_params;
};

struct MaterialColoredDescription
{
    std::string name;
    math::vec4f ambient;
    math::vec4f diffuse;
    math::vec4f specular;
};

struct MaterialTexturedDescription
{
    std::string name;
    std::string ambient;
    std::string diffuse;
    std::string specular;
};

using MaterialDescription = rfl::Variant<rfl::Field<"colored", MaterialColoredDescription>,
                                         rfl::Field<"textured", MaterialTexturedDescription>>;

struct ProceduralEntityDescription
{
    std::string name;
    std::string material;
    std::string geometry;
    std::optional<math::OrientationF32> orientation;
};

struct GLTFEntityDescription
{
    std::string name;
    std::string gltf;
    std::optional<math::OrientationF32> orientation;
};

struct SceneError
{
    std::string err;
};

struct SceneDescription
{
    std::vector<MaterialDescription> materials;
    std::vector<GltfGeometryDescription> gltf_geometries;
    std::vector<ProceduralGeometryDescription> procedural_geometries;
    std::vector<scene::DirectionalLight> directional_lights;
    std::vector<scene::PointLight> point_lights;
    std::vector<scene::SpotLight> spot_lights;
    std::vector<ProceduralEntityDescription> procedural_entities;
    std::vector<GLTFEntityDescription> gltf_entities;

    static tl::expected<SceneDescription, SceneError> from_file(const std::filesystem::path& file_path);
};

void
write_test_scene_definition(const std::filesystem::path& scene_file);

}
