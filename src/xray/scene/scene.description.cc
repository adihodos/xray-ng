#include "xray/scene/scene.description.hpp"

#if 0
#include "xray/base/logger.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/serialization/serialization.hpp"
#include "xray/base/xray.slice.hpp"
#include "xray/base/xray.stringview.hpp"

#include "xray/rendering/colors/color_palettes.hpp"

void xray::scene::write_test_scene_definition(
	xray::base::MemoryArena& arena, const xray::base::xrStringView_t scene_file
) {
	using namespace xray::base;
	using namespace xray::math;
	using namespace xray::rendering;

	base::ScratchPadArena scratch_pad = base::ThreadLocalContext::acquire_scratchpad({&arena});

	const SceneDescription test_scene_description{
		.materials =
			{
				MaterialDescription::from_texture(
					MaterialTexturedDescription{
						.name	  = string_from_c_str(*scratch_pad.arena, "MonkaTexture"),
						.ambient  = string_from_c_str(*scratch_pad.arena, "monka/ambient.ktx2"),
						.diffuse  = string_from_c_str(*scratch_pad.arena, "monka/diffuse.ktx2"),
						.specular = string_from_c_str(*scratch_pad.arena, "monka/specular.ktx2"),
					}
				),
				MaterialDescription::from_color(
					MaterialColoredDescription{
						.name	  = string_from_c_str(*scratch_pad.arena, "Color001"),
						.ambient  = vec4f{color_palette::web::royal_blue},
						.diffuse  = vec4f{color_palette::web::royal_blue},
						.specular = vec4f{color_palette::web::light_blue},
					}
				),
			},

		.gltf_geometries = {GltfGeometryDescription{
			.name = string_from_c_str(arena, "sa23"),
			.path = string_from_c_str(arena, "sa23/sa23.glb"),
		}},

		.procedural_geometries =
			{
				ProceduralGeometryDescription{
					.name		= string_from_c_str(arena, "grid_basic"),
					.gen_params = xrGeometryParams_t::from_grid(
						GridParams{
							.cellsx = 32,
							.cellsy = 32,
							.width	= 1,
							.height = 1,
						}
					),
				},
				ProceduralGeometryDescription{
					.name		= string_from_c_str(arena, "cone_section"),
					.gen_params = xrGeometryParams_t::from_cone(
						ConeParams{
							.upper_radius = 2.0f,
							.lower_radius = 4.0f,
							.height		  = 5.0f,
						}
					),
				},
			},

		.directional_lights =
			{
				scene::DirectionalLight{
					.direction = -vec3f::stdc::unit_y,
					.ambient   = vec4f{color_palette::flat::clouds50.components},
					.diffuse   = vec4f{color_palette::flat::concrete50.components},
					.specular  = vec4f{color_palette::flat::concrete50.components},
				},
				scene::DirectionalLight{
					.direction = vec3f::stdc::unit_z,
					.ambient   = vec4f{color_palette::flat::clouds50.components},
					.diffuse   = vec4f{color_palette::flat::concrete50.components},
					.specular  = vec4f{color_palette::flat::concrete50.components},
				},
			},

		.point_lights =
			{
				scene::PointLight{
					.position	 = vec3f{10.0f, 10.0f, 10.0f},
					.range		 = 25.0f,
					.attenuation = {1.0f, 1.0f, 1.0f},
					.ambient	 = vec4f{color_palette::web::ghost_white},
					.diffuse	 = vec4f{color_palette::web::ghost_white},
					.specular	 = vec4f{color_palette::web::ghost_white},
				},
				scene::PointLight{
					.position	 = vec3f{-10.0f, 10.0f, 10.0f},
					.range		 = 25.0f,
					.attenuation = {1.0f, 1.0f, 1.0f},
					.ambient	 = vec4f{color_palette::web::deep_sky_blue},
					.diffuse	 = vec4f{color_palette::web::deep_sky_blue},
					.specular	 = vec4f{color_palette::web::deep_sky_blue},
				},
			},

		.spot_lights =
			{
				scene::SpotLight{
					.position	 = vec3f{0.0f, 10.0f, 25.0f},
					.range		 = 100.0f,
					.attenuation = {1.0f, 1.0f, 1.0f},
					.ambient	 = vec4f{color_palette::material::orange500},
					.diffuse	 = vec4f{color_palette::material::orange500},
					.specular	 = vec4f{color_palette::material::orange500},
				},
			},

		.procedural_entities =
			{ProceduralEntityDescription{
				 .name	   = string_from_c_str(arena, "grid"),
				 .material = string_from_c_str(arena, "MonkaTexture"),
				 .geometry = string_from_c_str(arena, "grid_basic"),
			 },

			 ProceduralEntityDescription{
				 .name	   = string_from_c_str(arena, "cone"),
				 .material = string_from_c_str(arena, "Color001"),
				 .geometry = string_from_c_str(arena, "cone_section"),
			 }},

		.gltf_entities = {
			GLTFEntityDescription{
				.name		 = string_from_c_str(arena, "main_ship"),
				.gltf		 = string_from_c_str(arena, "sa23"),
				.orientation = OrientationF32{
					.origin	  = vec3f::stdc::zero,
					.rotation = quaternionf{1.0f, 0.0f, 0.0f, 0.0f},
					.scale	  = 0.75f,
				}
			},
		},
	};

	// xrGeometryParams_t params = xrGeometryParams_t::from_box(
	// 	rendering::BoxParams{
	// 		.width	= 100,
	// 		.height = 100,
	// 		.depth	= 100,
	// 	}
	// );
	//
	// MaterialDescription mtl_desc = MaterialDescription::from_color(
	// 	MaterialColoredDescription{
	// 		.name	  = base::string_from_c_str(arena, "just a field"),
	// 		.ambient  = math::vec4f32{1.0f, 0.0f, 0.0f},
	// 		.diffuse  = math::vec4f32{0.0f, 1.0f, 0.0f},
	// 		.specular = math::vec4f32{0.0f, 0.0f, 1.0f},
	// 	}
	// );

	base::serialize_to_file(test_scene_description, scene_file);

	SceneDescription mtl_read{};
	const bool result = base::deserialize_from_file(arena, mtl_read, scene_file);
	XRAY_ASSERT_NOMSG(result == true);

	base::xrSlice_t<U8> mem_scratch{
		.s_ptr = static_cast<U8*>(scratch_pad.arena->alloc_align<U8>(4096)),
		.s_len = 4096,
	};

	const bool s_res = base::serialize_to_memory(mtl_read, mem_scratch);
	XRAY_ASSERT_NOMSG(s_res);
	XR_LOG_INFO("Deserialized %s", mem_scratch.cdata());
}

// 
// template xray::base::xrSerializeResult_t xray::base::serialize_to_file(
// 	const xray::scene2::SceneDescription&, const C8*
// );
// 
// template bool xray::base::serialize_to_memory(const xray::scene2::SceneDescription&, xray::base::xrSlice_t<U8>);
// 

#endif
