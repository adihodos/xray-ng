#pragma once

#include <cstdint>

#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <swl/variant.hpp>

#include "xray/math/math.units.hpp"
#include "xray/rendering/shapes.system/shape.defs.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.defs.hpp"

namespace xray::base {
class ConfigSystem;
}

namespace xray::scene {
class SceneResources;
}

namespace xray::rendering {

class VulkanRenderer;
struct FrameRenderData;

struct ShapeSystemRenderContext
{
    VulkanRenderer* renderer;
    const FrameRenderData* frame_data;
    const scene::SceneResources* sres;
};

class ShapesDrawingSystem
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() noexcept = default;
    };

  public:
    ShapesDrawingSystem(ShapesDrawingSystem&&) noexcept = default;

    ShapesDrawingSystem(PrivateConstructionToken,
                        BindlessStorageBufferResourceHandleEntryPair sbo,
                        ShapeSetup* mapped_gpu_mem,
                        const uint32_t capacity);

    static tl::expected<ShapesDrawingSystem, VulkanError> create(VulkanRenderer& renderer);

    void draw_shape(const ShapeKind kind,
                    const float xpos,
                    const float ypos,
                    const float size,
                    const float linewidth,
                    const float antialias,
                    const math::RadiansF32 orientation,
                    const uint32_t foreground_color,
                    const uint32_t background_color = 0);

    void render(const ShapeSystemRenderContext& render_ctx);

  private:
    BindlessStorageBufferResourceHandleEntryPair _sbodata;
    ShapeSetup* _buffer_ptr{};
    size_t _shapes_capacity{};
    uint32_t _buffer_offset{};
    uint32_t _shape_count{};
};

}
