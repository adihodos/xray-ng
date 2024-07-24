#include "triangle.hpp"

#include <string_view>
#include <array>
#include <random>
#include <Lz/Lz.hpp>

#include <itlib/small_vector.hpp>

#include "xray/rendering/vulkan.renderer/vulkan.pipeline.builder.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/ui/events.hpp"
#include "init_context.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/math/scalar2x3_math.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_base.hpp"
#include "xray/rendering/colors/hsv_color.hpp"
#include "xray/rendering/colors/color_cast_rgb_hsv.hpp"
#include "xray/base/veneers/sequence_container_veneer.hpp"
#include "xray/base/app_config.hpp"

using namespace std;
using namespace xray::rendering;
using namespace xray::base;
using namespace xray::ui;

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
                                xray::rendering::GraphicsPipeline pipeline)
    : app::DemoBase{ init_context }
    , _pipeline{ std::move(pipeline) }
{
    _timer.start();
}

xray::base::unique_pointer<app::DemoBase>
dvk::TriangleDemo::create(const app::init_context_t& init_ctx)
{
    const char* const tex_file{ "skyboxes/skybox/skybox1/skybox.bc5.ktx2" };
    auto pixel_buffer{ ManagedImage::from_file(*init_ctx.renderer, init_ctx.cfg->texture_path(tex_file)) };
    tl::optional<GraphicsPipeline> pipeline{
        GraphicsPipelineBuilder{}
            .add_shader(ShaderStage::Vertex, init_ctx.cfg->shader_root() / "triangle/tri.vert")
            .add_shader(ShaderStage::Fragment, init_ctx.cfg->shader_root() / "triangle/tri.frag")
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .create(*init_ctx.renderer),
    };

    if (!pipeline)
        return nullptr;

    // if (!pixel_buffer) {
    //     XR_LOG_ERR("Error creating image {}", pixel_buffer.error());
    //     return nullptr;
    // }

    const auto rounded_slab_size = xray::math::roundup_to<VkDeviceSize>(
        1024 * 1024 * 4 * sizeof(float),
        init_ctx.renderer->physical().properties.base.properties.limits.minTexelBufferOffsetAlignment);

    const auto buffer_bytes_size = rounded_slab_size * init_ctx.renderer->buffering_setup().buffers;

    const VkBufferCreateInfo buff_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = buffer_bytes_size,
        .usage = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkBuffer buff{};
    const VkResult err =
        WRAP_VULKAN_FUNC(vkCreateBuffer, init_ctx.renderer->device(), &buff_create_info, nullptr, &buff);
    if (err != VK_SUCCESS) {
        return nullptr;
    }

    const VkBufferMemoryRequirementsInfo2 buf_mem_req_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .pNext = nullptr,
        .buffer = buff,
    };

    VkMemoryRequirements2 buf_mem_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = nullptr,
    };
    vkGetBufferMemoryRequirements2(init_ctx.renderer->device(), &buf_mem_req_info, &buf_mem_info);

    const VkMemoryAllocateFlagsInfo mem_alloc_flags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0,
    };
    const VkMemoryAllocateInfo mem_alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &mem_alloc_flags,
        .allocationSize = buf_mem_info.memoryRequirements.size,
        .memoryTypeIndex = init_ctx.renderer->find_allocation_memory_type(
            buf_mem_info.memoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    };

    VkDeviceMemory allocated_memory{};
    const VkResult mem_alloc_result =
        WRAP_VULKAN_FUNC(vkAllocateMemory, init_ctx.renderer->device(), &mem_alloc_info, nullptr, &allocated_memory);
    if (mem_alloc_result != VK_SUCCESS)
        return nullptr;

    const VkBindBufferMemoryInfo bind_memory_info{
        .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
        .pNext = nullptr,
        .buffer = buff,
        .memory = allocated_memory,
        .memoryOffset = 0,
    };

    const VkResult bind_mem_result =
        WRAP_VULKAN_FUNC(vkBindBufferMemory2, init_ctx.renderer->device(), 1, &bind_memory_info);
    if (bind_mem_result != VK_SUCCESS)
        return nullptr;

    const VkBufferDeviceAddressInfo buffer_dev_addr_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = buff,
    };

    const VkDeviceAddress buf_device_addr =
        vkGetBufferDeviceAddress(init_ctx.renderer->device(), &buffer_dev_addr_info);

    XR_LOG_INFO("Buffer rounded_slabv_size {}, bytes size {}, memory requirements {}, mem type bits {:0>32b}, address "
                "0x{:0>8x}",
                rounded_slab_size,
                buffer_bytes_size,
                buf_mem_info.memoryRequirements.size,
                buf_mem_info.memoryRequirements.memoryTypeBits,
                buf_device_addr);

    itlib::small_vector<rgb_color, 8> random_colors;
    [&]() {
        //
        // https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
        random_engine rng{};

        //
        // use golden ratio
        constexpr const float golden_ratio_conjugate = 0.618033988749895f;
        float hue = rng.next_float();
        for (uint32_t color = 0, max_colors = init_ctx.renderer->buffering_setup().buffers; color < max_colors;
             ++color) {
            random_colors.push_back(hsv_to_rgb(hsv_color{ hue * 360.0f, 0.5f, 0.95f }));
            hue += golden_ratio_conjugate;
            hue = fmod(hue, 1.0f);
        }
    }();

    using stack_vector_t =
        xray::base::sequence_container_veneer<itlib::small_vector<VkBufferView>, VkResourceDeleter_VkBufferView>;
    using heap_vector_t =
        xray::base::sequence_container_veneer<std::vector<VkBufferView>, VkResourceDeleter_VkBufferView>;

    stack_vector_t buffer_views{ VkResourceDeleter_VkBufferView{ init_ctx.renderer->device() } };

    UniqueMemoryMapping::create(init_ctx.renderer->device(), allocated_memory, 0, 0, 0)
        .map([&random_colors, &buffer_views, buff, rounded_slab_size, &init_ctx](UniqueMemoryMapping mm) {
            for (size_t idx = 0; idx < random_colors.size(); ++idx) {

                span<rgb_color> buffer_mem_span{ static_cast<rgb_color*>(mm._mapped_memory) + 1024 * 1024 * idx,
                                                 1024 * 1024 };
                uninitialized_fill(buffer_mem_span.begin(), buffer_mem_span.end(), random_colors[idx]);

                const VkBufferViewCreateInfo buf_view_create_info{
                    .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .buffer = buff,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = rounded_slab_size * idx,
                    .range = 1024 * 1024 * 4 * sizeof(float),
                };

                VkBufferView buff_view{};
                const VkResult view_create_result = WRAP_VULKAN_FUNC(
                    vkCreateBufferView, init_ctx.renderer->device(), &buf_view_create_info, nullptr, &buff_view);

                if (view_create_result != VK_SUCCESS)
                    return;

                buffer_views.push_back(buff_view);
            }
        });

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

    return xray::base::make_unique<TriangleDemo>(PrivateConstructionToken{}, init_ctx, std::move(*pipeline.take()));
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

    const VkViewport viewport{
        .x = 0.0f,
        .y = static_cast<float>(frt.fbsize.height),
        .width = static_cast<float>(frt.fbsize.width),
        .height = -static_cast<float>(frt.fbsize.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(frt.cmd_buf, 0, 1, &viewport);

    const VkRect2D scissor{
        .offset = VkOffset2D{ 0, 0 },
        .extent = frt.fbsize,
    };

    vkCmdSetScissor(frt.cmd_buf, 0, 1, &scissor);
    render_event.renderer->clear_attachments(frt.cmd_buf, 1.0f, 0.0f, 1.0f, frt.fbsize.width, frt.fbsize.height);

    const xray::math::scalar2x3<float> r = xray::math::R2::rotate(_angle);
    const std::array<float, 4> vkmtx{ r.a00, r.a01, r.a10, r.a11 };

    vkCmdBindPipeline(frt.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline.pipeline_handle());

    struct PushConstants
    {
        float mtx[4];
        xray::math::vec3f color;
    } const push_consts{
        .mtx = { r.a00, r.a01, r.a10, r.a11 },
        .color = { 0.0f, 1.0f, 0.0f },
    };

    vkCmdPushConstants(frt.cmd_buf,
                       _pipeline.layout_handle(),
                       VK_SHADER_STAGE_ALL,
                       0,
                       static_cast<uint32_t>(sizeof(push_consts)),
                       &push_consts);

    vkCmdDraw(frt.cmd_buf, 3, 1, 0, 0);
    render_event.renderer->end_rendering();
}
