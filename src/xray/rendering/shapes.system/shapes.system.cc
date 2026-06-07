#include "xray/rendering/shapes.system/shapes.system.hpp"

#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.packed.pushconst.hpp"
#include "xray/scene/scene.definition.hpp"
#include "xray/rendering/colors/color_palettes.hpp"

xray::rendering::ShapesDrawingSystem::ShapesDrawingSystem(PrivateConstructionToken,
                                                          BindlessStorageBufferResourceHandleEntryPair sbo,
                                                          ShapeSetup* mapped_gpu_mem,
                                                          const uint32_t capacity)
    : _sbodata{ sbo }
    , _buffer_ptr{ mapped_gpu_mem }
    , _shapes_capacity{ capacity }
{
}

tl::expected<xray::rendering::ShapesDrawingSystem, xray::rendering::VulkanError>
xray::rendering::ShapesDrawingSystem::create(VulkanRenderer& renderer)
{
    static constexpr const uint32_t MAX_SHAPES_COUNT = 1024;
    auto shapes_sbo = VulkanBuffer::create(
        renderer,
        VulkanBufferCreateInfo{
            .name_tag = "shapes.instancedata",
            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .bytes = sizeof(ShapeSetup) * MAX_SHAPES_COUNT,
            .frames = renderer.max_inflight_frames(),
        });
    XR_VK_PROPAGATE_ERROR(shapes_sbo);

    void* mapped_buffer{};
    const VkResult map_result =         vkMapMemory( renderer.device(), shapes_sbo->memory_handle(), 0, VK_WHOLE_SIZE, 0, &mapped_buffer);

    XR_VK_CHECK_RESULT(map_result);

    const BindlessStorageBufferResourceHandleEntryPair sbo_entry = renderer.bindless_sys().add_chunked_storage_buffer(
        std::move(*shapes_sbo), renderer.buffering_setup().buffers, tl::nullopt);

    return tl::expected<ShapesDrawingSystem, VulkanError>{
        tl::in_place, PrivateConstructionToken{}, sbo_entry, static_cast<ShapeSetup*>(mapped_buffer), MAX_SHAPES_COUNT,
    };
}

void
xray::rendering::ShapesDrawingSystem::draw_shape(const ShapeKind kind,
                                                 const float xpos,
                                                 const float ypos,
                                                 const float size,
                                                 const float linewidth,
                                                 const float antialias,
                                                 const math::RadiansF32 orientation,
                                                 const uint32_t foreground_color,
                                                 const uint32_t background_color)
{
    if (_shape_count >= _shapes_capacity) {
        return;
    }

    using namespace xray::math;
    _buffer_ptr[_buffer_offset + _shape_count] = ShapeSetup{
        .position = vec3f32{ xpos, ypos, 0.0f },
        .size = size,
        .cos_theta = std::cos(orientation.value_of()),
        .sin_theta = std::sin(orientation.value_of()),
        .line_width = linewidth,
        .antialias = antialias,
        .fg_color = foreground_color,
        .bg_color = background_color,
        .shape_kind = static_cast<uint32_t>(kind),
    };

    _shape_count += 1;
}

void
xray::rendering::ShapesDrawingSystem::render(const ShapeSystemRenderContext& rctx)
{
    if (_shape_count >= 1) {
        [[maybe_unused]] const auto marker = rctx.renderer->dbg_marker_begin(
            rctx.frame_data->cmd_buf, "Draw 2D shapes", xray::rendering::color_palette::flat::orange500);

        vkCmdBindPipeline(
            rctx.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, rctx.sres->pipelines.p_shapes.handle());

        const xray::rendering::PackedU32PushConstant push_const{
            _sbodata.first,
            0,
            rctx.frame_data->id,
        };
        vkCmdPushConstants(rctx.frame_data->cmd_buf,
                           rctx.sres->pipelines.p_sprites.layout(),
                           VK_SHADER_STAGE_ALL,
                           0,
                           push_const.size(),
                           push_const.as_bytes().data());
        vkCmdDraw(rctx.frame_data->cmd_buf, 1, static_cast<uint32_t>(_shape_count), 0, 0);
    }

    _shape_count = 0;
    _buffer_offset = static_cast<uint32_t>(_shapes_capacity * rctx.frame_data->id);
}
