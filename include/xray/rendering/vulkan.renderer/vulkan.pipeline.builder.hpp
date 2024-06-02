#pragma once

#include "xray/xray.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"

#include <string_view>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <vulkan/vulkan_core.h>

#include <swl/variant.hpp>
#include <tl/optional.hpp>

namespace xray::rendering {

struct ShaderStage
{
    static constexpr const uint32_t Vertex{ static_cast<uint32_t>(VK_SHADER_STAGE_VERTEX_BIT) };
    static constexpr const uint32_t Fragment{ static_cast<uint32_t>(VK_SHADER_STAGE_FRAGMENT_BIT) };
};

struct InputAssemblyState
{
    VkPrimitiveTopology topology{ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST };
    bool restart_enabled{ false };
};

struct RasterizationState
{
    VkPolygonMode poly_mode{ VK_POLYGON_MODE_FILL };
    VkCullModeFlags cull_mode{ VK_CULL_MODE_BACK_BIT };
    VkFrontFace front_face{ VK_FRONT_FACE_COUNTER_CLOCKWISE };
    float line_width{ 1.0f };
};

struct DepthStencilState
{
    bool depth_test_enable{ true };
    bool depth_write_enable{ true };
    VkCompareOp depth_op{ VK_COMPARE_OP_LESS_OR_EQUAL };
    float min_depth{ 0.0f };
    float max_depth{ 1.0f };
};

struct GraphicsPipeline
{
    xrUniqueVkPipeline pipeline;
    xrUniqueVkPipelineLayout layout;
    std::vector<xrUniqueVkDescriptorSetLayout> descriptor_set_layout;
};

class GraphicsPipelineBuilder
{
  public:
    GraphicsPipelineBuilder();

    GraphicsPipelineBuilder& add_shader(const uint32_t stage, std::string_view code)
    {
        _stage_modules.emplace(stage, code);
        return *this;
    }

    GraphicsPipelineBuilder& add_shader(const uint32_t stage, std::filesystem::path p)
    {
        _stage_modules.emplace(stage, std::move(p));
        return *this;
    }

    GraphicsPipelineBuilder& input_assembly_state(const InputAssemblyState ia_state)
    {
        _input_assembly = ia_state;
        return *this;
    }

    GraphicsPipelineBuilder& rasterization_state(const RasterizationState& raster_state)
    {
        _raster = raster_state;
        return *this;
    }

    GraphicsPipelineBuilder& depth_stencil_state(const DepthStencilState& depth_stencil)
    {
        _depth_stencil = depth_stencil;
        return *this;
    }

    GraphicsPipelineBuilder& color_blend(const VkPipelineColorBlendAttachmentState color_blend)
    {
        _colorblend = color_blend;
        return *this;
    }

    GraphicsPipelineBuilder& dynamic_state(std::initializer_list<VkDynamicState> dyn_state)
    {
        _dynstate.assign(dyn_state);
        return *this;
    }

    tl::optional<GraphicsPipeline> create(VkDevice device,
                                              const VkPipelineRenderingCreateInfo& rendering_create_info);

  private:
    using ShaderModuleSource = swl::variant<std::string_view, std::filesystem::path>;
    std::unordered_map<uint32_t, ShaderModuleSource> _stage_modules;
    bool _optimize_shaders{ false };
    InputAssemblyState _input_assembly{};
    RasterizationState _raster{};
    DepthStencilState _depth_stencil{};
    VkPipelineColorBlendAttachmentState _colorblend{ .blendEnable = false,
                                                     .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                                                     .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
                                                     .colorBlendOp = VK_BLEND_OP_ADD,
                                                     .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                                                     .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                                                     .alphaBlendOp = VK_BLEND_OP_ADD,
                                                     .colorWriteMask =
                                                         VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                                                         VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT };

    std::vector<VkDynamicState> _dynstate;
};

} // namespace xray::rendering
