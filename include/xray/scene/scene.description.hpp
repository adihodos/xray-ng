#pragma once

#include <cstdint>
#include <string>
#include <filesystem>
#include <vector>
#include <rfl.hpp>

#include "xray/math/scalar4.hpp"
#include "xray/math/orientation.hpp"
#include "xray/math/serialization/parser.scalar3.hpp"
#include "xray/math/serialization/parser.scalar4.hpp"
#include "xray/math/serialization/parser.quaternion.hpp"
#include "xray/scene/light.types.hpp"

namespace xray::scene {

struct GltfGeometryDescription
{
    std::string name;
    std::filesystem::path path;
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

struct TorusParams
{
    float outer;
    float inner;
    uint32_t rings;
    uint32_t sides;
};

struct ProceduralGeometryDescription
{
    std::string name;
    rfl::Variant<rfl::Field<"grid", GridParams>, rfl::Field<"cone", ConeParams>, rfl::Field<"torus", TorusParams>>
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
};

void
write_test_scene_definition(const std::filesystem::path& scene_file);

}
