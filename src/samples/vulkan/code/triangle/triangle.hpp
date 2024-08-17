#pragma once

#include "xray/xray.hpp"

#include <vector>

#include "demo_base.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"

namespace dvk {

class TriangleDemo : public app::DemoBase
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    virtual void event_handler(const xray::ui::window_event& evt) override;
    virtual void loop_event(const app::RenderEvent&) override;

    static std::string_view short_desc() noexcept { return "Vulkan triangle."; }
    static std::string_view detailed_desc() noexcept { return "Rendering a simple triangle using Vulkan."; }
    static xray::base::unique_pointer<app::DemoBase> create(const app::init_context_t& initContext);

  private:
    xray::rendering::VulkanBuffer _g_ubo;
    xray::rendering::VulkanBuffer _g_vertexbuffer;
    xray::rendering::VulkanBuffer _g_indexbuffer;
    xray::rendering::VulkanBuffer _g_instancebuffer;
    xray::rendering::GraphicsPipeline _pipeline;
    xray::rendering::VulkanImage _pixel_buffer;
    xray::rendering::xrUniqueVkImageView _imageview;
    xray::rendering::xrUniqueVkSampler _sampler;
    std::vector<VkDescriptorSet> _desc_sets;
    float _angle{};
    uint32_t _vbuffer_handle{};
    uint32_t _ibuffer_handle{};

  public:
    TriangleDemo(PrivateConstructionToken,
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
                 std::vector<VkDescriptorSet> desc_sets);
};

} //
