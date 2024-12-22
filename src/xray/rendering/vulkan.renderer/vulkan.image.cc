#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"

#include <cmath>
#include <system_error>

#include <Lz/Lz.hpp>
#include <itlib/small_vector.hpp>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <ktx.h>
#include <mio/mmap.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

//
// Most of the code in this file was adapted from https://github.com/KhronosGroup/KTX-Software, vkloader.c

namespace ktx_internal_details {

/*
 * Pad nbytes to next multiple of n
 */
#define _KTX_PADN(n, nbytes) (ktx_uint32_t)(n * ceilf((float)(nbytes) / n))
/*
 * Calculate bytes of of padding needed to reach next multiple of n.
 */
/* Equivalent to (n * ceil(nbytes / n)) - nbytes */
#define _KTX_PADN_LEN(n, nbytes) (ktx_uint32_t)((n * ceilf((float)(nbytes) / n)) - (nbytes))

/*
 * Pad nbytes to next multiple of 4
 */
#define _KTX_PAD4(nbytes) _KTX_PADN(4, nbytes)
/*
 * Calculate bytes of of padding needed to reach next multiple of 4.
 */
#define _KTX_PAD4_LEN(nbytes) _KTX_PADN_LEN(4, nbytes)

/*
 * Pad nbytes to next multiple of 8
 */
#define _KTX_PAD8(nbytes) _KTX_PADN(8, nbytes)
/*
 * Calculate bytes of of padding needed to reach next multiple of 8.
 */
#define _KTX_PAD8_LEN(nbytes) _KTX_PADN_LEN(8, nbytes)

/*
 * Pad nbytes to KTX_GL_UNPACK_ALIGNMENT
 */
#define _KTX_PAD_UNPACK_ALIGN(nbytes) _KTX_PADN(KTX_GL_UNPACK_ALIGNMENT, nbytes)
/*
 * Calculate bytes of of padding needed to reach KTX_GL_UNPACK_ALIGNMENT.
 */
#define _KTX_PAD_UNPACK_ALIGN_LEN(nbytes) _KTX_PADN_LEN(KTX_GL_UNPACK_ALIGNMENT, nbytes)

// Recursive function to return the greatest common divisor of a and b.
static uint32_t
gcd(uint32_t a, uint32_t b)
{
    if (a == 0)
        return b;
    return gcd(b % a, a);
}

// Function to return the least common multiple of a & 4.
uint32_t
lcm4(uint32_t a)
{
    if (!(a & 0x03))
        return a; // a is a multiple of 4.
    return (a * 4) / gcd(a, 4);
}

/** @memberof ktxTexture2
 * @~English
 * @brief Return the VkFormat enum of a ktxTexture2 object.
 *
 * @return The VkFormat of the texture object.
 */
VkFormat
ktxTexture2_GetVkFormat(ktxTexture2* This)
{
    return static_cast<VkFormat>(This->vkFormat);
}

/** @memberof ktxTexture
 * @~English
 * @brief Return the VkFormat enum of a ktxTexture object.
 *
 * In ordert to ensure that the Vulkan uploader is not linked into an application unless explicitly called,
 * this is not a virtual function. It determines the texture type then dispatches to the correct function.
 * @sa @ref ktxTexture1::ktxTexture1_GetVkFormat "ktxTexture1_GetVkFormat()"
 * @sa @ref ktxTexture2::ktxTexture2_GetVkFormat "ktxTexture2_GetVkFormat()"
 */
VkFormat
ktxTexture_GetVkFormat(ktxTexture* This)
{
    assert(This->classId == ktxTexture2_c);
    return ktxTexture2_GetVkFormat((ktxTexture2*)This);
}

struct user_cbdata_optimal
{
    VkBufferImageCopy* region; // Specify destination region in final image.
    VkDeviceSize offset;       // Offset of current level in staging buffer
    ktx_uint32_t numFaces;
    ktx_uint32_t numLayers;
    // The following are used only by optimalTilingPadCallback
    ktx_uint8_t* dest; // Pointer to mapped staging buffer.
    ktx_uint32_t elementSize;
    ktx_uint32_t numDimensions;
#if defined(_DEBUG)
    VkBufferImageCopy* regionsArrayEnd;
#endif
};

/**
 * @internal
 * @~English
 * @brief Callback for optimally tiled textures with no source row padding.
 *
 * Images must be preloaded into the staging buffer. Each iteration, i.e.
 * the value of @p faceLodSize must be for a complete mip level, regardless of
 * texture type. This should be used only with @c ktx_Texture_IterateLevels.
 *
 * Sets up a region to copy the data from the staging buffer to the final
 * image.
 *
 * @note @p pixels is not used.
 *
 * @copydetails PFNKTXITERCB
 */
static KTX_error_code
optimalTilingCallback(int miplevel,
                      int face,
                      int width,
                      int height,
                      int depth,
                      ktx_uint64_t faceLodSize,
                      void* pixels,
                      void* userdata)
{
    user_cbdata_optimal* ud = (user_cbdata_optimal*)userdata;

    // Set up copy to destination region in final image
#if defined(_DEBUG)
    assert(ud->region < ud->regionsArrayEnd);
#endif
    ud->region->bufferOffset = ud->offset;
    ud->offset += faceLodSize;
    // These 2 are expressed in texels.
    ud->region->bufferRowLength = 0;
    ud->region->bufferImageHeight = 0;
    ud->region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ud->region->imageSubresource.mipLevel = miplevel;
    ud->region->imageSubresource.baseArrayLayer = face;
    ud->region->imageSubresource.layerCount = ud->numLayers * ud->numFaces;
    ud->region->imageOffset.x = 0;
    ud->region->imageOffset.y = 0;
    ud->region->imageOffset.z = 0;
    ud->region->imageExtent.width = width;
    ud->region->imageExtent.height = height;
    ud->region->imageExtent.depth = depth;

    ud->region += 1;

    return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Callback for optimally tiled textures with possible source row
 *        padding.
 *
 * Copies data to the staging buffer removing row padding, if necessary.
 * Increments the offset for destination of the next copy increasing it to an
 * appropriate common multiple of the element size and 4 to comply with Vulkan
 * valid usage. Finally sets up a region to copy the face/lod from the staging
 * buffer to the final image.
 *
 * This longer method is needed because row padding is different between
 * KTX (pad to 4) and Vulkan (none). Also region->bufferOffset, i.e. the start
 * of each image, has to be a multiple of 4 and also a multiple of the
 * element size.
 *
 * This should be used with @c ktx_Texture_IterateFaceLevels or
 * @c ktx_Texture_IterateLoadLevelFaces. Face-level iteration has been
 * selected to minimize the buffering needed between reading the file and
 * copying the data into the staging buffer. Obviously when
 * @c ktx_Texture_IterateFaceLevels is being used, this is a moot point.
 *
 * @copydetails PFNKTXITERCB
 */
KTX_error_code
optimalTilingPadCallback(int miplevel,
                         int face,
                         int width,
                         int height,
                         int depth,
                         ktx_uint64_t faceLodSize,
                         void* pixels,
                         void* userdata)
{
    user_cbdata_optimal* ud = (user_cbdata_optimal*)userdata;
    ktx_uint32_t rowPitch = width * ud->elementSize;

    // Set bufferOffset in destination region in final image
#if defined(_DEBUG)
    assert(ud->region < ud->regionsArrayEnd);
#endif
    ud->region->bufferOffset = ud->offset;

    // Copy data into staging buffer
    if (_KTX_PAD_UNPACK_ALIGN_LEN(rowPitch) == 0) {
        // No padding. Can copy in bulk.
        memcpy(ud->dest + ud->offset, pixels, faceLodSize);
        ud->offset += faceLodSize;
    } else {
        // Must remove padding. Copy a row at a time.
        ktx_uint32_t image, imageIterations;
        ktx_int32_t row;
        ktx_uint32_t paddedRowPitch;

        if (ud->numDimensions == 3)
            imageIterations = depth;
        else if (ud->numLayers > 1)
            imageIterations = ud->numLayers * ud->numFaces;
        else
            imageIterations = 1;
        rowPitch = paddedRowPitch = width * ud->elementSize;
        paddedRowPitch = _KTX_PAD_UNPACK_ALIGN(paddedRowPitch);
        for (image = 0; image < imageIterations; image++) {
            for (row = 0; row < height; row++) {
                memcpy(ud->dest + ud->offset, pixels, rowPitch);
                ud->offset += rowPitch;
                pixels = (ktx_uint8_t*)pixels + paddedRowPitch;
            }
        }
    }

    // Round to needed multiples for next region, if necessary.
    if (ud->offset % ud->elementSize != 0 || ud->offset % 4 != 0) {
        // Only elementSizes of 1,2 and 3 will bring us here.
        assert(ud->elementSize < 4 && ud->elementSize > 0);
        ktx_uint32_t lcm = lcm4(ud->elementSize);
        ud->offset = _KTX_PADN(lcm, ud->offset);
    }
    // These 2 are expressed in texels; not suitable for dealing with padding.
    ud->region->bufferRowLength = 0;
    ud->region->bufferImageHeight = 0;
    ud->region->imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ud->region->imageSubresource.mipLevel = miplevel;
    ud->region->imageSubresource.baseArrayLayer = face;
    ud->region->imageSubresource.layerCount = ud->numLayers * ud->numFaces;
    ud->region->imageOffset.x = 0;
    ud->region->imageOffset.y = 0;
    ud->region->imageOffset.z = 0;
    ud->region->imageExtent.width = width;
    ud->region->imageExtent.height = height;
    ud->region->imageExtent.depth = depth;

    ud->region += 1;

    return KTX_SUCCESS;
}

/** @internal
 * @brief Generate mipmaps from base using @c VkCmdBlitImage.
 *
 * Mipmaps are generated by blitting level n from level n-1 as it should
 * be faster than the alternative of blitting all levels from the base level.
 *
 * After generation, the image is transitioned to the layout indicated by
 * @c vkTexture->imageLayout.
 *
 * @param[in] vkTexture     pointer to an object with information about the
 *                          image for which to generate mipmaps.
 * @param[in] vdi           pointer to an object with information about the
 *                          Vulkan device and command buffer to use.
 * @param[in] blitFilter    the type of filter to use in the @c VkCmdBlitImage.
 * @param[in] initialLayout the layout of the image on entry to the function.
 */
static void
generateMipmaps(VkCommandBuffer cmd_buf,
                const xray::rendering::VulkanTextureInfo& vkTexture,
                VkImage image,
                const VkFilter blitFilter,
                const VkImageLayout initialLayout)
{
    VkImageSubresourceRange subresourceRange{};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = vkTexture.layerCount;

    // Transition base level to SRC_OPTIMAL for blitting.
    xray::rendering::set_image_layout(
        cmd_buf, image, initialLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

    // Generate the mip chain
    // ----------------------
    // Blit level n from level n-1.
    for (uint32_t i = 1; i < vkTexture.levelCount; i++) {
        VkImageBlit imageBlit{};

        // Source
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = vkTexture.layerCount;
        imageBlit.srcSubresource.mipLevel = i - 1;
        imageBlit.srcOffsets[1].x = std::max<int32_t>(1, vkTexture.width >> (i - 1));
        imageBlit.srcOffsets[1].y = std::max<int32_t>(1, vkTexture.height >> (i - 1));
        imageBlit.srcOffsets[1].z = std::max<int32_t>(1, vkTexture.depth >> (i - 1));
        ;

        // Destination
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel = i;
        imageBlit.dstOffsets[1].x = std::max<int32_t>(1, vkTexture.width >> i);
        imageBlit.dstOffsets[1].y = std::max<int32_t>(1, vkTexture.height >> i);
        imageBlit.dstOffsets[1].z = std::max<int32_t>(1, vkTexture.depth >> i);

        const VkImageSubresourceRange mipSubRange{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = i,
            .levelCount = 1,
            .layerCount = vkTexture.layerCount,
        };

        // Transiton current mip level to transfer dest
        xray::rendering::set_image_layout(
            cmd_buf, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipSubRange);

        // Blit from previous level
        vkCmdBlitImage(cmd_buf,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &imageBlit,
                       blitFilter);

        // Transiton current mip level to transfer source for read in
        // next iteration.
        xray::rendering::set_image_layout(
            cmd_buf, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mipSubRange);
    }

    // After the loop, all mip layers are in TRANSFER_SRC layout.
    // Transition all to final layout.
    subresourceRange.levelCount = vkTexture.levelCount;
    xray::rendering::set_image_layout(
        cmd_buf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkTexture.imageLayout, subresourceRange);
}

} // namespace ktx_internal_details

tl::expected<xray::rendering::VulkanImage, xray::rendering::VulkanError>
xray::rendering::VulkanImage::from_memory(VulkanRenderer& renderer, const VulkanImageCreateInfo& create_info)
{
    struct ImageTypeWithImageView
    {
        VkImageType image;
        VkImageViewType view;
    };

    if (!(create_info.memory_flags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))) {
        assert(create_info.layers == static_cast<uint32_t>(create_info.pixels.size()));
    }

    const VkImageViewType image_view_type = [](const VulkanImageCreateInfo& ci) {
        switch (ci.type) {
            case VK_IMAGE_TYPE_1D:
                return ci.layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;

            case VK_IMAGE_TYPE_2D:
            default: {
                if (ci.cubemap) {
                    return ci.layers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
                } else {
                    return ci.layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
                }
            } break;

            case VK_IMAGE_TYPE_3D: {
                assert(ci.layers == 1);
                return VK_IMAGE_VIEW_TYPE_3D;
            }
        }
    }(create_info);

    const VkImageCreateInfo vk_create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = create_info.type,
        .format = create_info.format,
        .extent = VkExtent3D{ .width = create_info.width, .height = create_info.height, .depth = create_info.depth },
        .mipLevels = 1,
        .arrayLayers = create_info.layers,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = create_info.usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    xrUniqueVkImage image{ nullptr, VkResourceDeleter_VkImage{ renderer.device() } };
    const VkResult img_create_res =
        WRAP_VULKAN_FUNC(vkCreateImage, renderer.device(), &vk_create_info, nullptr, base::raw_ptr_ptr(image));
    XR_VK_CHECK_RESULT(img_create_res);

    const VkImageMemoryRequirementsInfo2 mem_req_info{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                                                       .pNext = nullptr,
                                                       .image = base::raw_ptr(image) };
    VkMemoryRequirements2 mem_requirements{ .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, .pNext = nullptr };
    vkGetImageMemoryRequirements2(renderer.device(), &mem_req_info, &mem_requirements);

    const VkMemoryAllocateInfo mem_alloc_info{ .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                               .pNext = nullptr,
                                               .allocationSize = mem_requirements.memoryRequirements.size,
                                               .memoryTypeIndex = renderer.find_allocation_memory_type(
                                                   mem_requirements.memoryRequirements.memoryTypeBits,
                                                   create_info.memory_flags) };

    xrUniqueVkDeviceMemory image_memory{ nullptr, VkResourceDeleter_VkDeviceMemory{ renderer.device() } };
    const VkResult mem_alloc_res = WRAP_VULKAN_FUNC(
        vkAllocateMemory, renderer.device(), &mem_alloc_info, nullptr, base::raw_ptr_ptr(image_memory));
    XR_VK_CHECK_RESULT(mem_alloc_res);

    const VkResult bind_res =
        WRAP_VULKAN_FUNC(vkBindImageMemory, renderer.device(), base::raw_ptr(image), base::raw_ptr(image_memory), 0);
    XR_VK_CHECK_RESULT(bind_res);

    if (create_info.tag_name) {
        renderer.dbg_set_object_name(base::raw_ptr(image), create_info.tag_name);
    }

    if (!create_info.pixels.empty()) {
        auto staging_buffer = renderer.create_staging_buffer(create_info.wpkg, mem_alloc_info.allocationSize);
        XR_VK_PROPAGATE_ERROR(staging_buffer);

        const auto copied_to_buffer =
            UniqueMemoryMapping::create_ex(renderer.device(), staging_buffer->mem, 0, 0)
                .map([&create_info](UniqueMemoryMapping mapping) {
                    for (size_t i = 0, count = create_info.pixels.size(); i < count; ++i) {
                        memcpy((void*)((uint8_t*)mapping._mapped_memory + i * create_info.width * create_info.height),
                               static_cast<const void*>(create_info.pixels[i].data()),
                               create_info.pixels[i].size_bytes());
                    }
                    return std::true_type{};
                });
        XR_VK_PROPAGATE_ERROR(copied_to_buffer);

        VkCommandBuffer cmd_buf = renderer.get_cmd_buf_for_work_package(create_info.wpkg);

        //
        // set image layout to transfer_dst
        set_image_layout(cmd_buf,
                         base::raw_ptr(image),
                         VK_IMAGE_LAYOUT_UNDEFINED,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         VkImageSubresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                  .baseMipLevel = 0,
                                                  .levelCount = 1,
                                                  .baseArrayLayer = 0,
                                                  .layerCount = create_info.layers });

        const VkBufferImageCopy buffer_image_cpy{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = VkImageSubresourceLayers{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                          .mipLevel = 0,
                                                          .baseArrayLayer = 0,
                                                          .layerCount = create_info.layers },
            .imageOffset = {},
            .imageExtent = { create_info.width, create_info.height, create_info.depth }
        };

        vkCmdCopyBufferToImage(cmd_buf,
                               staging_buffer->buf,
                               base::raw_ptr(image),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &buffer_image_cpy);

        //
        // set layout to shader readonly optimal
        if (create_info.usage_flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
            set_image_layout(cmd_buf,
                             base::raw_ptr(image),
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             VkImageSubresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                      .baseMipLevel = 0,
                                                      .levelCount = 1,
                                                      .baseArrayLayer = 0,
                                                      .layerCount = create_info.layers });
        } else {
            assert(false && "Needs to be implemented");
        }
    }

    return VulkanImage{ xrUniqueImageWithMemory{ renderer.device(),
                                                 base::unique_pointer_release(image),
                                                 base::unique_pointer_release(image_memory) },
                        VulkanTextureInfo{ create_info.width,
                                           create_info.height,
                                           create_info.depth,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                           create_info.format,
                                           1,
                                           create_info.layers,
                                           image_view_type } };
}

tl::expected<xray::rendering::VulkanImage, xray::rendering::VulkanError>
xray::rendering::VulkanImage::from_file(VulkanRenderer& renderer, const VulkanImageLoadInfo& load_info)
{
    std::error_code err_code{};
    const mio::mmap_source texf{ mio::make_mmap_source(load_info.path.generic_string(), err_code) };
    if (err_code) {
        XR_LOG_ERR("Failed to open texture file {}, error {}", load_info.path.generic_string(), err_code.message());
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
    }

    ktxTexture2* loaded_ktx{};
    const KTX_error_code tex_load_result =
        ktxTexture2_CreateFromMemory(reinterpret_cast<const ktx_uint8_t*>(texf.data()),
                                     texf.size(),
                                     KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                     &loaded_ktx);

    if (tex_load_result != KTX_SUCCESS) {
        XR_LOG_ERR("Failed to load KTX2 texture, {}", static_cast<int32_t>(tex_load_result));
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
    }

    assert(loaded_ktx->classId == ktxTexture2_c && "Only KTX2 textures are supported!");
    assert(loaded_ktx->numFaces == 6 ? loaded_ktx->numDimensions == 2 : VK_TRUE);

    VkImageCreateFlags img_create_flags{};
    uint32_t numImageLayers = loaded_ktx->numLayers;
    if (loaded_ktx->isCubemap) {
        numImageLayers *= 6;
        img_create_flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VkImageType imageType{ VK_IMAGE_TYPE_MAX_ENUM };
    VkImageViewType viewType{ VK_IMAGE_VIEW_TYPE_MAX_ENUM };

    assert(loaded_ktx->numDimensions >= 1 && loaded_ktx->numDimensions <= 3);
    switch (loaded_ktx->numDimensions) {
        case 1:
            imageType = VK_IMAGE_TYPE_1D;
            viewType = loaded_ktx->isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
            break;
        case 2:
        default: // To keep compilers happy.
            imageType = VK_IMAGE_TYPE_2D;
            if (loaded_ktx->isCubemap)
                viewType = loaded_ktx->isArray ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
            else
                viewType = loaded_ktx->isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
            break;
        case 3:
            imageType = VK_IMAGE_TYPE_3D;
            /* 3D array textures not supported in Vulkan. Attempts to create or
             * load them should have been trapped long before this.
             */
            assert(!loaded_ktx->isArray);
            viewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
    }

    const VkFormat vkFormat = ktx_internal_details::ktxTexture_GetVkFormat(ktxTexture(loaded_ktx));
    if (vkFormat == VK_FORMAT_UNDEFINED) {
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_FORMAT_NOT_SUPPORTED);
    }

    VkImageUsageFlags img_usage_flags{ load_info.usage_flags };

    /* Get device properties for the requested image format */
    if (load_info.tiling == VK_IMAGE_TILING_OPTIMAL) {
        // Ensure we can copy from staging buffer to image.
        img_usage_flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (loaded_ktx->generateMipmaps) {
        // Ensure we can blit between levels.
        img_usage_flags |= (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    }

    VkImageFormatProperties imageFormatProperties;
    const VkResult image_supported_chk_res = WRAP_VULKAN_FUNC(vkGetPhysicalDeviceImageFormatProperties,
                                                              renderer.physical().device,
                                                              vkFormat,
                                                              imageType,
                                                              load_info.tiling,
                                                              img_usage_flags,
                                                              img_create_flags,
                                                              &imageFormatProperties);

    if (image_supported_chk_res == VK_ERROR_FORMAT_NOT_SUPPORTED) {
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_FORMAT_NOT_SUPPORTED);
    }
    if (loaded_ktx->numLayers > imageFormatProperties.maxArrayLayers) {
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_FORMAT_NOT_SUPPORTED);
    }

    VkFilter blit_filter = VK_FILTER_LINEAR;
    uint32_t numImageLevels{};

    if (loaded_ktx->generateMipmaps) {
        VkFormatProperties formatProperties;
        VkFormatFeatureFlags formatFeatureFlags;
        VkFormatFeatureFlags neededFeatures = VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT;

        WRAP_VULKAN_FUNC(vkGetPhysicalDeviceFormatProperties, renderer.physical().device, vkFormat, &formatProperties);
        if (load_info.tiling == VK_IMAGE_TILING_OPTIMAL)
            formatFeatureFlags = formatProperties.optimalTilingFeatures;
        else
            formatFeatureFlags = formatProperties.linearTilingFeatures;

        if ((formatFeatureFlags & neededFeatures) != neededFeatures)
            return XR_MAKE_VULKAN_ERROR(VK_ERROR_FEATURE_NOT_PRESENT);

        if (formatFeatureFlags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)
            blit_filter = VK_FILTER_LINEAR;
        else
            blit_filter = VK_FILTER_NEAREST; // XXX INVALID_OP?

        const uint32_t max_dim =
            std::max(std::max(loaded_ktx->baseWidth, loaded_ktx->baseHeight), loaded_ktx->baseDepth);
        numImageLevels = (uint32_t)floor(log2(max_dim)) + 1;
    } else {
        numImageLevels = loaded_ktx->numLevels;
    }

    if (numImageLevels > imageFormatProperties.maxMipLevels) {
        return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
    }

    const bool canUseFasterPath{ true }; // always true for KTX2 and no need to bother since
                                         // we only support KTX2
    ktx_uint32_t elementSize = ktxTexture_GetElementSize(ktxTexture(loaded_ktx));
    const uint64_t textureSize = ktxTexture_GetDataSizeUncompressed(ktxTexture(loaded_ktx));

    const VulkanTextureInfo tex_info{
        .width = loaded_ktx->baseWidth,
        .height = loaded_ktx->baseHeight,
        .depth = loaded_ktx->baseDepth,
        .imageLayout = load_info.final_layout,
        .imageFormat = vkFormat,
        .levelCount = numImageLevels,
        .layerCount = numImageLayers,
        .viewType = viewType,
    };

    if (load_info.tiling == VK_IMAGE_TILING_OPTIMAL) {
        //
        // Create a host-visible staging buffer that contains the raw image data
        const uint32_t numCopyRegions{ loaded_ktx->numLevels };

        auto staging_buffer{ renderer.create_staging_buffer(load_info.wpkg, textureSize) };
        if (!staging_buffer)
            return tl::unexpected{ staging_buffer.error() };

        itlib::small_vector<VkBufferImageCopy> copy_regions{ static_cast<size_t>(numCopyRegions), VkBufferImageCopy{} };

        UniqueMemoryMapping::create_ex(renderer.device(), staging_buffer->mem, 0, VK_WHOLE_SIZE)
            .map([&](UniqueMemoryMapping bufmap) {
                ktx_internal_details::user_cbdata_optimal cbData
                {
                    .region = copy_regions.data(), .offset = 0, .numFaces = loaded_ktx->numFaces,
                    .numLayers = loaded_ktx->numLayers, .dest = static_cast<uint8_t*>(bufmap._mapped_memory),
                    .elementSize = elementSize, .numDimensions = loaded_ktx->numDimensions,
#if defined(_DEBUG)
                    .regionsArrayEnd = copy_regions.end(),
#endif
                };

                if (loaded_ktx->pData) {
                    // Image data has already been loaded. Copy to staging
                    // buffer.
                    memcpy(bufmap._mapped_memory, loaded_ktx->pData, loaded_ktx->dataSize);
                } else {
                    /* Load the image data directly into the staging buffer. */
                    const ktxResult kResult = ktxTexture_LoadImageData(ktxTexture(loaded_ktx),
                                                                       static_cast<ktx_uint8_t*>(bufmap._mapped_memory),
                                                                       (ktx_size_t)textureSize);
                    if (kResult != KTX_SUCCESS) {
                        // TODO: error handling
                    }
                }

                // Iterate over mip levels to set up the copy regions.
                const ktxResult kResult = ktxTexture_IterateLevels(
                    ktxTexture(loaded_ktx), ktx_internal_details::optimalTilingCallback, &cbData);
                // XXX Check for possible errors.
            });

        // Create optimal tiled target image
        const VkImageCreateInfo imageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = img_create_flags,
            .imageType = imageType,
            .format = vkFormat,
            .extent =
                VkExtent3D{
                    .width = tex_info.width,
                    .height = tex_info.height,
                    .depth = tex_info.depth,
                },
            // numImageLevels ensures enough levels for generateMipmaps.
            .mipLevels = numImageLevels,
            .arrayLayers = numImageLayers,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = img_usage_flags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VkImage img_handle{};
        const VkResult create_result =
            WRAP_VULKAN_FUNC(vkCreateImage, renderer.device(), &imageCreateInfo, nullptr, &img_handle);
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
            .memoryTypeIndex = renderer.find_allocation_memory_type(mem_req.memoryRequirements.memoryTypeBits,
                                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
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

        const VkBindImageMemoryInfo bind_mem_info{
            .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
            .pNext = nullptr,
            .image = raw_ptr(image),
            .memory = raw_ptr(image_memory),
            .memoryOffset = 0,
        };

        const VkResult bind_img_mem_result = WRAP_VULKAN_FUNC(vkBindImageMemory2, renderer.device(), 1, &bind_mem_info);
        if (bind_img_mem_result != VK_SUCCESS) {
            return XR_MAKE_VULKAN_ERROR(bind_img_mem_result);
        }

        const VkImageSubresourceRange subresource_range{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = loaded_ktx->numLevels,
            .baseArrayLayer = 0,
            .layerCount = numImageLayers,
        };

        VkCommandBuffer cmdbuf = renderer.get_cmd_buf_for_work_package(load_info.wpkg);

        // Image barrier to transition, possibly only the base level, image
        // layout to TRANSFER_DST_OPTIMAL so it can be used as the copy
        // destination.
        set_image_layout(
            cmdbuf, raw_ptr(image), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresource_range);

        vkCmdCopyBufferToImage(cmdbuf,
                               staging_buffer->buf,
                               raw_ptr(image),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<uint32_t>(copy_regions.size()),
                               copy_regions.data());

        if (loaded_ktx->generateMipmaps) {
            //
            // this will handle the transition to the final image layout too
            ktx_internal_details::generateMipmaps(
                cmdbuf, tex_info, raw_ptr(image), blit_filter, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        } else {
            //
            // Transition image layout to finalLayout after all mip levels
            // have been copied.
            // In this case numImageLevels == This->numLevels
            // subresourceRange.levelCount = numImageLevels;

            set_image_layout(cmdbuf,
                             raw_ptr(image),
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             load_info.final_layout,
                             subresource_range);
        }

        return VulkanImage{
            xrUniqueImageWithMemory{
                renderer.device(),
                xray::base::unique_pointer_release(image),
                xray::base::unique_pointer_release(image_memory),
            },
            tex_info,
        };
    } else {
        //
        // TODO: add linear tiling support if needed ... Right now its not necessary.
        assert(false && "Only optimal tiling images are supported for now");
    }

    return XR_MAKE_VULKAN_ERROR(VK_ERROR_NOT_PERMITTED_KHR);
}

VkImageMemoryBarrier2
xray::rendering::make_image_layout_memory_barrier(const VkImage image,
                                                  const VkImageLayout previous_layout,
                                                  const VkImageLayout new_layout,
                                                  const VkImageSubresourceRange subresource_range)
{
    VkImageMemoryBarrier2 mem_barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, // Put barrier on top of pipeline.
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, // Put barrier on top of pipeline.
        .dstAccessMask = 0,
        .oldLayout = previous_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresource_range,
    };

    //
    // Source layouts (old)
    // The source access mask controls actions to be finished on the old
    // layout before it will be transitioned to the new layout.
    switch (previous_layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter).
            // Only valid as initial layout. No flags required.
            // Do nothing.
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized.
            // Only valid as initial layout for linear images; preserves memory
            // contents. Make sure host writes have finished.
            mem_barrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment.
            // Make sure writes to the color buffer have finished
            mem_barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment.
            // Make sure any writes to the depth/stencil buffer have finished.
            mem_barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source.
            // Make sure any reads from the image have finished
            mem_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination.
            // Make sure any writes to the image have finished.
            mem_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader.
            // Make sure any shader reads from the image have finished
            mem_barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;

        default:
            // Value not used by callers, so not supported.
            XR_LOG_ERR("Unsupported value for previous layoutr of image: {}",
                       vk::to_string(static_cast<vk::ImageLayout>(previous_layout)));
            assert(false);
            break;
    }

    // Target layouts (new)
    // The destination access mask controls the dependency for the new image
    // layout.
    switch (new_layout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination.
            // Make sure any writes to the image have finished.
            mem_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source.
            // Make sure any reads from and writes to the image have finished.
            mem_barrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT;
            mem_barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment.
            // Make sure any writes to the color buffer have finished.
            mem_barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
            mem_barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment.
            // Make sure any writes to depth/stencil buffer have finished.
            mem_barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment).
            // Make sure any writes to the image have finished.
            if (mem_barrier.srcAccessMask == 0) {
                mem_barrier.srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
            }
            mem_barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
            break;
        default:
            /* Value not used by callers, so not supported. */
            XR_LOG_ERR("Unsupported value for new layout of image: {}",
                       vk::to_string(static_cast<vk::ImageLayout>(new_layout)));
            assert(false);
            break;
    }

    return mem_barrier;
}

void
xray::rendering::set_image_layout(VkCommandBuffer cmd_buffer,
                                  VkImage image,
                                  const VkImageLayout initial_layout,
                                  const VkImageLayout final_layout,
                                  const VkImageSubresourceRange& subresource_range)
{
    const VkImageMemoryBarrier2 img_mem_barrier{ make_image_layout_memory_barrier(
        image, initial_layout, final_layout, subresource_range) };

    const VkDependencyInfo dep_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &img_mem_barrier,
    };

    vkCmdPipelineBarrier2(cmd_buffer, &dep_info);
}
