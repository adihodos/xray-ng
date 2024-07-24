#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"

#include <system_error>

#include <ktx.h>
#include <mio/mmap.hpp>

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

auto
ktx_lvl_iter_fn(int mip_lvl, int face, int w, int h, int depth, ktx_uint64_t face_lod, void* pixels, void* userdata)
    -> KTX_error_code
{
    XR_LOG_INFO("Mip lvl {}, face {}, {}x{}x{}", mip_lvl, face, w, h, depth);
    return KTX_SUCCESS;
};

tl::expected<xray::rendering::ManagedImage, xray::rendering::VulkanError>
xray::rendering::ManagedImage::from_file(xray::rendering::VulkanRenderer& renderer,
                                         const std::filesystem::path& texture_file)
{
    std::error_code err_code{};
    const mio::mmap_source texf{ mio::make_mmap_source(texture_file.generic_string(), err_code) };
    if (err_code) {
        XR_LOG_ERR("Failed to open texture file {}, error {}", texture_file.generic_string(), err_code.message());
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
    }

    ktxTexture2* ktx_texture{};
    const KTX_error_code tex_load_result =
        ktxTexture2_CreateFromMemory(reinterpret_cast<const ktx_uint8_t*>(texf.data()),
                                     texf.size(),
                                     KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                     &ktx_texture);

    if (tex_load_result != KTX_SUCCESS) {
        XR_LOG_ERR("Failed to load KTX2 texture, {}", static_cast<int32_t>(tex_load_result));
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
    }

    const ktx_size_t data_size = ktxTexture_GetDataSize(ktxTexture(ktx_texture));
    const ktx_size_t data_size_uncompressed = ktxTexture_GetDataSizeUncompressed(ktxTexture(ktx_texture));
    const ktx_size_t image_size = ktxTexture_GetImageSize(ktxTexture(ktx_texture), 0);

    auto lvl_iter_fn =
        [](int mip_lvl, int face, int w, int h, int depth, ktx_uint64_t face_lod, void* pixels, void* userdata)
        -> KTX_error_code {
        XR_LOG_INFO("Mip lvl {}, face {}, {}x{}x{}", mip_lvl, face, w, h, depth);
        return KTX_SUCCESS;
    };

    ktxTexture_IterateLevels(ktxTexture(ktx_texture), &ktx_lvl_iter_fn, nullptr);
    ktxTexture_IterateLevelFaces(ktxTexture(ktx_texture), &ktx_lvl_iter_fn, nullptr);

    XR_LOG_INFO("Texture {}, data size {}, data size uncompressed {}, image size {}",
                texture_file.generic_string(),
                data_size,
                data_size_uncompressed,
                image_size);

    return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
}
