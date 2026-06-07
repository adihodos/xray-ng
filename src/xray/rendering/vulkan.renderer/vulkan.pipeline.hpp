#pragma once

#include "xray/xray.hpp"

#include <cstdint>
#include <string_view>
#include <span>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

#include <tl/expected.hpp>

#include "xray/rendering/shader.code.builder.hpp"
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

enum class VulkanPipelineKind : uint8_t {
	Graphics,
	Compute,
};

class VulkanPipelineBuilder;

class VulkanPipeline {
private:
	friend class VulkanPipelineBuilder;
	struct BindlessLayout {
		VkPipelineLayout layout;
		std::span<const VkDescriptorSetLayout> set_layouts;
	};

	struct OwnedLayout {
		VkPipelineLayout layout;
		std::vector<VkDescriptorSetLayout> set_layouts;
	};

	// enum xrPipelineLayout_t {
	// 	Bindless,
	// 	Owned,
	// };
	//
	// struct pipeline_layout_t {
	// 	xrPipelineLayout_t type_;
	// 	union {
	// 		BindlessLayout bindless;
	// 		OwnedLayout owned;
	// 	};
	// };

	using pipeline_layout_t = swl::variant<BindlessLayout, OwnedLayout>;

	VkPipeline _pipeline{};
	pipeline_layout_t _layout{};

public:
	VulkanPipeline(VkPipeline p, pipeline_layout_t p_layout) noexcept : _pipeline{p}, _layout{std::move(p_layout)} {}
	~VulkanPipeline();

	VulkanPipeline(const VulkanPipeline&) = delete;
	VulkanPipeline(VulkanPipeline&& rhs) noexcept
		: _pipeline(std::exchange(rhs._pipeline, nullptr)), _layout(std::exchange(rhs._layout, {})) {}

	std::span<const VkDescriptorSetLayout> descriptor_sets_layouts() const noexcept;
	VkPipelineLayout layout() const noexcept;
	VkPipeline handle() const noexcept { return _pipeline; }
	void release_resources(VkDevice device, const VkAllocationCallbacks* alloc_cb = nullptr);
};

struct ShaderStage {
	static constexpr const uint32_t Vertex{static_cast<uint32_t>(VK_SHADER_STAGE_VERTEX_BIT)};
	static constexpr const uint32_t Fragment{static_cast<uint32_t>(VK_SHADER_STAGE_FRAGMENT_BIT)};
	static constexpr const uint32_t Compute{static_cast<uint32_t>(VK_SHADER_STAGE_COMPUTE_BIT)};
};

struct InputAssemblyState {
	VkPrimitiveTopology topology{VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};
	bool restart_enabled{false};
};

struct RasterizationState {
	VkPolygonMode poly_mode{VK_POLYGON_MODE_FILL};
	VkCullModeFlags cull_mode{VK_CULL_MODE_BACK_BIT};
	VkFrontFace front_face{VK_FRONT_FACE_COUNTER_CLOCKWISE};
	float line_width{1.0f};
};

struct DepthStencilState {
	bool depth_test_enable{true};
	bool depth_write_enable{true};
	VkCompareOp depth_op{VK_COMPARE_OP_LESS_OR_EQUAL};
	float min_depth{0.0f};
	float max_depth{1.0f};
};

struct VulkanPipelineCreateData {
	uint16_t uniform_descriptors{1};
	uint16_t storage_buffer_descriptors{1};
	uint16_t combined_image_sampler_descriptors{1};
	uint16_t image_descriptors{1};
};

struct VulkanPipelineTemplate {
	VkPipelineLayout layout;
	std::span<const VkDescriptorSetLayout> descriptor_set_layouts;
};

class VulkanPipelineBuilder {
public:
	VulkanPipelineBuilder(base::MemoryArena* arena_perm, std::string_view tag = {})
		: _arena_perm(arena_perm), _tag_name{tag} {}

	VulkanPipelineBuilder& add_shader(const uint32_t stage, ShaderBuildOptions so) {
		_stage_modules.emplace(stage, so);
		return *this;
	}

	VulkanPipelineBuilder& input_assembly_state(const InputAssemblyState ia_state) {
		_input_assembly = ia_state;
		return *this;
	}

	VulkanPipelineBuilder& rasterization_state(const RasterizationState& raster_state) {
		_raster = raster_state;
		return *this;
	}

	VulkanPipelineBuilder& depth_stencil_state(const DepthStencilState& depth_stencil) {
		_depth_stencil = depth_stencil;
		return *this;
	}

	VulkanPipelineBuilder& color_blend(const VkPipelineColorBlendAttachmentState color_blend) {
		_colorblend = color_blend;
		return *this;
	}

	VulkanPipelineBuilder& dynamic_state(std::initializer_list<VkDynamicState> dyn_state) {
		_dynstate.assign(dyn_state);
		return *this;
	}

	VulkanPipelineBuilder& input_state(const std::span<const VertexInputAttributeDescriptor> vtx_input_atts);

	[[nodiscard]] tl::expected<VulkanPipeline, VulkanError> create(
		const VulkanRenderer& renderer,
		const VulkanPipelineKind pipeline_kind,
		const swl::variant<VulkanPipelineCreateData, VulkanPipelineTemplate> create_data
	);

private:
	base::MemoryArena* _arena_perm;
	std::string_view _tag_name;
	std::unordered_map<uint32_t, ShaderBuildOptions> _stage_modules;
	bool _optimize_shaders{false};
	InputAssemblyState _input_assembly{};
	std::vector<VkVertexInputBindingDescription> _vertex_binding_description;
	std::vector<VkVertexInputAttributeDescription> _vertex_attribute_description;
	RasterizationState _raster{};
	DepthStencilState _depth_stencil{};
	VkPipelineColorBlendAttachmentState _colorblend{
		.blendEnable		 = false,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.colorBlendOp		 = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.alphaBlendOp		 = VK_BLEND_OP_ADD,
		.colorWriteMask =
			VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
	};
	std::vector<VkDynamicState> _dynstate;
};

}  // namespace xray::rendering
