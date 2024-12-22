#pragma once

#include "xray/xray.hpp"

#include <tl/expected.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"

namespace xray::rendering {
class VulkanRenderer;
}

namespace xray::ui {

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
                                      rendering::xrUniqueVkSampler&& sampler);

    static tl::expected<UserInterfaceRenderBackend_Vulkan, rendering::VulkanError> create(
        rendering::VulkanRenderer& renderer,
        const UserInterfaceBackendCreateInfo& backend_create_info);

    void render(const UserInterfaceRenderContext&, rendering::VulkanRenderer& vkr);

  private:
    rendering::VulkanBuffer _vertexbuffer;
    rendering::VulkanBuffer _indexbuffer;
    xray::rendering::GraphicsPipeline _pipeline;
    rendering::VulkanImage _font_atlas;
    rendering::xrUniqueVkSampler _sampler;
};

} // namespace xray::ui
