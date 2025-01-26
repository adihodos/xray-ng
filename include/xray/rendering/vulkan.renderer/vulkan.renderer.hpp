#pragma once

#include <span>
#include <tuple>
#include <vector>
#include <filesystem>
#include <initializer_list>
#include <unordered_map>

#include <tl/optional.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "xray/base/xray.misc.hpp"
#include "xray/base/concurrency/spin.mutex.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.work.package.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.handles.hpp"

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
        VkPhysicalDeviceDescriptorIndexingProperties descriptor_indexing;
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
    xrUniqueBufferWithMemory staging_buffer;
    UniqueMemoryMapping mapped_staging_buffer;
    std::atomic_uintptr_t staging_buffer_offset;
    std::vector<Queue> queues;
    xray::base::unique_pointer<xray::base::concurrency::spin_mutex[]> queue_cmd_pool_mutex;
    xray::base::unique_pointer<xray::base::concurrency::spin_mutex[]> queue_submit_mutex;
    RenderingAttachments attachments;

    RenderState(const PhysicalDeviceData& pd,
                xrUniqueVkDevice&& logical,
                xrUniqueBufferWithMemory&& staging,
                UniqueMemoryMapping&& mapped_staging,
                std::vector<Queue>&& qs,
                RenderingAttachments&& atts)
        : dev_physical{ pd }
        , dev_logical{ std::move(logical) }
        , staging_buffer{ std::move(staging) }
        , mapped_staging_buffer{ std::move(mapped_staging) }
        , staging_buffer_offset{ 0 }
        , queues{ std::move(qs) }
        , queue_cmd_pool_mutex{ new xray::base::concurrency::spin_mutex[queues.size()] }
        , queue_submit_mutex{ new xray::base::concurrency::spin_mutex[queues.size()] }
        , attachments{ std::move(atts) }
    {
    }

    RenderState(RenderState&& rhs) noexcept
        : dev_physical(std::move(rhs.dev_physical))
        , dev_logical(std::move(rhs.dev_logical))
        , staging_buffer(std::move(rhs.staging_buffer))
        , mapped_staging_buffer(std::move(rhs.mapped_staging_buffer))
        , staging_buffer_offset(rhs.staging_buffer_offset.load())
        , queues(std::move(rhs.queues))
        , queue_cmd_pool_mutex(std::move(rhs.queue_cmd_pool_mutex))
        , queue_submit_mutex(std::move(rhs.queue_submit_mutex))
        , attachments(std::move(rhs.attachments))
    {
    }
};

struct DescriptorPoolState
{
    xrUniqueVkDescriptorPool handle;
};

} // namespace detail

struct FrameRenderData
{
    uint32_t id;
    uint32_t max_frames;
    VkCommandBuffer cmd_buf;
    VkExtent2D fbsize;
};

struct RenderBufferingSetup
{
    uint32_t frame_id;
    uint32_t buffers;
};

struct WorkPackageSetup
{
    WorkPackageHandle pkg;
    VkCommandBuffer cmdbuf;
};

struct BufferWithDeviceMemoryPair
{
    StagingBufferHandle buffer;
    StagingBufferMemoryHandle memory;
};

struct WorkPackageTrackingInfo
{
    std::vector<BufferWithDeviceMemoryPair> staging_buffers;
    FenceHandle fence;
    CommandBufferHandle cmd_buf;
};

struct WorkPackageState
{
    VkDevice device{};
    VkQueue queue{};
    VkCommandPool cmd_pool{};

    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkBuffer> staging_buffers;
    std::vector<VkDeviceMemory> staging_memory;
    std::vector<VkFence> fences;

    std::unordered_map<WorkPackageHandle, WorkPackageTrackingInfo> packages;

    static tl::expected<WorkPackageState, VulkanError> create(VkDevice device,
                                                              VkQueue queue,
                                                              const uint32_t queue_family_idx);

    WorkPackageState(VkDevice device, VkQueue queue, VkCommandPool cmd_pool);
    ~WorkPackageState();
    WorkPackageState(const WorkPackageState&) = delete;
    WorkPackageState& operator=(const WorkPackageState&) = delete;
    WorkPackageState(WorkPackageState&& rhs) noexcept;
};

struct StagingBuffer
{
    VkBuffer buf;
    VkDeviceMemory mem;
};

enum class QueueType : uint8_t
{
    Graphics,
    Transfer,
};

struct QueueSubmitWaitToken;
struct RendererConfig;

class VulkanRenderer
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    static tl::optional<VulkanRenderer> create(const WindowPlatformData& win_data, const RendererConfig& cfg);

    VulkanRenderer(PrivateConstructionToken,
                   detail::InstanceState instance_state,
                   detail::RenderState render_state,
                   detail::PresentationState presentation_state,
                   detail::DescriptorPoolState pool_state,
                   WorkPackageState wpkg_state,
                   BindlessSystem bindless);

    FrameRenderData begin_rendering();
    void end_rendering();
    void clear_attachments(VkCommandBuffer cmd_buf,
                           const float red,
                           const float green,
                           const float blue,
                           const float depth = 1.0,
                           const uint32_t stencil = 0);

    void wait_device_idle() noexcept;

    VkDevice device() const noexcept { return xray::base::raw_ptr(_render_state.dev_logical); }

    const detail::PhysicalDeviceData& physical() const noexcept { return _render_state.dev_physical; }

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

    RenderBufferingSetup buffering_setup() const noexcept
    {
        return { _presentation_state.frame_index, _presentation_state.max_frames };
    }

    uint32_t max_inflight_frames() const noexcept { return _presentation_state.max_frames; }

    uint32_t find_allocation_memory_type(const uint32_t memory_requirements,
                                         const VkMemoryPropertyFlags required_flags) const noexcept;

    void release_staging_resources();

    void wait_on_packages(std::initializer_list<WorkPackageHandle> pkgs) const noexcept;

    tl::expected<WorkPackageSetup, VulkanError> create_work_package();

    void submit_work_package(const WorkPackageHandle pkg_handle);

    tl::expected<StagingBuffer, VulkanError> create_staging_buffer(WorkPackageHandle pkg, const size_t bytes_size);

    VkCommandBuffer get_cmd_buf_for_work_package(const WorkPackageHandle pkg)
    {
        return _work_queue.command_buffers[_work_queue.packages.find(pkg)->second.cmd_buf.value_of()];
    }

    // @group Bindless resource handling
    const BindlessSystem& bindless_sys() const noexcept { return _bindless; }
    BindlessSystem& bindless_sys() noexcept { return _bindless; }
    VkDescriptorPool descriptor_pool() const noexcept { return xray::base::raw_ptr(_dpool_state.handle); }
    // @endgroup

    // @group Debugging
    template<typename VkObjectType>
    void dbg_set_object_name(VkObjectType vkobj, const char* name) const noexcept;
    void dbg_set_object_name(const uint64_t object,
                             const VkDebugReportObjectTypeEXT obj_type,
                             const char* name) const noexcept;
    void dbg_marker_begin(VkCommandBuffer cmd_buf, const char* name, const rgb_color color) noexcept;
    void dbg_marker_end(VkCommandBuffer cmd_buf) noexcept;
    void dbg_marker_insert(VkCommandBuffer cmd_buf, const char* name, const rgb_color color) noexcept;
    // @endgroup

    // @group Misc
    void add_shader_include_directories(std::initializer_list<std::filesystem::path> include_dirs)
    {
        _shader_include_directories.assign(include_dirs);
    }

    std::span<const std::filesystem::path> shader_include_directories() const noexcept
    {
        return std::span{ _shader_include_directories };
    }
    // @endgroup

    std::tuple<VkQueue, uint32_t, VkCommandPool> queue_data(const uint32_t idx) const noexcept
    {
        return { _render_state.queues[idx].handle,
                 _render_state.queues[idx].index,
                 xray::base::unique_pointer_get_ptr(_render_state.queues[idx].cmd_pool) };
    }

    struct QueueData
    {
        VkQueue handle;
        uint32_t family;
        VkCommandPool cmdpool;
        std::reference_wrapper<xray::base::concurrency::spin_mutex> cmdpool_lock;
        std::reference_wrapper<xray::base::concurrency::spin_mutex> submit_lock;
    };

    QueueData queue_data(const QueueType qtype) noexcept
    {
        return QueueData{
            .handle = _render_state.queues[static_cast<uint32_t>(qtype)].handle,
            .family = _render_state.queues[static_cast<uint32_t>(qtype)].index,
            .cmdpool = xray::base::unique_pointer_get_ptr(_render_state.queues[static_cast<uint32_t>(qtype)].cmd_pool),
            .cmdpool_lock = std::reference_wrapper{ _render_state.queue_cmd_pool_mutex[static_cast<uint32_t>(qtype)] },
            .submit_lock = std::reference_wrapper{ _render_state.queue_submit_mutex[static_cast<uint32_t>(qtype)] },
        };
    }
    //
    uintptr_t reserve_staging_buffer_memory(const size_t bytes) noexcept
    {
        const uintptr_t aligned_size = xray::base::align<uintptr_t>(
            bytes, this->_render_state.dev_physical.properties.base.properties.limits.nonCoherentAtomSize);
        return _render_state.staging_buffer_offset.fetch_add(aligned_size);
    }

    uintptr_t staging_buffer_memory() const noexcept
    {
        return reinterpret_cast<uintptr_t>(_render_state.mapped_staging_buffer._mapped_memory);
    }

    VkBuffer staging_buffer() const noexcept { return _render_state.staging_buffer.handle<VkBuffer>(); }

    void queue_image_ownership_transfer(const BindlessResourceHandle_Image img) { _ownership_transfers.push_back(img); }

    /// @group async tasks

    tl::expected<VkCommandBuffer, VulkanError> create_job(const QueueType qtype) noexcept;
    tl::expected<QueueSubmitWaitToken, VulkanError> submit_job(VkCommandBuffer cmd_buffer,
                                                               const QueueType qtype) noexcept;
    /// @endgroup

  private:
    const detail::Queue& graphics_queue() const noexcept { return _render_state.queues[0]; }
    const detail::Queue& transfer_queue() const noexcept { return _render_state.queues[1]; }

    enum class SwapchainReacquireAfterSuboptimal
    {
        Always_,
        Never_
    };

    void handle_swapchain_suboptimal_out_of_date(const SwapchainReacquireAfterSuboptimal reacquire);

    detail::InstanceState _instance_state;
    detail::RenderState _render_state;
    detail::PresentationState _presentation_state;
    detail::DescriptorPoolState _dpool_state;
    WorkPackageState _work_queue;
    BindlessSystem _bindless;
    std::vector<std::filesystem::path> _shader_include_directories;
    std::vector<BindlessResourceHandle_Image> _ownership_transfers;
};

template<typename VkObjectType>
void
VulkanRenderer::dbg_set_object_name(VkObjectType vkobj, const char* name) const noexcept
{
    if constexpr (std::is_same_v<VkBuffer, VkObjectType>) {
        dbg_set_object_name(reinterpret_cast<uint64_t>(vkobj), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, name);
    } else if constexpr (std::is_same_v<VkImage, VkObjectType>) {
        dbg_set_object_name(reinterpret_cast<uint64_t>(vkobj), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, name);
    } else {
        static_assert(false, "Unsupported object type!");
    }
}

uint32_t
vk_format_bytes_size(const VkFormat format);

} // namespace xray::rendering
