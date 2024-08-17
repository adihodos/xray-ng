#include "triangle.hpp"

#include <string_view>
#include <array>
#include <random>
#include <Lz/Lz.hpp>

#include <itlib/small_vector.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/math/scalar2x3_math.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_base.hpp"
#include "xray/rendering/colors/hsv_color.hpp"
#include "xray/rendering/colors/color_cast_rgb_hsv.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/base/veneers/sequence_container_veneer.hpp"
#include "xray/base/app_config.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;
using namespace xray::math;

class random_engine
{
  public:
    random_engine() {}

    void set_float_range(const float minval, const float maxval) noexcept
    {
        _f32distribution = std::uniform_real_distribution<float>{ minval, maxval };
    }

    void set_integer_range(const int32_t minval, const int32_t maxval) noexcept
    {
        _i32distribution = std::uniform_int_distribution<int32_t>{ minval, maxval };
    }

    float next_float() noexcept { return _f32distribution(_engine); }
    int32_t next_int() noexcept { return _i32distribution(_engine); }

  private:
    std::random_device _device{};
    std::mt19937 _engine{ _device() };
    std::uniform_real_distribution<float> _f32distribution{ 0.0f, 1.0f };
    std::uniform_int_distribution<int32_t> _i32distribution{ 0, 1 };
};

dvk::TriangleDemo::TriangleDemo(PrivateConstructionToken,
                                const app::init_context_t& init_context,
                                xray::rendering::VulkanBuffer g_ubo,
                                xray::rendering::VulkanBuffer g_vertexbuffer,
                                xray::rendering::VulkanBuffer g_indexbuffer,
                                xray::rendering::VulkanBuffer g_instancebuffer,
                                xray::rendering::GraphicsPipeline pipeline,
                                xray::rendering::VulkanImage pixel_buffer,
                                xray::rendering::xrUniqueVkImageView imageview,
                                xray::rendering::xrUniqueVkSampler sampler,
                                const uint32_t vbuffer,
                                const uint32_t ibuffer,
                                std::vector<VkDescriptorSet> desc_sets)
    : app::DemoBase{ init_context }
    , _g_ubo{ std::move(g_ubo) }
    , _g_vertexbuffer{ std::move(g_vertexbuffer) }
    , _g_indexbuffer{ std::move(g_indexbuffer) }
    , _g_instancebuffer{ std::move(g_instancebuffer) }
    , _pipeline{ std::move(pipeline) }
    , _pixel_buffer{ std::move(pixel_buffer) }
    , _imageview{ std::move(imageview) }
    , _sampler{ std::move(sampler) }
    , _vbuffer_handle{ vbuffer }
    , _ibuffer_handle{ ibuffer }
    , _desc_sets{ std::move(desc_sets) }
{
    _timer.start();
}

struct FrameGlobalData
{
    mat4f world_view_proj;
    mat4f projection;
    mat4f inv_projection;
    mat4f view;
    mat4f ortho;
    vec3f eye_pos;
    uint frame;
};

struct InstanceRenderInfo
{
    mat4f model;
    uint vtx_buff;
    uint idx_buff;
    uint mtl_coll;
    uint mtl;
};

struct VertexPTC
{
    vec2f pos;
    vec2f uv;
    vec4f color;
};

template<typename T, size_t N>
inline constexpr std::span<const uint8_t>
to_bytes_span(const T (&array_ref)[N]) noexcept
{
    return std::span{ reinterpret_cast<const uint8_t*>(&array_ref[0]), sizeof(array_ref) };
}

xray::base::unique_pointer<app::DemoBase>
dvk::TriangleDemo::create(const app::init_context_t& init_ctx)
{
    const RenderBufferingSetup rbs{ init_ctx.renderer->buffering_setup() };

    auto pkgs{ init_ctx.renderer->create_work_package() };
    if (!pkgs)
        return nullptr;

    const VulkanBufferCreateInfo bci{
        .name_tag = "UBO - FrameGlobalData",
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .bytes = sizeof(FrameGlobalData),
        .frames = rbs.buffers,
        .initial_data = {},
    };

    auto g_ubo{ VulkanBuffer::create(*init_ctx.renderer, bci) };
    if (!g_ubo)
        return nullptr;

    constexpr const VertexPTC tri_vertices[] = {
        { vec2f{ -1.0f, -1.0f }, vec2f{ 0.0f, 1.0f }, vec4f{ 1.0f, 0.0f, 0.0f, 1.0f } },
        { vec2f{ 1.0f, -1.0f }, vec2f{ 1.0f, 1.0f }, vec4f{ 0.0f, 1.0f, 0.0f, 1.0f } },
        { vec2f{ 0.0f, 1.0f }, vec2f{ 0.5f, 0.0f }, vec4f{ 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    const VulkanBufferCreateInfo vbinfo{
        .name_tag = "Triangle vertices",
        .work_package = pkgs->pkg,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bytes = sizeof(tri_vertices),
        .frames = 1,
        .initial_data = { to_bytes_span(tri_vertices) },
    };

    auto g_vertexbuffer{ VulkanBuffer::create(*init_ctx.renderer, vbinfo) };
    if (!g_vertexbuffer)
        return nullptr;

    constexpr const uint32_t tri_indices[] = { 0, 1, 2 };
    const VulkanBufferCreateInfo ibinfo{
        .name_tag = "Triangle index buffer",
        .work_package = pkgs->pkg,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .bytes = sizeof(tri_indices),
        .frames = 1,
        .initial_data = { to_bytes_span(tri_indices) },
    };

    auto g_indexbuffer{ VulkanBuffer::create(*init_ctx.renderer, ibinfo) };
    if (!g_indexbuffer)
        return nullptr;

    const VulkanBufferCreateInfo inst_buf_info{
        .name_tag = "Instances buffer",
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .bytes = 1024 * sizeof(InstanceRenderInfo),
        .frames = rbs.buffers,
        .initial_data = {},
    };

    auto g_instance_buffer{ VulkanBuffer::create(*init_ctx.renderer, inst_buf_info) };
    if (!g_instance_buffer)
        return nullptr;

    const char* const tex_file{ "uv_grids/ash_uvgrid01.ktx2" };

    auto pixel_buffer{
        VulkanImage::from_file(*init_ctx.renderer,
                               VulkanImageLoadInfo{
                                   .tag_name = tex_file,
                                   .wpkg = pkgs->pkg,
                                   .path = init_ctx.cfg->texture_path(tex_file),
                                   .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                                   .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                   .tiling = VK_IMAGE_TILING_OPTIMAL,
                               }),
    };

    if (!pixel_buffer) {
        return nullptr;
    }

    VulkanImage image{ std::move(*pixel_buffer) };

    init_ctx.renderer->submit_work_package(pkgs->pkg);

    tl::expected<GraphicsPipeline, VulkanError> pipeline{
        GraphicsPipelineBuilder{}
            .add_shader(ShaderStage::Vertex, init_ctx.cfg->shader_root() / "triangle/tri.vert")
            .add_shader(ShaderStage::Fragment, init_ctx.cfg->shader_root() / "triangle/tri.frag")
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .create(*init_ctx.renderer,
                    GraphicsPipelineCreateData{
                        .uniform_descriptors = 16,
                        .storage_buffer_descriptors = 256,
                        .combined_image_sampler_descriptors = 256,
                        .image_descriptors = 1,
                    }),
    };

    if (!pipeline)
        return nullptr;

    xrUniqueVkSampler sampler{
        [&]() {
            const VkSamplerCreateInfo create_info{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .mipLodBias = 0.0f,
                .anisotropyEnable = false,
                .maxAnisotropy = 1.0f,
                .compareEnable = false,
                .compareOp = VK_COMPARE_OP_NEVER,
                .minLod = 0.0f,
                .maxLod = VK_LOD_CLAMP_NONE,
                .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalizedCoordinates = false,
            };

            VkSampler sampler{};
            WRAP_VULKAN_FUNC(vkCreateSampler, init_ctx.renderer->device(), &create_info, nullptr, &sampler);
            return sampler;
        }(),
        VkResourceDeleter_VkSampler{ init_ctx.renderer->device() },
    };

    xrUniqueVkImageView img_view{
        [&]() {
            const VkImageViewCreateInfo img_view_create{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = image.image(),
                .viewType = image._info.viewType,
                .format = image._info.imageFormat,
                .components =
                    VkComponentMapping{
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                    },
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = image._info.levelCount,
                        .baseArrayLayer = 0,
                        .layerCount = image._info.layerCount,
                    },
            };
            VkImageView img_view{};
            WRAP_VULKAN_FUNC(vkCreateImageView, init_ctx.renderer->device(), &img_view_create, nullptr, &img_view);
            return img_view;
        }(),
        VkResourceDeleter_VkImageView{ init_ctx.renderer->device() }
    };

    auto descriptor_sets_layouts = pipeline->descriptor_sets_layouts();
    const VkDescriptorSetAllocateInfo dset_alloc{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = init_ctx.renderer->descriptor_pool(),
        .descriptorSetCount = static_cast<uint32_t>(descriptor_sets_layouts.size()),
        .pSetLayouts = descriptor_sets_layouts.data(),
    };

    vector<VkDescriptorSet> desc_sets;
    desc_sets.resize(descriptor_sets_layouts.size(), nullptr);
    const VkResult sets_alloc_res =
        WRAP_VULKAN_FUNC(vkAllocateDescriptorSets, init_ctx.renderer->device(), &dset_alloc, desc_sets.data());

    if (sets_alloc_res != VK_SUCCESS) {
        return nullptr;
    }

    const VkDescriptorImageInfo desc_img_info{
        .sampler = raw_ptr(sampler),
        .imageView = raw_ptr(img_view),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    vector<VkWriteDescriptorSet> write_dsets;
    vector<VkDescriptorBufferInfo> descriptor_buffer_info;
    descriptor_buffer_info.reserve(rbs.buffers * 4); // ubo + instance data

    struct WriteBufferDescriptorsHelper
    {
        VkDescriptorSet dest;
        reference_wrapper<VulkanBuffer> buffer;
        VkDescriptorType dtype;
        uint32_t count;
        uint16_t dst_arr_elem;
        uint16_t desc_gbuff_offset;
    } write_buf_desc_helpers[] = {
        WriteBufferDescriptorsHelper{
            .dest = desc_sets[0],
            .buffer = { *g_ubo },
            .dtype = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .count = rbs.buffers,
            .dst_arr_elem = 0,
            .desc_gbuff_offset = 0,
        },
        WriteBufferDescriptorsHelper{
            .dest = desc_sets[1],
            .buffer = { *g_instance_buffer },
            .dtype = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .count = rbs.buffers,
            .dst_arr_elem = 0,
            .desc_gbuff_offset = 0,
        },
        WriteBufferDescriptorsHelper{
            .dest = desc_sets[1],
            .buffer = { *g_vertexbuffer },
            .dtype = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .count = 1,
            .dst_arr_elem = 0,
            .desc_gbuff_offset = 0,
        },
        WriteBufferDescriptorsHelper{
            .dest = desc_sets[1],
            .buffer = { *g_indexbuffer },
            .dtype = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .count = 1,
            .dst_arr_elem = 0,
            .desc_gbuff_offset = 0,
        },
    };

    array<uint32_t, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC> array_offsets_by_descriptor_type{};

    for (WriteBufferDescriptorsHelper& write_desc_helper : write_buf_desc_helpers) {
        // if (!last_desc_type || *last_desc_type != wbdh.dtype) {
        //     offset = 0;
        // }

        write_desc_helper.desc_gbuff_offset = static_cast<uint16_t>(descriptor_buffer_info.size());
        write_desc_helper.dst_arr_elem = array_offsets_by_descriptor_type[write_desc_helper.dtype];

        for (uint32_t i = 0; i < write_desc_helper.count; ++i) {
            descriptor_buffer_info.push_back(VkDescriptorBufferInfo{
                .buffer = write_desc_helper.buffer.get().buffer_handle(),
                .offset = write_desc_helper.buffer.get().aligned_size * i,
                .range = write_desc_helper.buffer.get().aligned_size,
            });
        }

        array_offsets_by_descriptor_type[write_desc_helper.dtype] += write_desc_helper.count;
    }

    for (const WriteBufferDescriptorsHelper& wbdh : write_buf_desc_helpers) {
        const VkWriteDescriptorSet write_desc_set{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = wbdh.dest,
            .dstBinding = 0,
            .dstArrayElement = wbdh.dst_arr_elem,
            .descriptorCount = wbdh.count,
            .descriptorType = wbdh.dtype,
            .pImageInfo = nullptr,
            .pBufferInfo = &descriptor_buffer_info[wbdh.desc_gbuff_offset],
            .pTexelBufferView = nullptr,
        };

        write_dsets.push_back(write_desc_set);
    }

    //
    // have [0, buffers) amount of instance buffers
    // so vertex buffer gets written at buffers  and
    // index buffer at buffers + 1
    const uint32_t idx_dset_vertexbuffer{ write_buf_desc_helpers[2].dst_arr_elem };
    const uint32_t idx_dset_indexbuffer{ write_buf_desc_helpers[3].dst_arr_elem };

    write_dsets.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = desc_sets[2],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &desc_img_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    });

    vkUpdateDescriptorSets(
        init_ctx.renderer->device(), static_cast<uint32_t>(size(write_dsets)), write_dsets.data(), 0, nullptr);

    // if (!pixel_buffer) {
    //     XR_LOG_ERR("Error creating image {}", pixel_buffer.error());
    //     return nullptr;
    // }

    // const auto rounded_slab_size = xray::math::roundup_to<VkDeviceSize>(
    //     1024 * 1024 * 4 * sizeof(float),
    //     init_ctx.renderer->physical().properties.base.properties.limits.minTexelBufferOffsetAlignment);
    //
    // const auto buffer_bytes_size = rounded_slab_size * init_ctx.renderer->buffering_setup().buffers;
    //
    // const VkBufferCreateInfo buff_create_info{
    //     .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    //     .pNext = nullptr,
    //     .flags = 0,
    //     .size = buffer_bytes_size,
    //     .usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    //     .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    //     .queueFamilyIndexCount = 0,
    //     .pQueueFamilyIndices = nullptr,
    // };
    //
    // VkBuffer buff{};
    // const VkResult err =
    //     WRAP_VULKAN_FUNC(vkCreateBuffer, init_ctx.renderer->device(), &buff_create_info, nullptr, &buff);
    // if (err != VK_SUCCESS) {
    //     return nullptr;
    // }
    //
    // const VkBufferMemoryRequirementsInfo2 buf_mem_req_info{
    //     .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
    //     .pNext = nullptr,
    //     .buffer = buff,
    // };
    //
    // VkMemoryRequirements2 buf_mem_info{
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
    //     .pNext = nullptr,
    // };
    // vkGetBufferMemoryRequirements2(init_ctx.renderer->device(), &buf_mem_req_info, &buf_mem_info);
    //
    // const VkMemoryAllocateFlagsInfo mem_alloc_flags{
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
    //     .pNext = nullptr,
    //     .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
    //     .deviceMask = 0,
    // };
    // const VkMemoryAllocateInfo mem_alloc_info{
    //     .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //     .pNext = &mem_alloc_flags,
    //     .allocationSize = buf_mem_info.memoryRequirements.size,
    //     .memoryTypeIndex = init_ctx.renderer->find_allocation_memory_type(
    //         buf_mem_info.memoryRequirements.memoryTypeBits,
    //         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    // };
    //
    // VkDeviceMemory allocated_memory{};
    // const VkResult mem_alloc_result =
    //     WRAP_VULKAN_FUNC(vkAllocateMemory, init_ctx.renderer->device(), &mem_alloc_info, nullptr,
    //     &allocated_memory);
    // if (mem_alloc_result != VK_SUCCESS)
    //     return nullptr;
    //
    // const VkBindBufferMemoryInfo bind_memory_info{
    //     .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
    //     .pNext = nullptr,
    //     .buffer = buff,
    //     .memory = allocated_memory,
    //     .memoryOffset = 0,
    // };
    //
    // const VkResult bind_mem_result =
    //     WRAP_VULKAN_FUNC(vkBindBufferMemory2, init_ctx.renderer->device(), 1, &bind_memory_info);
    // if (bind_mem_result != VK_SUCCESS)
    //     return nullptr;
    //
    // const VkBufferDeviceAddressInfo buffer_dev_addr_info{
    //     .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    //     .pNext = nullptr,
    //     .buffer = buff,
    // };
    //
    // const VkDeviceAddress buf_device_addr =
    //     vkGetBufferDeviceAddress(init_ctx.renderer->device(), &buffer_dev_addr_info);
    //
    // XR_LOG_INFO("Buffer rounded_slabv_size {}, bytes size {}, memory requirements {}, mem type bits {:0>32b},
    // address "
    //             "0x{:0>8x}",
    //             rounded_slab_size,
    //             buffer_bytes_size,
    //             buf_mem_info.memoryRequirements.size,
    //             buf_mem_info.memoryRequirements.memoryTypeBits,
    //             buf_device_addr);
    //
    // itlib::small_vector<rgb_color, 8> random_colors;
    // [&]() {
    //     //
    //     // https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
    //     random_engine rng{};
    //
    //     //
    //     // use golden ratio
    //     constexpr const float golden_ratio_conjugate = 0.618033988749895f;
    //     float hue = rng.next_float();
    //     for (uint32_t color = 0, max_colors = init_ctx.renderer->buffering_setup().buffers; color < max_colors;
    //          ++color) {
    //         random_colors.push_back(hsv_to_rgb(hsv_color{ hue * 360.0f, 0.5f, 0.95f }));
    //         hue += golden_ratio_conjugate;
    //         hue = fmod(hue, 1.0f);
    //     }
    // }();
    //
    // using stack_vector_t =
    //     xray::base::sequence_container_veneer<itlib::small_vector<VkBufferView>, VkResourceDeleter_VkBufferView>;
    // using heap_vector_t =
    //     xray::base::sequence_container_veneer<std::vector<VkBufferView>, VkResourceDeleter_VkBufferView>;
    //
    // stack_vector_t buffer_views{ VkResourceDeleter_VkBufferView{ init_ctx.renderer->device() } };
    //
    // UniqueMemoryMapping::create(init_ctx.renderer->device(), allocated_memory, 0, 0, 0)
    //     .map([&random_colors, &buffer_views, buff, rounded_slab_size, &init_ctx](UniqueMemoryMapping mm) {
    //         for (size_t idx = 0; idx < random_colors.size(); ++idx) {
    //
    //             span<rgb_color> buffer_mem_span{ static_cast<rgb_color*>(mm._mapped_memory) + 1024 * 1024 * idx,
    //                                              1024 * 1024 };
    //             uninitialized_fill(buffer_mem_span.begin(), buffer_mem_span.end(), random_colors[idx]);
    //
    //             const VkBufferViewCreateInfo buf_view_create_info{
    //                 .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
    //                 .pNext = nullptr,
    //                 .flags = 0,
    //                 .buffer = buff,
    //                 .format = VK_FORMAT_R32G32B32A32_SFLOAT,
    //                 .offset = rounded_slab_size * idx,
    //                 .range = 1024 * 1024 * 4 * sizeof(float),
    //             };
    //
    //             VkBufferView buff_view{};
    //             const VkResult view_create_result = WRAP_VULKAN_FUNC(
    //                 vkCreateBufferView, init_ctx.renderer->device(), &buf_view_create_info, nullptr, &buff_view);
    //
    //             if (view_create_result != VK_SUCCESS)
    //                 return;
    //
    //             buffer_views.push_back(buff_view);
    //         }
    //     });
    //

    //
    // wait for data to be uploaded to GPU
    init_ctx.renderer->wait_on_packages({ pkgs->pkg });

    return xray::base::make_unique<TriangleDemo>(PrivateConstructionToken{},
                                                 init_ctx,
                                                 std::move(*g_ubo),
                                                 std::move(*g_vertexbuffer),
                                                 std::move(*g_indexbuffer),
                                                 std::move(*g_instance_buffer),
                                                 std::move(*pipeline),
                                                 std::move(image),
                                                 std::move(img_view),
                                                 std::move(sampler),
                                                 idx_dset_vertexbuffer,
                                                 idx_dset_indexbuffer,
                                                 std::move(desc_sets));
}

void
dvk::TriangleDemo::event_handler(const xray::ui::window_event& evt)
{
    if (is_input_event(evt)) {
        if (evt.event.key.keycode == KeySymbol::escape) {
            _quit_receiver();
            return;
        }
    }
}

void
dvk::TriangleDemo::loop_event(const app::RenderEvent& render_event)
{
    static constexpr const auto sixty_herz = std::chrono::duration<float, std::milli>{ 1000.0f / 60.0f };

    _timer.end();
    const auto elapsed_duration = std::chrono::duration<float, std::milli>{ _timer.ts_end() - _timer.ts_start() };

    if (elapsed_duration > sixty_herz) {
        _angle += 0.025f;
        if (_angle >= xray::math::two_pi<float>)
            _angle -= xray::math::two_pi<float>;
        _timer.update_and_reset();
    }

    const FrameRenderData frt{ render_event.renderer->begin_rendering() };

    render_event.renderer->dbg_marker_begin(frt.cmd_buf, "Update UBO & instances", color_palette::web::orange_red);
    _g_ubo.mmap(render_event.renderer->device(), frt.id).map([frt](UniqueMemoryMapping ubo) {
        FrameGlobalData* fgd = ubo.as<FrameGlobalData>();
        fgd->frame = frt.id;
        fgd->world_view_proj = mat4f::stdc::identity;
        fgd->view = mat4f::stdc::identity;
        fgd->eye_pos = vec3f::stdc::zero;
        fgd->ortho = mat4f::stdc::identity;
        fgd->projection = mat4f::stdc::identity;
    });

    _g_instancebuffer.mmap(render_event.renderer->device(), frt.id).map([this](UniqueMemoryMapping inst_buf) {
        const xray::math::scalar2x3<float> r = xray::math::R2::rotate(_angle);
        const std::array<float, 4> vkmtx{ r.a00, r.a01, r.a10, r.a11 };

        InstanceRenderInfo* inst = inst_buf.as<InstanceRenderInfo>();
        inst->mtl = 0;
        inst->model = mat4f::stdc::identity;
        inst->idx_buff = _ibuffer_handle;
        inst->vtx_buff = _vbuffer_handle;
        inst->mtl_coll = 0;
    });

    render_event.renderer->dbg_marker_end(frt.cmd_buf);

    const VkViewport viewport{
        .x = 0.0f,
        .y = static_cast<float>(frt.fbsize.height),
        .width = static_cast<float>(frt.fbsize.width),
        .height = -static_cast<float>(frt.fbsize.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(frt.cmd_buf, 0, 1, &viewport);

    render_event.renderer->dbg_marker_insert(frt.cmd_buf, "Rendering triangle", color_palette::web::sea_green);

    const VkRect2D scissor{
        .offset = VkOffset2D{ 0, 0 },
        .extent = frt.fbsize,
    };

    vkCmdSetScissor(frt.cmd_buf, 0, 1, &scissor);
    render_event.renderer->clear_attachments(frt.cmd_buf, 1.0f, 0.0f, 1.0f);

    vkCmdBindPipeline(frt.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.handle());
    vkCmdBindDescriptorSets(frt.cmd_buf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _pipeline.layout(),
                            0,
                            static_cast<uint32_t>(_desc_sets.size()),
                            _desc_sets.data(),
                            0,
                            nullptr);

    vkCmdPushConstants(
        frt.cmd_buf, _pipeline.layout(), VK_SHADER_STAGE_ALL, 0, static_cast<uint32_t>(sizeof(frt.id)), &frt.id);

    vkCmdDraw(frt.cmd_buf, 3, 1, 0, 0);
    render_event.renderer->end_rendering();
}
