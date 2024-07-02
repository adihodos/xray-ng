#pragma once

#include "xray/xray.hpp"

#include <span>
#include <tuple>
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

#if defined(XRAY_OS_IS_WINDOWS)
struct WindowPlatformDataWin32;
using WindowPlatformData = swl::variant<WindowPlatformDataWin32>;
#else
struct WindowPlatformDataXcb;
struct WindowPlatformDataXlib;
using WindowPlatformData = swl::variant<WindowPlatformDataXcb, WindowPlatformDataXlib>;
#endif

namespace detail {

struct SyncState
{
    std::vector<xrUniqueVkFence> fences;
    std::vector<xrUniqueVkSemaphore> rendering_sem;
    std::vector<xrUniqueVkSemaphore> present_sem;
};

struct SwapchainState
{
    xrUniqueVkSwapchainKHR swapchain;
    std::vector<VkImage> swapchain_images;
    std::vector<xrUniqueVkImageView> swapchain_imageviews;
    std::vector<UniqueImage> depth_stencil_images;
    std::vector<xrUniqueVkImageView> depth_stencil_image_views;
    SyncState sync;
};

struct SurfaceState
{
    xrUniqueVkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode;
    VkFormat depth_stencil_format;
};

struct PresentationState
{
    static constexpr const uint32_t STATE_SWAPCHAIN_SUBOPTIMAL{ 0x1 };
    uint32_t frame_index{};
    uint32_t max_frames{};
    uint32_t acquired_image{};
    uint32_t state_bits{};
    SurfaceState surface_state;
    SwapchainState swapchain_state;
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

struct RenderingAttachments
{
    uint32_t view_mask;
    // [0, size - 2) - color attachments
    // [size - 2, ...) - dept + stencil
    std::vector<VkFormat> attachments;
};

struct RenderState
{
    PhysicalDeviceData dev_physical;
    xrUniqueVkDevice dev_logical;
    std::vector<Queue> queues;
    RenderingAttachments attachments;
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

    FrameRenderData begin_rendering(const VkRect2D& render_area);
    void end_rendering();
    void clear_attachments(VkCommandBuffer cmd_buf,
                           const float red,
                           const float green,
                           const float blue,
                           const uint32_t width,
                           const uint32_t height,
                           const float depth = 1.0,
                           const uint32_t stencil = 0);

    void wait_device_idle() noexcept;

    VkDevice device() const noexcept { return xray::base::raw_ptr(_render_state.dev_logical); }

    const detail::SurfaceState& surface_state() const noexcept { return _presentation_state.surface_state; }

    std::tuple<uint32_t, std::span<const VkFormat>, VkFormat, VkFormat> pipeline_render_create_info() const noexcept
    {
        const size_t att_count = _render_state.attachments.attachments.size();

        return {
            _render_state.attachments.view_mask,
            std::span{ _render_state.attachments.attachments.cbegin(), att_count - 2 },
            _render_state.attachments.attachments[att_count - 2],
            _render_state.attachments.attachments[att_count - 1],
        };
    }

    std::pair<uint32_t, uint32_t> buffering_setup() const noexcept
    {
        return { _presentation_state.frame_index, _presentation_state.max_frames };
    }

    uint32_t max_inflight_frames() const noexcept { return _presentation_state.max_frames; }

    tl::optional<ManagedUniqueBuffer> create_buffer(const VkBufferUsageFlags usage,
                                                    const size_t bytes,
                                                    const size_t frames,
                                                    const VkMemoryPropertyFlags memory_properties) noexcept;

  private:
    const detail::Queue& graphics_queue() const noexcept { return _render_state.queues[0]; }
    const detail::Queue& transfer_queue() const noexcept { return _render_state.queues[1]; }

    detail::InstanceState _instance_state;
    detail::RenderState _render_state;
    detail::PresentationState _presentation_state;
};

uint32_t
vk_format_bytes_size(const VkFormat format);

} // namespace xray::rendering
