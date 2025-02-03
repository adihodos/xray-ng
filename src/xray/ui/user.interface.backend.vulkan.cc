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

#include "xray/ui/user.interface.backend.vulkan.hpp"

#include <cstdint>
#include <string_view>
#include <imgui/imgui.h>

#include "xray/base/scoped_guard.hpp"
#include "xray/ui/user.interface.backend.hpp"
#include "xray/ui/user_interface_render_context.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"

namespace xray::ui {

constexpr const uint32_t MAX_VERTICES = 8192;
constexpr const uint32_t MAX_INDICES = 65535;

constexpr const std::string_view UI_VERTEX_SHADER{ R"#(
#version 460 core

#include "core/bindless.core.glsl"

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in uint color;

layout (location = 0) out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 1) out VS_OUT_FS_IN {
    vec4 color;
    vec2 uv;
    flat uint textureid;
} vs_out;

void main() {
    const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
    const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
    gl_Position = vec4(pos * fgd.ui.scale + fgd.ui.translate, 0.0f, 1.0f);
    vs_out.color = unpackUnorm4x8(color);
    vs_out.uv = uv;
    vs_out.textureid = fgd.ui.textureid;
}

)#" };

constexpr const std::string_view UI_FRAGMENT_SHADER{ R"#(
#version 460 core

#include "core/bindless.core.glsl"

layout (location = 1) in VS_OUT_FS_IN {
    vec4 color;
    vec2 uv;
    flat uint textureid;
} fs_in;

layout (location = 0) out vec4 FinalFragColor;

void main() {
    FinalFragColor = texture(g_Textures2DGlobal[fs_in.textureid], fs_in.uv.st) * fs_in.color;
}
)#" };

UserInterfaceRenderBackend_Vulkan::UserInterfaceRenderBackend_Vulkan(
    PrivateConstructionToken,
    rendering::VulkanBuffer&& vertex_buffer,
    rendering::VulkanBuffer&& index_buffer,
    rendering::GraphicsPipeline&& pipeline,
    rendering::BindlessImageResourceHandleEntryPair font_atlas,
    VkSampler sampler)
    : _vertexbuffer(std::move(vertex_buffer))
    , _indexbuffer(std::move(index_buffer))
    , _pipeline(std::move(pipeline))
    , _font_atlas(font_atlas)
    , _sampler(sampler)
{
}

tl::expected<UserInterfaceRenderBackend_Vulkan, rendering::VulkanError>
UserInterfaceRenderBackend_Vulkan::create(rendering::VulkanRenderer& renderer,
                                          const UserInterfaceBackendCreateInfo& backend_create_info,
                                          base::MemoryArena* arena_perm,
                                          base::MemoryArena* arena_temp)
{
    using namespace rendering;

    const RenderBufferingSetup rbs{ renderer.buffering_setup() };

    auto vertex_buffer = VulkanBuffer::create(renderer,
                                              VulkanBufferCreateInfo{
                                                  .name_tag = "UI vertex buffer",
                                                  .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                  .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                  .bytes = MAX_VERTICES * sizeof(ImDrawVert),
                                                  .frames = rbs.buffers,
                                              });
    XR_VK_PROPAGATE_ERROR(vertex_buffer);

    auto index_buffer = VulkanBuffer::create(renderer,
                                             VulkanBufferCreateInfo{
                                                 .name_tag = "UI Index buffer",
                                                 .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                 .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                 .bytes = MAX_INDICES * sizeof(ImDrawIdx),
                                                 .frames = rbs.buffers,
                                             });
    XR_VK_PROPAGATE_ERROR(index_buffer);

    const VkPipelineColorBlendAttachmentState enable_blending{
        .blendEnable = true,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT,
    };

    auto graphics_pipeline =
        GraphicsPipelineBuilder{ arena_perm, arena_temp }
            .add_shader(ShaderStage::Vertex,
                        ShaderBuildOptions{
                            .code_or_file_path = UI_VERTEX_SHADER,
                        })
            .add_shader(ShaderStage::Fragment,
                        ShaderBuildOptions{
                            .code_or_file_path = UI_FRAGMENT_SHADER,
                        })
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .rasterization_state(RasterizationState{
                .poly_mode = VK_POLYGON_MODE_FILL,
                .cull_mode = VK_CULL_MODE_NONE,
                .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .line_width = 1.0f,
            })
            .depth_stencil_state(DepthStencilState{ .depth_test_enable = false, .depth_write_enable = false })
            .color_blend(enable_blending)
            .create_bindless(renderer);
    XR_VK_PROPAGATE_ERROR(graphics_pipeline);

    auto resources_job = renderer.create_job(QueueType::Transfer);
    XR_VK_PROPAGATE_ERROR(resources_job);

    auto font_atlas =
        VulkanImage::from_memory(renderer,
                                 VulkanImageCreateInfo{
                                     .tag_name = "UI font atlas",
                                     .wpkg = resources_job->buffer,
                                     .type = VK_IMAGE_TYPE_2D,
                                     .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                     .format = VK_FORMAT_R8G8B8A8_UNORM,
                                     .width = backend_create_info.atlas_width,
                                     .height = backend_create_info.atlas_height,
                                     .pixels = { backend_create_info.font_atlas_pixels },
                                 });
    XR_VK_PROPAGATE_ERROR(font_atlas);

    auto wait_token = renderer.submit_job(std::move(*resources_job));
    XRAY_SCOPE_EXIT_NOEXCEPT {
        renderer.consume_wait_token(std::move(*wait_token));
    };

    auto sampler = renderer.bindless_sys().get_sampler(
        VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipLodBias = 0.0f,
            .anisotropyEnable = false,
            .maxAnisotropy = 1.0f,
            .compareEnable = false,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = 1.0f,
            .borderColor = VkBorderColor{ VK_BORDER_COLOR_INT_OPAQUE_BLACK },
            .unnormalizedCoordinates = false,
        },
        renderer);
    XR_VK_PROPAGATE_ERROR(sampler);

    const BindlessImageResourceHandleEntryPair bindless_img =
        renderer.bindless_sys().add_image(std::move(*font_atlas), *sampler, tl::nullopt);

    if (backend_create_info.upload_callback) {
        (backend_create_info.upload_callback)(destructure_bindless_resource_handle(bindless_img.first).first,
                                              backend_create_info.upload_cb_context);
    }

    return tl::expected<UserInterfaceRenderBackend_Vulkan, rendering::VulkanError>{
        tl::in_place,
        PrivateConstructionToken{},
        std::move(*vertex_buffer),
        std::move(*index_buffer),
        std::move(*graphics_pipeline),
        bindless_img,
        *sampler,
    };
}

void
UserInterfaceRenderBackend_Vulkan::render(const UserInterfaceRenderContext& ctx,
                                          rendering::VulkanRenderer& vkr,
                                          const rendering::FrameRenderData& rd)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer
    // coordinates)
    int fb_width = (int)(ctx.draw_data->DisplaySize.x * ctx.draw_data->FramebufferScale.x);
    int fb_height = (int)(ctx.draw_data->DisplaySize.y * ctx.draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    //
    // TODO: add buffers resizing support
    assert(ctx.draw_data->TotalVtxCount <= MAX_VERTICES && "Vertex buffer too small");
    assert(ctx.draw_data->TotalIdxCount <= MAX_INDICES && "Index buffer too small");

    using namespace xray::rendering;
    const auto [frameid, buffer_count] = vkr.buffering_setup();

    if (ctx.draw_data->TotalVtxCount > 0) {
        const auto op_result = _vertexbuffer.mmap(vkr.device(), frameid).and_then([&](UniqueMemoryMapping vertexmap) {
            return _indexbuffer.mmap(vkr.device(), frameid).map([&](UniqueMemoryMapping indexmap) {
                ImDrawVert* vertex_buff = vertexmap.as<ImDrawVert>();
                ImDrawIdx* index_buff = indexmap.as<ImDrawIdx>();

                for (size_t i = 0; i < ctx.draw_data->CmdListsCount; ++i) {
                    const ImDrawList* draw_list = ctx.draw_data->CmdLists[i];
                    memcpy(vertex_buff, draw_list->VtxBuffer.Data, draw_list->VtxBuffer.size_in_bytes());
                    memcpy(index_buff, draw_list->IdxBuffer.Data, draw_list->IdxBuffer.size_in_bytes());

                    vertex_buff += draw_list->VtxBuffer.Size;
                    index_buff += draw_list->IdxBuffer.Size;
                }

                return std::true_type{};
            });
        });

        assert(op_result && "Failed to upload UI draw data to GPU!");

        vkCmdBindPipeline(rd.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.handle());

        const VkViewport viewport {
            .x = 0,
                .y = 0,
                .width = static_cast<float>(rd.fbsize.width),
                .height = static_cast<float>(rd.fbsize.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
        };

        vkCmdSetViewportWithCount(rd.cmd_buf, 1, &viewport);

        const VkBuffer vertex_buffers[] = { _vertexbuffer.buffer_handle() };
        const VkDeviceSize vertex_offsets[] = { 0 };
        vkCmdBindVertexBuffers(rd.cmd_buf, 0, 1, vertex_buffers, vertex_offsets);
        vkCmdBindIndexBuffer(rd.cmd_buf,
                             _indexbuffer.buffer_handle(),
                             0,
                             sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
        vkCmdPushConstants(rd.cmd_buf, _pipeline.layout(), VK_SHADER_STAGE_ALL, 0, 4, &rd.id);

        //
        // Will project scissor/clipping rectangles into framebuffer space
        const ImVec2 clip_off = ctx.draw_data->DisplayPos; // (0,0) unless using multi-viewports
        const ImVec2 clip_scale =
            ctx.draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        //
        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int global_vtx_offset = 0;
        int global_idx_offset = 0;
        for (int n = 0; n < ctx.draw_data->CmdListsCount; n++) {
            const ImDrawList* draw_list = ctx.draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++) {
                const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
                //
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                //
                // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
                if (clip_min.x < 0.0f) {
                    clip_min.x = 0.0f;
                }
                if (clip_min.y < 0.0f) {
                    clip_min.y = 0.0f;
                }
                if (clip_max.x > fb_width) {
                    clip_max.x = (float)fb_width;
                }
                if (clip_max.y > fb_height) {
                    clip_max.y = (float)fb_height;
                }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                //
                // Apply scissor/clipping rectangle
                VkRect2D scissor;
                scissor.offset.x = (int32_t)(clip_min.x);
                scissor.offset.y = (int32_t)(clip_min.y);
                scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
                scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
                vkCmdSetScissor(rd.cmd_buf, 0, 1, &scissor);

                //
                // Draw
                vkCmdDrawIndexed(rd.cmd_buf,
                                 pcmd->ElemCount,
                                 1,
                                 pcmd->IdxOffset + global_idx_offset,
                                 pcmd->VtxOffset + global_vtx_offset,
                                 0);
            }
            global_idx_offset += draw_list->IdxBuffer.Size;
            global_vtx_offset += draw_list->VtxBuffer.Size;
        }
    }
}

UIRenderUniform
UserInterfaceRenderBackend_Vulkan::uniform_data() const noexcept
{
    // 2.0f / draw_data->DisplaySize.x;
    // 2.0f / draw_data->DisplaySize.y;
    // float translate[2];
    // translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
    // translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
    return UIRenderUniform{};
}

}
