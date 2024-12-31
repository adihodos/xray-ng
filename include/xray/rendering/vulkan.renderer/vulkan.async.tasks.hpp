#pragma once

#include <vector>
#include <concurrencpp/concurrencpp.h>
#include <tl/expected.hpp>

#include "xray/rendering/geometry.hpp"
#include "xray/rendering/geometry.importer.gltf.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"

namespace xray::rendering {

class VulkanRenderer;

struct GeometryWithRenderData
{
    xray::rendering::LoadedGeometry geometry;
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
    VulkanBuffer material_buffer;
    std::vector<VulkanImage> images;
    uint32_t images_slot;
    uint32_t vertexcount;
    uint32_t indexcount;
};

struct GeneratedGeometryWithRenderData
{
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
    GraphicsPipeline pipeline;
    std::vector<ObjectDrawData> objects;
};

struct RendererAsyncTasks
{
    static concurrencpp::result<tl::expected<GeometryWithRenderData, VulkanError>> create_rendering_resources_task(
        concurrencpp::executor_tag,
        concurrencpp::thread_pool_executor*,
        VulkanRenderer* r,
        xray::rendering::LoadedGeometry geometry);

    static concurrencpp::result<tl::expected<GeneratedGeometryWithRenderData, VulkanError>>
    create_generated_geometry_resources_task(concurrencpp::executor_tag,
                                             concurrencpp::thread_pool_executor*,
                                             VulkanRenderer* r);
};

}
