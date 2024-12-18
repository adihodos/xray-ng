#pragma once

#include "xray/xray.hpp"

#include <tl/expected.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"

namespace xray::ui {

struct UserInterfaceBackendCreateInfo;

class UserInterfaceBackend_Vulkan
{
  public:
    static tl::expected<UserInterfaceBackend_Vulkan, rendering::VulkanError> create(
        const UserInterfaceBackendCreateInfo& backend_create_info);

  private:
    rendering::VulkanBuffer _vertexbuffer;
    rendering::VulkanBuffer _indexbuffer;
    rendering::VulkanImage _font_atlas;
    rendering::xrUniqueVkSampler _sampler;
};

} // namespace xray::ui
