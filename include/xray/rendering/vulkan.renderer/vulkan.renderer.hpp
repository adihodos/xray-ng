#pragma once

#include "xray/xray.hpp"

#include <vector>

#include <tl/optional.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"

namespace swl {
template<typename... Ts>
class variant;
}

namespace xray::rendering {

struct WindowPlatformDataXcb;
struct WindowPlatformDataXlib;
using WindowPlatformData = swl::variant<WindowPlatformDataXcb, WindowPlatformDataXlib>;

namespace detail {

struct SwapchainState
{
    xrUniqueVkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<xrUniqueVkImageView> swapchain_imageviews;
    std::vector<UniqueImage> depth_stencil_images;
    std::vector<xrUniqueVkImageView> depth_stencil_image_views;
};

struct PresentationState
{
    uint32_t frame_index{};
    uint32_t max_frames{};
    xrUniqueVkSurfaceKHR surface;
    SwapchainState swapchain_state;
    std::vector<xrUniqueVkFence> fences;
    std::vector<xrUniqueVkSemaphore> rendering_sem;
    std::vector<xrUniqueVkSemaphore> present_sem;
    std::vector<VkCommandBuffer> command_buffers;
};

struct PhysicalDeviceData
{
    VkPhysicalDevice device;
    struct
    {
        VkPhysicalDeviceProperties2 base;
        VkPhysicalDeviceVulkan11Properties vk11;
        VkPhysicalDeviceVulkan12Properties vk12;
        VkPhysicalDeviceVulkan13Properties vk13;
    } properties;
    struct
    {
        VkPhysicalDeviceFeatures2 base;
        VkPhysicalDeviceVulkan11Features vk11;
        VkPhysicalDeviceVulkan12Features vk12;
        VkPhysicalDeviceVulkan13Features vk13;
    } features;

    VkPhysicalDeviceMemoryProperties memory;
    std::vector<VkQueueFamilyProperties> queue_props;

    explicit PhysicalDeviceData(VkPhysicalDevice dev);
    PhysicalDeviceData(const PhysicalDeviceData& rhs);
    PhysicalDeviceData& operator=(const PhysicalDeviceData& rhs);
};

struct InstanceState
{
    xrUniqueVkInstance handle;
    xrUniqueVkDebugUtilsMessengerEXT debug;
};

struct Queue
{
    uint32_t index;
    VkQueue handle;
    xrUniqueVkCommandPool cmd_pool;
};

struct RenderState
{
    PhysicalDeviceData dev_physical;
    xrUniqueVkDevice dev_logical;
    std::vector<Queue> queues;
};

} // namespace detail

struct FrameRenderData
{
    uint32_t id;
    uint32_t max_frames;
    VkCommandBuffer cmd_buf;
};

class VulkanRenderer
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    static tl::optional<VulkanRenderer> create(const WindowPlatformData& win_data);

    VulkanRenderer(PrivateConstructionToken,
                   detail::InstanceState instance_state,
                   detail::RenderState render_state,
                   detail::PresentationState presentation_state);

    FrameRenderData begin_rendering();
    void end_rendering();

  private:
    const detail::Queue& graphics_queue() const noexcept { return _render_state.queues[0]; }
    const detail::Queue& transfer_queue() const noexcept { return _render_state.queues[1]; }

    detail::InstanceState _instance_state;
    detail::RenderState _render_state;
    detail::PresentationState _presentation_state;
};

} // namespace xray::rendering
