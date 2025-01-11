#include "xray/scene/scene.description.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/base/serialization/rfl.libconfig/config.save.hpp"

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
