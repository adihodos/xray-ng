#pragma once

#include "xray/xray.hpp"
#include "xray/base/unique_pointer.hpp"

#include <string_view>
#include <type_traits>

#include <vulkan/vulkan.h>

#include "xray/rendering/renderer.vulkan/vulkan.call.wrapper.hpp"
#include "xray/rendering/renderer.vulkan/vulkan.dynamic.dispatch.hpp"

namespace xray::rendering {

struct NotOwnedVulkanResource
{};

template<typename VulkanResourceType, typename VulkanResourceOwner>
struct VulkanResourceDeleterBase
{
    using pointer = VulkanResourceType;

    explicit VulkanResourceDeleterBase(VulkanResourceOwner owner,
                                       const VkAllocationCallbacks* alloc_cb = nullptr) noexcept
        : _owner{ owner }
        , _alloc_cb{ alloc_cb }
    {
    }

    VulkanResourceOwner _owner;
    const VkAllocationCallbacks* _alloc_cb;
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

} // namespace xray::rendering
