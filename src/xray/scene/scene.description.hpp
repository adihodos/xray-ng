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

#include "xray/xray.hpp"

#include <vector>

#include "xray/base/xray.string.hpp"
#include "xray/base/xray.stringview.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/orientation.hpp"
#include "xray/scene/light.types.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/geometry/procedural.terrain.hpp"

namespace xray::base {
struct MemoryArena;
}

namespace xray::scene {

struct GltfGeometryDescription {
	base::xrString_t name;
	base::xrString_t path;
};

struct xrGeometryParams_t {
	//
	// Mom, can we has reflection at home ? The reflection at home:
#define XR_GEOMETRY_TYPE_LIST                                \
	XR_GEOMETRY_TYPE_LIST_ENTRY(GridParams, grid)            \
	XR_GEOMETRY_TYPE_LIST_ENTRY(ConeParams, cone)            \
	XR_GEOMETRY_TYPE_LIST_ENTRY(TorusParams, torus)          \
	XR_GEOMETRY_TYPE_LIST_ENTRY(SphereParams, sphere)        \
	XR_GEOMETRY_TYPE_LIST_ENTRY(RingGeometryParams, ring)    \
	XR_GEOMETRY_TYPE_LIST_ENTRY(TorusKnotParams, torus_knot) \
	XR_GEOMETRY_TYPE_LIST_ENTRY(BoxParams, box)

	enum class xrTagType_t : U8 {
#define XR_GEOMETRY_TYPE_LIST_ENTRY(e, m) e,
		XR_GEOMETRY_TYPE_LIST
#undef XR_GEOMETRY_TYPE_LIST_ENTRY
	};

	xrTagType_t tag;

	union {
#define XR_GEOMETRY_TYPE_LIST_ENTRY(e, m) [[= xrTagType_t::e]] rendering::e m;
		XR_GEOMETRY_TYPE_LIST
#undef XR_GEOMETRY_TYPE_LIST_ENTRY
	};

#define XR_GEOMETRY_TYPE_LIST_ENTRY(e, m)                        \
	static xrGeometryParams_t from_##m(const rendering::e val) { \
		return xrGeometryParams_t{                               \
			.tag = xrTagType_t::e,                               \
			.m	 = val,                                          \
		};                                                       \
	}

	XR_GEOMETRY_TYPE_LIST

#undef XR_GEOMETRY_TYPE_LIST_ENTRY
#undef XR_GEOMETRY_TYPE_LIST
};

struct ProceduralGeometryDescription {
	base::xrString_t name;
	xrGeometryParams_t gen_params;
};

struct MaterialColoredDescription {
	base::xrString_t name;
	math::vec4f ambient;
	math::vec4f diffuse;
	math::vec4f specular;
};

struct MaterialTexturedDescription {
	base::xrString_t name;
	base::xrString_t ambient;
	base::xrString_t diffuse;
	base::xrString_t specular;
};

enum class MaterialType : U8 {
	Color,
	Texture,
};

struct MaterialDescription {
#define XR_MATERIAL_TYPE_LIST                                      \
	XR_MATERIAL_TYPE_LIST_ENTRY(MaterialColoredDescription, color) \
	XR_MATERIAL_TYPE_LIST_ENTRY(MaterialTexturedDescription, texture)

	enum class xrTagType_t : U8 {
#define XR_MATERIAL_TYPE_LIST_ENTRY(e, m) e,
		XR_MATERIAL_TYPE_LIST
#undef XR_MATERIAL_TYPE_LIST_ENTRY
	};

	xrTagType_t tag;
	union {
#define XR_MATERIAL_TYPE_LIST_ENTRY(e, m) [[= xrTagType_t::e]] e m;
		XR_MATERIAL_TYPE_LIST
#undef XR_MATERIAL_TYPE_LIST_ENTRY
	};

	MaterialDescription() noexcept {}

#define XR_MATERIAL_TYPE_LIST_ENTRY(e, m)               \
	static MaterialDescription from_##m(const e& val) { \
		MaterialDescription m;                          \
		m.tag = xrTagType_t::e;                         \
		m.m	  = val;                                    \
		return m;                                       \
	}
	XR_MATERIAL_TYPE_LIST
#undef XR_MATERIAL_TYPE_LIST_ENTRY
#undef XR_MATERIAL_TYPE_LIST
};

struct ProceduralEntityDescription {
	base::xrString_t name;
	base::xrString_t material;
	base::xrString_t geometry;
	math::OrientationF32 orientation;
};

struct GLTFEntityDescription {
	base::xrString_t name;
	base::xrString_t gltf;
	math::OrientationF32 orientation;
};

struct SceneError {
	base::xrString_t err;
};

struct SceneDescription {
	std::vector<MaterialDescription> materials;
	std::vector<GltfGeometryDescription> gltf_geometries;
	std::vector<ProceduralGeometryDescription> procedural_geometries;
	std::vector<scene::DirectionalLight> directional_lights;
	std::vector<scene::PointLight> point_lights;
	std::vector<scene::SpotLight> spot_lights;
	std::vector<ProceduralEntityDescription> procedural_entities;
	std::vector<GLTFEntityDescription> gltf_entities;
	std::vector<rendering::TerrainRange> terrain_ranges;
	xray::rendering::TerrainParams terrain_params;

	// SceneDescription* from_file(base::MemoryArena& arena, const std::filesystem::path& file_path);
};

void write_test_scene_definition(xray::base::MemoryArena& arena, const xray::base::xrStringView_t path);

}  // namespace xray::scene
