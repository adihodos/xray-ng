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

#include "xray/scene/scene.definition.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

xray::scene::SceneResources
xray::scene::SceneResources::from_scene(SceneDefinition* sdef, xray::rendering::VulkanRenderer* r)
{
    using namespace std;
    using namespace xray::rendering;

    BindlessSystem* bsys = &r->bindless_sys();

    auto def_sampler = bsys->get_sampler(
        VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .mipLodBias = 0.0f,
            .anisotropyEnable = false,
            .maxAnisotropy = 1.0f,
            .compareEnable = false,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = false,
        },
        *r);

    //
    // color texture added 1st so need to add 1 to slot
    vector<BindlessImageResourceHandleEntryPair> materials_tex{};
    for (uint32_t idx = 0, count = static_cast<uint32_t>(sdef->materials_nongltf.textures.size()); idx < count; ++idx) {
        materials_tex.push_back(bsys->add_image(std::move(sdef->materials_nongltf.textures[idx]),
                                                *def_sampler,
                                                sdef->materials_nongltf.image_slot_start + 1 + idx));
    }

    SceneResources scene_resources{
        .color_tex = bsys->add_image(
            std::move(sdef->materials_nongltf.color_texture), *def_sampler, sdef->materials_nongltf.image_slot_start),
        .materials_tex = std::move(materials_tex),
        .sbo_color_materials = bsys->add_storage_buffer(std::move(sdef->materials_nongltf.sbo_materials_colored),
                                                        sdef->materials_nongltf.sbo_slot_start + 0),
        .sbo_texture_materials = bsys->add_storage_buffer(std::move(sdef->materials_nongltf.sbo_materials_textured),
                                                          sdef->materials_nongltf.sbo_slot_start + 1),
        .sbo_instances = bsys->add_chunked_storage_buffer(
            std::move(sdef->instances_buffer), r->buffering_setup().buffers, tl::nullopt),
    };

    //
    // transfer ownership to graphics queue
    r->queue_image_ownership_transfer(scene_resources.color_tex.first);
    for (const BindlessImageResourceHandleEntryPair& e : scene_resources.materials_tex) {
        r->queue_image_ownership_transfer(e.first);
    }

    return scene_resources;
}
