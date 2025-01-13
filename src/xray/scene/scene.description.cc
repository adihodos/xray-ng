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

#include "xray/scene/scene.description.hpp"
#include <rfl.hpp>
#include "xray/math/serialization/parser.scalar3.hpp"
#include "xray/math/serialization/parser.scalar4.hpp"
#include "xray/math/serialization/parser.quaternion.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/base/serialization/rfl.libconfig/config.save.hpp"
#include "xray/base/serialization/rfl.libconfig/config.load.hpp"

tl::expected<xray::scene::SceneDescription, xray::scene::SceneError>
xray::scene::SceneDescription::from_file(const std::filesystem::path& file_path)
{
    const rfl::Result<SceneDescription> loaded_scene = rfl::libconfig::read<SceneDescription>(file_path);

    if (!loaded_scene) {
        return tl::make_unexpected(
            SceneError{ .err = fmt::format("Failed to parse scene: {}", loaded_scene.error()->what()) });
    }

    return loaded_scene.value();
}

void
xray::scene::write_test_scene_definition(const std::filesystem::path& scene_file)
{
    using namespace xray::math;
    using namespace xray::rendering;

    const SceneDescription test_scene_description {
        .materials = {
            rfl::make_field<"textured">(MaterialTexturedDescription {
                    .name = "MonkaTexture",
                    .ambient = "monka/ambient.ktx2",
                    .diffuse = "monka/diffuse.ktx2",
                    .specular = "monka/specular.ktx2",
                    }),
            rfl::make_field<"colored">(MaterialColoredDescription {
                    .name = "Color001",
                    .ambient = vec4f{color_palette::web::royal_blue},
                    .diffuse = vec4f{color_palette::web::royal_blue},
                    .specular = vec4f{color_palette::web::light_blue},
                    }),
        },

        .gltf_geometries = {
            GltfGeometryDescription {
                .name = "sa23",
                .path = "sa23/sa23.glb",
            }
        },

        .procedural_geometries = {
            ProceduralGeometryDescription {
                .name = "grid_basic",
                .gen_params = rfl::make_field<"grid">(GridParams{
                        .cellsx = 32,
                        .cellsy = 32,
                        .width = 0.5f,
                        .height = 0.5f
                        }),
            },
            ProceduralGeometryDescription {
                .name = "cone_section",
                .gen_params = rfl::make_field<"cone">(ConeParams{
                    .upper_radius = 2.0f,
                    .lower_radius = 4.0f,
                    .height = 5.0f,
                    }),
            },
        },

        .directional_lights = {
            scene::DirectionalLight {
                .direction = -vec3f::stdc::unit_y,
                .ambient = vec4f{color_palette::flat::clouds50.components},
                .diffuse = vec4f{color_palette::flat::concrete50.components},
                .specular = vec4f{color_palette::flat::concrete50.components},
            },
            scene::DirectionalLight {
                .direction = vec3f::stdc::unit_z,
                .ambient = vec4f{color_palette::flat::clouds50.components},
                .diffuse = vec4f{color_palette::flat::concrete50.components},
                .specular = vec4f{color_palette::flat::concrete50.components},
            },
        },

        .point_lights = {
            scene::PointLight {
                .position = vec3f{10.0f, 10.0f, 10.0f},
                .range = 25.0f,
                .attenuation = {1.0f, 1.0f, 1.0f},
                .ambient = vec4f{color_palette::web::ghost_white},
                .diffuse = vec4f{color_palette::web::ghost_white},
                .specular = vec4f{color_palette::web::ghost_white},
            },
            scene::PointLight {
                .position = vec3f{-10.0f, 10.0f, 10.0f},
                .range = 25.0f,
                .attenuation = {1.0f, 1.0f, 1.0f},
                .ambient = vec4f{color_palette::web::deep_sky_blue},
                .diffuse = vec4f{color_palette::web::deep_sky_blue},
                .specular = vec4f{color_palette::web::deep_sky_blue},
            },
        },

        .spot_lights = {
            scene::SpotLight {
                .position = vec3f{0.0f, 10.0f, 25.0f},
                .range = 100.0f,
                .attenuation = {1.0f, 1.0f, 1.0f},
                .ambient = vec4f{color_palette::material::orange500},
                .diffuse = vec4f{color_palette::material::orange500},
                .specular = vec4f{color_palette::material::orange500},
            },
        },

        .procedural_entities = {
            ProceduralEntityDescription {
                .name = "grid",
                .material = "MonkaTexture",
                .geometry = "grid_basic",
            },

            ProceduralEntityDescription {
                .name = "cone",
                .material = "Color001",
                .geometry = "cone_section",
            }
        },

        .gltf_entities = {
            GLTFEntityDescription {
                .name = "main_ship",
                .gltf = "sa23",
                .orientation = OrientationF32 {
                    .origin = vec3f::stdc::zero,
                    .rotation = quaternionf {1.0f, 0.0f, 0.0f, 0.0f},
                    .scale = 0.75f,
                }
            },
        },
    };

    rfl::libconfig::save(scene_file, test_scene_description);
}
