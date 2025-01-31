#pragma once

#include "xray/xray.hpp"

#include <string_view>
#include <span>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <vulkan/vulkan_core.h>

#include <swl/variant.hpp>
#include <tl/optional.hpp>
#include <tl/expected.hpp>

#include "xray/base/containers/arena.string.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vertex_format/vertex_format.hpp"

namespace xray::base {
struct MemoryArena;
}

namespace xray::rendering {

class VulkanRenderer;

//
// TODO: there's way too many redundant VkDevice handles inside of all
// these unique handle objects, needs redesigning to avoid this and reduce the
// memory footprint

class GraphicsPipelineBuilder;

class GraphicsPipeline
{
  private:
    friend class GraphicsPipelineBuilder;
    struct BindlessLayout
    {
        VkPipelineLayout layout;
        std::span<const VkDescriptorSetLayout> set_layouts;
    };

    struct OwnedLayout
    {
        VkPipelineLayout layout;
        std::vector<VkDescriptorSetLayout> set_layouts;
    };

    using pipeline_layout_t = swl::variant<BindlessLayout, OwnedLayout>;

    xrUniqueVkPipeline _pipeline{};
    pipeline_layout_t _layout{};

  public:
    GraphicsPipeline(xrUniqueVkPipeline p, pipeline_layout_t p_layout)
        : _pipeline{ std::move(p) }
        , _layout{ std::move(p_layout) }
    {
    }

    std::span<const VkDescriptorSetLayout> descriptor_sets_layouts() const noexcept;
    VkPipelineLayout layout() const noexcept;
    VkPipeline handle() const noexcept { return xray::base::raw_ptr(_pipeline); }
    void release_resources(VkDevice device, const VkAllocationCallbacks* alloc_cb = nullptr);
};

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

struct GraphicsPipelineCreateData
{
    uint16_t uniform_descriptors{ 1 };
    uint16_t storage_buffer_descriptors{ 1 };
    uint16_t combined_image_sampler_descriptors{ 1 };
    uint16_t image_descriptors{ 1 };
};

struct ShaderBuildOptions
{
    static constexpr const uint32_t Compile_EnabledOptimizations = 1 << 0;
    static constexpr const uint32_t Compile_GenerateDebugInfo = 1 << 1;
    static constexpr const uint32_t Compile_WarningsToErrors = 1 << 2;
    static constexpr const uint32_t Compile_SuppressWarnings = 1 << 2;
    static constexpr const uint32_t Compile_DumpShaderCode = 1 << 20;

    swl::variant<std::string_view, std::filesystem::path> code_or_file_path;
    std::string_view entry_point{};
    std::span<const std::pair<base::containers::string, base::containers::string>> defines{};
    uint32_t compile_options{ Compile_GenerateDebugInfo };
};

class GraphicsPipelineBuilder
{
  public:
    GraphicsPipelineBuilder(base::MemoryArena* arena_perm, base::MemoryArena* arena_temp)
        : _arena_perm(arena_perm)
        , _arena_temp(arena_temp)
    {
    }

    GraphicsPipelineBuilder& add_shader(const uint32_t stage, ShaderBuildOptions so)
    {
        _stage_modules.emplace(stage, so);
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

    GraphicsPipelineBuilder& input_state(const std::span<const VertexInputAttributeDescriptor> vtx_input_atts);

    tl::expected<GraphicsPipeline, VulkanError> create(const VulkanRenderer& renderer,
                                                       const GraphicsPipelineCreateData& pcd)
    {
        return create_impl(renderer, PipelineType::Owned, pcd);
    }

    tl::expected<GraphicsPipeline, VulkanError> create_bindless(const VulkanRenderer& renderer)
    {
        return create_impl(renderer, PipelineType::Bindless, GraphicsPipelineCreateData{});
    }

  private:
    enum class PipelineType
    {
        Bindless,
        Owned,
    };

    tl::expected<GraphicsPipeline, VulkanError> create_impl(const VulkanRenderer& renderer,
                                                            const PipelineType pipeline_type,
                                                            const GraphicsPipelineCreateData& pcd);

    base::MemoryArena* _arena_perm;
    base::MemoryArena* _arena_temp;
    std::unordered_map<uint32_t, ShaderBuildOptions> _stage_modules;
    bool _optimize_shaders{ false };
    InputAssemblyState _input_assembly{};
    std::vector<VkVertexInputBindingDescription> _vertex_binding_description;
    std::vector<VkVertexInputAttributeDescription> _vertex_attribute_description;
    RasterizationState _raster{};
    DepthStencilState _depth_stencil{};
    VkPipelineColorBlendAttachmentState _colorblend{
        .blendEnable = false,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
            VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
    };
    std::vector<VkDynamicState> _dynstate;
};

} // namespace xray::rendering
