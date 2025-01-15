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
#include <vector>

#include <strong_type/strong_type.hpp>
#include <strong_type/bitarithmetic.hpp>
#include <strong_type/convertible_to.hpp>
#include <strong_type/equality.hpp>
#include <strong_type/formattable.hpp>
#include <strong_type/hashable.hpp>

#include "xray/math/scalar2.hpp"
#include "xray/math/orientation.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/scene/light.types.hpp"

namespace xray::rendering {
class VulkanRenderer;
}

namespace xray::scene {

using GeometryHandleType =
    strong::type<uint32_t, struct GeometryHandleTypeTag, strong::equality, strong::formattable, strong::hashable>;

struct ProceduralGeometryEntry
{
    std::string name;
    GeometryHandleType hashed_name;
    xray::math::vec2ui32 vertex_index_count;
    xray::math::vec2ui32 buffer_offsets;
};

struct ProceduralGeometry
{
    std::vector<ProceduralGeometryEntry> procedural_geometries;
    std::vector<VkDrawIndexedIndirectCommand> draw_indirect_template;
    xray::rendering::VulkanBuffer vertex_buffer;
    xray::rendering::VulkanBuffer index_buffer;
};

struct GltfGeometryEntry
{
    std::string name;
    GeometryHandleType hashed_name;
    xray::math::vec2ui32 vertex_index_count;
    xray::math::vec2ui32 buffer_offsets;
    xray::math::vec2ui32 materials_buffer; // x - offset, y - number of materials
};

struct GltfGeometry
{
    std::vector<GltfGeometryEntry> entries;
    std::vector<VkDrawIndexedIndirectCommand> indirect_draw_templates;
    xray::rendering::VulkanBuffer vertex_buffer;
    xray::rendering::VulkanBuffer index_buffer;
};

struct EntityDrawableComponent
{
    std::string name;
    uint32_t hashed_name;
    GeometryHandleType geometry_id;
    uint32_t material_id;
    xray::math::OrientationF32 orientation;
};

struct ColoredMaterial
{
    std::string name;
    uint32_t hashed_name;
    uint32_t texel;
};

struct TexturedMaterial
{
    std::string name;
    uint32_t hashed_name;
    uint32_t ambient;
    uint32_t diffuse;
    uint32_t specular;
};

struct NonGltfMaterialsData
{
    xray::rendering::VulkanImage null_tex;
    std::vector<ColoredMaterial> materials_colored;
    std::vector<TexturedMaterial> materials_textured;
    xray::rendering::VulkanImage color_texture;
    std::vector<xray::rendering::VulkanImage> textures;
    xray::rendering::VulkanBuffer sbo_materials_colored;
    xray::rendering::VulkanBuffer sbo_materials_textured;
    uint32_t image_slot_start{};
    uint32_t sbo_slot_start{};
};

struct GltfMaterialsData
{
    std::vector<xray::rendering::VulkanImage> images;
    xray::rendering::VulkanBuffer sbo_materials;
    uint32_t reserved_image_slot_start{};
};

struct GraphicsPipelineResources
{
    xray::rendering::GraphicsPipeline p_ads_color;
    xray::rendering::GraphicsPipeline p_pbr_color;
};

struct SceneDefinition
{
    GltfGeometry gltf;
    ProceduralGeometry procedural;
    xray::rendering::VulkanBuffer instances_buffer;
    std::vector<EntityDrawableComponent> entities;
    NonGltfMaterialsData materials_nongltf;
    GltfMaterialsData materials_gltf;
    GraphicsPipelineResources pipelines;
    std::vector<rendering::VulkanBuffer> sbos_lights;
    std::vector<DirectionalLight> directional_lights;
    std::vector<PointLight> point_lights;
    std::vector<SpotLight> spot_lights;
};

struct SceneResources
{
    xray::rendering::BindlessImageResourceHandleEntryPair null_tex;
    xray::rendering::BindlessImageResourceHandleEntryPair color_tex;
    std::vector<xray::rendering::BindlessImageResourceHandleEntryPair> materials_tex;
    std::vector<xray::rendering::BindlessImageResourceHandleEntryPair> materials_gltf;
    xray::rendering::BindlessStorageBufferResourceHandleEntryPair sbo_color_materials;
    xray::rendering::BindlessStorageBufferResourceHandleEntryPair sbo_texture_materials;
    xray::rendering::BindlessStorageBufferResourceHandleEntryPair sbo_pbr_materials;
    xray::rendering::BindlessStorageBufferResourceHandleEntryPair sbo_instances;
    xray::rendering::BindlessStorageBufferResourceHandleEntryPair sbo_directional_lights;
    xray::rendering::BindlessStorageBufferResourceHandleEntryPair sbo_point_lights;
    xray::rendering::BindlessStorageBufferResourceHandleEntryPair sbo_spot_lights;
    GraphicsPipelineResources pipelines;

    static SceneResources from_scene(SceneDefinition* sdef, xray::rendering::VulkanRenderer* r);
};

}
