#pragma once

#include "xray/xray.hpp"

#include <tl/optional.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "xray/rendering/renderer.vulkan/vulkan.unique.resource.hpp"

namespace swl {
template<typename... Ts>
class variant;
}

namespace xray::rendering {

struct WindowPlatformDataXcb;
struct WindowPlatformDataXlib;

using WindowPlatformData = swl::variant<WindowPlatformDataXcb, WindowPlatformDataXlib>;

class VulkanRenderer
{
  public:
    static tl::optional<VulkanRenderer> create(const WindowPlatformData& win_data);

  private:
    xrUniqueVkInstance _vkinstance;
    xrUniqueVkDevice _vkdevice;

    struct
    {
        xrUniqueVkDebugUtilsMessengerEXT msg;
    } _debug;
};

} // namespace xray::rendering
