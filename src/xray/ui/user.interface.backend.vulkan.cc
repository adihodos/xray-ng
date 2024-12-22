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

#include "xray/ui/user.interface.backend.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"

namespace xray::ui {

constexpr const uint32_t MAX_VERTICES = 8192;
constexpr const uint32_t MAX_INDICES = 65535;

constexpr const std::string_view UI_VERTEX_SHADER{ R"#(
#version 460 core

)#" };

constexpr const std::string_view UI_FRAGMENT_SHADER{ R"#(
#version 460 core
)#" };

tl::expected<UserInterfaceRenderBackend_Vulkan, rendering::VulkanError>
UserInterfaceRenderBackend_Vulkan::create(rendering::VulkanRenderer& renderer,
                                          const UserInterfaceBackendCreateInfo& backend_create_info)
{
    using namespace rendering;

    const RenderBufferingSetup rbs{ renderer.buffering_setup() };
    auto work_pkg = renderer.create_work_package();
    XR_VK_PROPAGATE_ERROR(work_pkg);

    auto vertex_buffer =
        VulkanBuffer::create(renderer,
                             VulkanBufferCreateInfo{ .name_tag = "UI vertex buffer",
                                                     .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                     .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                     .bytes = MAX_VERTICES * sizeof(ImDrawVert),
                                                     .frames = rbs.buffers

                             });

    XR_VK_PROPAGATE_ERROR(vertex_buffer);

    auto index_buffer =
        VulkanBuffer::create(renderer,
                             VulkanBufferCreateInfo{ .name_tag = "UI Index buffer",
                                                     .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                     .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                     .bytes = MAX_INDICES * sizeof(ImDrawIdx),
                                                     .frames = rbs.buffers

                             });
    XR_VK_PROPAGATE_ERROR(index_buffer);

    auto graphics_pipeline = GraphicsPipelineBuilder{}
                                 .add_shader(ShaderStage::Vertex, UI_VERTEX_SHADER)
                                 .add_shader(ShaderStage::Fragment, UI_FRAGMENT_SHADER)
                                 .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
                                 .rasterization_state({ .poly_mode = VK_POLYGON_MODE_FILL,
                                                        .cull_mode = VK_CULL_MODE_BACK_BIT,
                                                        .front_face = VK_FRONT_FACE_CLOCKWISE,
                                                        .line_width = 1.0f })
                                 .create_bindless(renderer);
    XR_VK_PROPAGATE_ERROR(graphics_pipeline);

    auto font_atlas = VulkanImage::from_memory(
        renderer,
        VulkanImageCreateInfo{ .tag_name = "UI font atlas",
                               .wpkg = work_pkg->pkg,
                               .type = VK_IMAGE_TYPE_2D,
                               .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                               .format = VK_FORMAT_R8G8B8A8_UNORM,
                               .width = backend_create_info.atlas_width,
                               .height = backend_create_info.atlas_height,
                               .pixels = std::span{ &backend_create_info.font_atlas_pixels, 1 } });
    XR_VK_PROPAGATE_ERROR(font_atlas);

    renderer.submit_work_package(work_pkg->pkg);

    const BindlessImageResourceHandleEntryPair bindless_img = renderer.bindless_sys().add_image(std::move(*font_atlas), nullptr, nullptr);

    return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
}

void
UserInterfaceRenderBackend_Vulkan::render(const UserInterfaceRenderContext& ctx, rendering::VulkanRenderer& vkr)
{
}

}
