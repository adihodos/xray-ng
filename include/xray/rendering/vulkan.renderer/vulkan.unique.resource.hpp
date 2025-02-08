#pragma once

#include "xray/xray.hpp"

#include <string_view>
#include <type_traits>

#include <tl/optional.hpp>
#include <tl/expected.hpp>
#include <vulkan/vulkan_core.h>

#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatch.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"

namespace xray::rendering {

struct NotOwnedVulkanResource
{};

template<typename VulkanResourceType, typename VulkanResourceOwner>
struct VulkanResourceDeleterBase
{
    using pointer = VulkanResourceType;

    VulkanResourceDeleterBase() = default;
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
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkDescriptorPool, VkDevice, vkDestroyDescriptorPool)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkBufferView, VkDevice, vkDestroyBufferView)
XR_DECLARE_VULKAN_UNIQUE_RESOURCE(VkSampler, VkDevice, vkDestroySampler)

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
    UniqueMemoryMapping() = default;
    UniqueMemoryMapping(void* mapped_addr,
                        VkDeviceMemory device_mem,
                        VkDevice device,
                        const uint64_t mapped_size,
                        const uint64_t mapped_offset) noexcept
        : _mapped_memory{ mapped_addr }
        , _device_memory{ device_mem }
        , _device{ device }
        , _mapped_size{ mapped_size }
        , _mapped_offset{ mapped_offset }
    {
    }

    ~UniqueMemoryMapping();

    UniqueMemoryMapping(UniqueMemoryMapping&& rhs)
    {
        memcpy(this, &rhs, sizeof(*this));
        memset(&rhs, 0, sizeof(rhs));
    }

    UniqueMemoryMapping& operator=(UniqueMemoryMapping&& rhs)
    {
        if (this != &rhs) {
            memcpy(this, &rhs, sizeof(*this));
            memset(&rhs, 0, sizeof(rhs));
        }
        return *this;
    }

    static tl::expected<UniqueMemoryMapping, VulkanError> map_memory(VkDevice device,
                                                                     VkDeviceMemory memory,
                                                                     const uint64_t offset,
                                                                     const uint64_t size) noexcept;

    template<typename T>
    T* as() noexcept
    {
        return static_cast<T*>(_mapped_memory);
    }

    template<typename T>
    const T* as() const noexcept
    {
        return static_cast<T*>(_mapped_memory);
    }

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
        XR_LOG_TRACE("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (image)
            vkDestroyImage(device, image, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkImageView>
{
    void operator()(VkDevice device, VkImageView image_view, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_TRACE("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (image_view)
            vkDestroyImageView(device, image_view, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkDeviceMemory>
{
    void operator()(VkDevice device, VkDeviceMemory device_memory, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_TRACE("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (device_memory)
            vkFreeMemory(device, device_memory, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkBuffer>
{
    void operator()(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_TRACE("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (buffer)
            vkDestroyBuffer(device, buffer, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkDescriptorPool>
{
    void operator()(VkDevice device, VkDescriptorPool dpool, const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_TRACE("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (dpool)
            vkDestroyDescriptorPool(device, dpool, alloc_cb);
    }
};

template<>
struct VkResourceDeleter<VkPipelineLayout>
{
    void operator()(VkDevice device,
                    VkPipelineLayout pipeline_layout,
                    const VkAllocationCallbacks* alloc_cb) const noexcept
    {
        XR_LOG_TRACE("{}", XRAY_QUALIFIED_FUNCTION_NAME);
        if (pipeline_layout)
            vkDestroyPipelineLayout(device, pipeline_layout, alloc_cb);
    }
};

template<typename Owner, typename... Resources>
struct UniqueVulkanResourcePack
{
    UniqueVulkanResourcePack(Owner owner, Resources... args) noexcept
        : _owner{ owner }
        , _resources{ std::forward<Resources>(args)... }
    {
    }

    UniqueVulkanResourcePack() = default;

    UniqueVulkanResourcePack(const UniqueVulkanResourcePack&) = delete;
    UniqueVulkanResourcePack& operator=(const UniqueVulkanResourcePack) = delete;

    UniqueVulkanResourcePack(UniqueVulkanResourcePack&& rhs) noexcept
        : _owner{ rhs._owner }
        , _resources{ std::move(rhs._resources) }
    {
        rhs._resources = std::make_tuple(Resources{}...);
    }

    UniqueVulkanResourcePack& operator=(UniqueVulkanResourcePack&& rhs) noexcept
    {
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

    std::tuple<Resources...> release() noexcept
    {
        std::tuple<Resources...> temp{};
        std::swap(this->_resources, temp);
        return temp;
    }

    template<typename ResourceHandleType>
    auto handle() const noexcept
    {
        return std::get<ResourceHandleType>(_resources);
    }

    explicit operator bool() const noexcept
    {
        return std::apply([](auto&&... args) { return ((args != VK_NULL_HANDLE) && ...); }, _resources);
    }

    ~UniqueVulkanResourcePack() noexcept
    {
        XR_LOG_TRACE("{}", XRAY_QUALIFIED_FUNCTION_NAME);
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

using xrUniqueImageWithMemory = UniqueVulkanResourcePack<VkDevice, VkImage, VkDeviceMemory>;
using xrUniqueImageWithMemoryAndView = UniqueVulkanResourcePack<VkDevice, VkImage, VkDeviceMemory, VkImageView>;

template<typename ResourceDeleter, typename... Containers>
void
free_multiple_resources(ResourceDeleter deleter, Containers&&... containers)
{
    (
        [&deleter](Containers&& c) {
            for (auto&& r : c) {
                deleter(r);
            }
        }(std::forward<Containers>(containers)),
        ...);
}

} // namespace xray::rendering
