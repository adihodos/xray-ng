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

} // namespace xray::rendering
