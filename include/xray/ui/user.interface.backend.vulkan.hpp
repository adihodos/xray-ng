#pragma once

#include "xray/xray.hpp"

#include <tl/expected.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"

namespace xray::rendering {
class VulkanRenderer;
struct FrameRenderData;
}

namespace xray::ui {

struct UIRenderUniform;
struct UserInterfaceBackendCreateInfo;
struct UserInterfaceRenderContext;

class UserInterfaceRenderBackend_Vulkan
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    UserInterfaceRenderBackend_Vulkan(PrivateConstructionToken,
                                      rendering::VulkanBuffer&& vertex_buffer,
                                      rendering::VulkanBuffer&& index_buffer,
                                      rendering::GraphicsPipeline&& pipeline,
                                      rendering::BindlessImageResourceHandleEntryPair font_atlas,
                                      rendering::xrUniqueVkImageView&& font_atlas_view,
                                      VkSampler sampler);

    UserInterfaceRenderBackend_Vulkan(UserInterfaceRenderBackend_Vulkan&&) noexcept = default;

    static tl::expected<UserInterfaceRenderBackend_Vulkan, rendering::VulkanError> create(
        rendering::VulkanRenderer& renderer,
        const UserInterfaceBackendCreateInfo& backend_create_info);

    UIRenderUniform uniform_data() const noexcept;

    void render(const UserInterfaceRenderContext&,
                rendering::VulkanRenderer& vkr,
                const rendering::FrameRenderData& rd);

  private:
    rendering::VulkanBuffer _vertexbuffer;
    rendering::VulkanBuffer _indexbuffer;
    xray::rendering::GraphicsPipeline _pipeline;
    rendering::BindlessImageResourceHandleEntryPair _font_atlas;
    rendering::xrUniqueVkImageView _font_atlas_view;
    VkSampler _sampler;
};

} // namespace xray::ui
