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
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"

namespace xray::rendering {

class VulkanRenderer;

struct JobWaitToken
{
  public:
    JobWaitToken(VkCommandBuffer cmd_buf, xrUniqueVkFence&& fence, VulkanRenderer* renderer) noexcept
        : _cmdbuf{ cmd_buf }
        , _fence{ xray::base::unique_pointer_release(fence) }
        , _renderer{ renderer }
    {
    }

    JobWaitToken(JobWaitToken&& rhs) noexcept
        : _cmdbuf{ std::exchange(rhs._cmdbuf, VK_NULL_HANDLE) }
        , _fence{ std::exchange(rhs._fence, VK_NULL_HANDLE) }
        , _renderer{ rhs._renderer }
    {
    }

    JobWaitToken(const JobWaitToken&) = delete;
    JobWaitToken& operator=(const JobWaitToken&&) = delete;

    JobWaitToken& operator=(JobWaitToken&& rhs) noexcept
    {
        std::exchange(_cmdbuf, rhs._cmdbuf);
        std::exchange(_fence, rhs._fence);
        std::exchange(_renderer, rhs._renderer);
        return *this;
    }

    ~JobWaitToken() { dispose(); }

    void dispose() noexcept;
    void wait() noexcept;
    VkCommandBuffer command_buffer() const noexcept { return _cmdbuf; }
    VkFence fence() const noexcept { return _fence; }

  private:
    VkCommandBuffer _cmdbuf;
    VkFence _fence;
    VulkanRenderer* _renderer;
};

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
