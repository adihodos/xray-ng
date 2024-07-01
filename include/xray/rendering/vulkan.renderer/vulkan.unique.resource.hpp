#pragma once

#include "xray/xray.hpp"
#include "xray/base/unique_pointer.hpp"

#include <string_view>
#include <type_traits>

#include <tl/optional.hpp>

#include <vulkan/vulkan.h>

#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatch.hpp"

namespace xray::rendering {

struct NotOwnedVulkanResource
{};

template<typename VulkanResourceType, typename VulkanResourceOwner>
struct VulkanResourceDeleterBase
{
    using pointer = VulkanResourceType;

    // VulkanResourceDeleterBase() = default;

    explicit VulkanResourceDeleterBase(VulkanResourceOwner owner,
                                       const VkAllocationCallbacks* alloc_cb = nullptr) noexcept
        : _owner{ owner }
        , _alloc_cb{ alloc_cb }
    {
    }

    VulkanResourceOwner _owner{};
    const VkAllocationCallbacks* _alloc_cb{};
};

template<typename VulkanResource, typename VulkanResourceOwner, typename DestructorFn>
void
dispatch_destructor_func(VulkanResource resource,
                         VulkanResourceOwner owner,
                         DestructorFn destroy_fn,
                         const VkAllocationCallbacks* alloc_cb)
{
    if constexpr (std::is_same_v<VulkanResourceOwner, NotOwnedVulkanResource>) {
        WRAP_VULKAN_FUNC(destroy_fn, resource, alloc_cb);
    } else {
        WRAP_VULKAN_FUNC(destroy_fn, owner, resource, alloc_cb);
    }
}

#define XR_DECLARE_VULKAN_UNIQUE_RESOURCE(res_type, owner_type, destructor)                                            \
    struct VkResourceDeleter_##res_type : public VulkanResourceDeleterBase<res_type, owner_type>                       \
    {                                                                                                                  \
        using VulkanResourceDeleterBase::pointer;                                                                      \
        using VulkanResourceDeleterBase::VulkanResourceDeleterBase;                                                    \
        void operator()(pointer p) const noexcept                                                                      \
        {                                                                                                              \
            if (p) {                                                                                                   \
                xray::rendering::dispatch_destructor_func(p, _owner, destructor, _alloc_cb);                           \
            }                                                                                                          \
        }                                                                                                              \
    };                                                                                                                 \
    using xrUnique##res_type = xray::base::unique_pointer<res_type, VkResourceDeleter_##res_type>;

XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkInstance, NotOwnedVulkanResource, vkDestroyInstance)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkDevice, NotOwnedVulkanResource, vkDestroyDevice)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkSurfaceKHR, VkInstance, vkDestroySurfaceKHR)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkDebugUtilsMessengerEXT, VkInstance, vkfn::DestroyDebugUtilsMessengerEXT)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkImageView, VkDevice, vkDestroyImageView)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkSwapchainKHR, VkDevice, vkDestroySwapchainKHR)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkCommandPool, VkDevice, vkDestroyCommandPool)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkBuffer, VkDevice, vkDestroyBuffer)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkImage, VkDevice, vkDestroyImage)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkDeviceMemory, VkDevice, vkFreeMemory)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkShaderModule, VkDevice, vkDestroyShaderModule)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkDescriptorSetLayout, VkDevice, vkDestroyDescriptorSetLayout)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkPipelineLayout, VkDevice, vkDestroyPipelineLayout)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkPipeline, VkDevice, vkDestroyPipeline)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkFence, VkDevice, vkDestroyFence)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkSemaphore, VkDevice, vkDestroySemaphore)

struct GraphicsPipeline
{
    xrUniqueVkPipeline pipeline{ nullptr, VkResourceDeleter_VkPipeline{ nullptr } };
    xrUniqueVkPipelineLayout layout{ nullptr, VkResourceDeleter_VkPipelineLayout{ nullptr } };
    std::vector<xrUniqueVkDescriptorSetLayout> descriptor_set_layout;
};

struct UniqueBuffer
{
    xrUniqueVkBuffer buffer;
    xrUniqueVkDeviceMemory memory;
};

struct UniqueImage
{
    xrUniqueVkImage image;
    xrUniqueVkDeviceMemory memory;

    explicit operator bool() const noexcept { return image && memory; }
};

struct UniqueMemoryMapping
{
    ~UniqueMemoryMapping();

    static tl::optional<UniqueMemoryMapping> create(VkDevice device,
                                                    VkDeviceMemory memory,
                                                    const uint64_t offset,
                                                    const uint64_t size,
                                                    const VkMemoryMapFlags flags);

    void* _mapped_memory{};
    VkDeviceMemory _device_memory{};
    VkDevice _device{};
    uint64_t _mapped_size{};
    uint64_t _mapped_offset{};
};

template<typename VkResource>
struct VkResourceDeleter;

template<>
struct VkResourceDeleter<VkImage>
{
    void operator()(VkDevice device, VkImage image, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (image)
            vkDestroyImage(device, image, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkImageView>
{
    void operator()(VkDevice device, VkImageView image_view, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (image_view)
            vkDestroyImageView(device, image_view, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkDeviceMemory>
{
    void operator()(VkDevice device, VkDeviceMemory device_memory, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (device_memory)
            vkFreeMemory(device, device_memory, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkBuffer>
{
    void operator()(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (buffer)
            vkDestroyBuffer(device, buffer, alloc_cb);
    }
};

template<typename Owner, typename... Resources>
struct UniqueVulkanResourcePack
{
    UniqueVulkanResourcePack(Owner owner, Resources... args) noexcept
        : _owner{ owner }
        , _resources{ std::forward<Resources>(args)... }
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
    }

    UniqueVulkanResourcePack() = default;

    UniqueVulkanResourcePack(const UniqueVulkanResourcePack&) = delete;
    UniqueVulkanResourcePack& operator=(const UniqueVulkanResourcePack) = delete;

    UniqueVulkanResourcePack(UniqueVulkanResourcePack&& rhs) noexcept
        : _owner{ rhs._owner }
        , _resources{ std::move(rhs._resources) }
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        rhs._resources = std::make_tuple(Resources{}...);
    }

    UniqueVulkanResourcePack& operator=(UniqueVulkanResourcePack&& rhs) noexcept
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (this != &rhs) {
            std::apply(
                [this](auto&&... args) {
                    (void((VkResourceDeleter<std::decay_t<decltype(args)>>{})(
                         _owner, std::forward<decltype(args)>(args), nullptr)),
                     ...);
                },
                _resources);
            _resources = std::move(rhs._resources);
            rhs._resources = std::make_tuple(Resources{}...);
        }
        return *this;
    }

    template<typename ResourceHandleType>
    auto handle() const noexcept
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        return std::get<ResourceHandleType>(_resources);
    }

    explicit operator bool() const noexcept
    {
        return std::apply([](auto&&... args) { return ((args != VK_NULL_HANDLE) && ...); }, _resources);
    }

    ~UniqueVulkanResourcePack() noexcept
    {
        XR_LOG_INFO("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        std::apply(
            [this](auto&&... args) {
                (void((VkResourceDeleter<std::decay_t<decltype(args)>>{})(
                     _owner, std::forward<decltype(args)>(args), nullptr)),
                 ...);
            },
            _resources);
    }

    std::tuple<Resources...> _resources;
    Owner _owner;
};

using xrUniqueBufferWithMemory = UniqueVulkanResourcePack<VkDevice, VkBuffer, VkDeviceMemory>;
using xrUniqueImageWithMemory = UniqueVulkanResourcePack<VkDevice, VkImage, VkDeviceMemory>;
using xrUniqueImageWithMemoryAndView = UniqueVulkanResourcePack<VkDevice, VkImage, VkDeviceMemory, VkImageView>;

struct ManagedUniqueBuffer
{
    static constexpr const uint32_t NO_FRAME{ 0xffffffffu };

    xrUniqueBufferWithMemory buffer;
    uint32_t aligned_size;
    uint32_t alignment;

    VkDeviceMemory memory_handle() const noexcept { return buffer.handle<VkDeviceMemory>(); }
    VkBuffer buffer_handle() const noexcept { return buffer.handle<VkBuffer>(); }

    tl::optional<UniqueMemoryMapping> mmap(VkDevice device, const uint32_t frame_id = NO_FRAME) const noexcept
    {
        return UniqueMemoryMapping::create(device,
                                           buffer.handle<VkDeviceMemory>(),
                                           frame_id == NO_FRAME ? 0 : aligned_size * frame_id,
                                           aligned_size,
                                           0);
    }
};

} // namespace xray::rendering