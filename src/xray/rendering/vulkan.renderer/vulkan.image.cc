#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

tl::expected<xray::rendering::ManagedImage, xray::rendering::VulkanError>
xray::rendering::ManagedImage::create(xray::rendering::VulkanRenderer& renderer,
                                      const VkImageCreateInfo& img_create_info,
                                      const VkMemoryPropertyFlags image_memory_properties)
{
    VkImage img_handle{};
    const VkResult create_result =
        WRAP_VULKAN_FUNC(vkCreateImage, renderer.device(), &img_create_info, nullptr, &img_handle);
    if (create_result != VK_SUCCESS) {
        return XR_MAKE_VULKAN_ERROR(create_result);
    }

    xrUniqueVkImage image{ img_handle, VkResourceDeleter_VkImage{ renderer.device() } };

    const VkImageMemoryRequirementsInfo2 mem_req_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
        .pNext = nullptr,
        .image = img_handle,
    };

    VkMemoryRequirements2 mem_req{ .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, .pNext = nullptr };
    vkGetImageMemoryRequirements2(renderer.device(), &mem_req_info, &mem_req);

    const VkMemoryAllocateInfo mem_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.memoryRequirements.size,
        .memoryTypeIndex =
            renderer.find_allocation_memory_type(mem_req.memoryRequirements.memoryTypeBits, image_memory_properties),
    };

    VkResult img_alloc_result{};
    xrUniqueVkDeviceMemory image_memory{
        [d = renderer.device(), ma = &mem_alloc_info, &img_alloc_result]() {
            VkDeviceMemory image_memory{};
            img_alloc_result = WRAP_VULKAN_FUNC(vkAllocateMemory, d, ma, nullptr, &image_memory);
            return image_memory;
        }(),
        VkResourceDeleter_VkDeviceMemory{ renderer.device() },
    };

    if (img_alloc_result != VK_SUCCESS) {
        return XR_MAKE_VULKAN_ERROR(img_alloc_result);
    }

    return ManagedImage{
        xrUniqueImageWithMemory{
            renderer.device(),
            xray::base::unique_pointer_release(image),
            xray::base::unique_pointer_release(image_memory),
        },
        img_create_info.extent.width,
        img_create_info.extent.height,
        img_create_info.extent.depth,
        img_create_info.format,
    };
}
