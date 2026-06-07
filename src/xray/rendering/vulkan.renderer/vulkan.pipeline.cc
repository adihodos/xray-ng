#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"

#include <span>
#include <vector>
#include <unordered_map>

#include <tl/optional.hpp>
#include <swl/variant.hpp>
#include <mio/mio.hpp>

#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

#include "xray/base/logger.hpp"
#include "xray/base/xray.fmt.hpp"
#include "xray/base/xray.fmt.arena.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/containers/arena.string.hpp"
#include "xray/base/containers/arena.unorderered_map.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/variant.helpers.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

XR_DISABLE_OPTIMIZATIONS()

using namespace std;

namespace xray::rendering {

struct ShaderTraits {
	VkShaderStageFlagBits stage;
	shaderc_shader_kind kind;
};

tl::optional<ShaderTraits> shader_traits_from_shader_file(const std::filesystem::path& shader_file) {
	assert(shader_file.has_extension());

	if (shader_file.extension() == ".vert")
		return ShaderTraits{VK_SHADER_STAGE_VERTEX_BIT, shaderc_shader_kind::shaderc_vertex_shader};

	if (shader_file.extension() == ".geom")
		return ShaderTraits{VK_SHADER_STAGE_GEOMETRY_BIT, shaderc_shader_kind::shaderc_geometry_shader};

	if (shader_file.extension() == ".frag")
		return ShaderTraits{VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_shader_kind::shaderc_fragment_shader};

	if (shader_file.extension() == ".cs")
		return ShaderTraits{VK_SHADER_STAGE_COMPUTE_BIT, shaderc_shader_kind::shaderc_compute_shader};

	XR_LOG_CRITICAL("Unable to determine shader type from file extension %s", shader_file.generic_string().c_str());
	return tl::nullopt;
}

tl::optional<ShaderTraits> shader_traits_from_vk_stage(const VkShaderStageFlagBits vks) {
	switch (vks) {
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return ShaderTraits{vks, shaderc_shader_kind::shaderc_fragment_shader};
			break;

		case VK_SHADER_STAGE_VERTEX_BIT:
			return ShaderTraits{vks, shaderc_shader_kind::shaderc_vertex_shader};
			break;

		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return ShaderTraits{vks, shaderc_shader_kind::shaderc_geometry_shader};
			break;

		case VK_SHADER_STAGE_COMPUTE_BIT:
			return ShaderTraits{vks, shaderc_shader_kind::shaderc_compute_shader};
			break;

		default:
			XR_LOG_CRITICAL("Unable to determine shader type {}", std::to_underlying(vks));
			break;
	}

	return tl::nullopt;
}

struct ShaderModuleWithSpirVBlob {
	xrUniqueVkShaderModule module;
	xray::base::containers::vector<uint32_t> spirv;
};

struct IncludedShaderSource {
	shaderc_include_result include_result;
	mio::mmap_source mapped_file;
	std::string path;
};

class ShaderIncludesResolver : public shaderc::CompileOptions::IncluderInterface {
public:
	ShaderIncludesResolver() = default;
	ShaderIncludesResolver(std::span<const std::filesystem::path> include_dirs) : _include_dirs{include_dirs} {}

	virtual shaderc_include_result* GetInclude(
		const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth
	) override;

	virtual void ReleaseInclude(shaderc_include_result* data) override;

	static shaderc_include_result fail_include(const std::string_view err_msg) {
		return shaderc_include_result{
			.source_name		= "",
			.source_name_length = 0,
			.content			= err_msg.data(),
			.content_length		= err_msg.length(),
			.user_data			= nullptr,
		};
	}

private:
	std::span<const std::filesystem::path> _include_dirs;
	shaderc_include_result _include_result{};
	std::unordered_map<std::string, IncludedShaderSource> _resolved_includes;
};

shaderc_include_result* ShaderIncludesResolver::GetInclude(
	const char* requested_source, shaderc_include_type type, const char* requesting_source, size_t include_depth
) {
	XR_LOG_TRACE(
		"Include request: source %s, type %u, requesting src %s, depth %zu",
		requested_source,
		static_cast<uint32_t>(type),
		requesting_source,
		include_depth
	);

	if (const auto itr_entry = _resolved_includes.find(requested_source); itr_entry != std::cend(_resolved_includes)) {
		return &itr_entry->second.include_result;
	}

	namespace fs = std::filesystem;

	// const tl::expected<fs::path, std::string_view> included_file_path =
	//     [](const char* requested_source, const char* requesting_source) -> tl::expected<fs::path, std::string_view> {
	//     const fs::path requested_src_path{ requested_source };
	//
	//     if (!requested_src_path.has_parent_path()) {
	//         //
	//         // Files included from the same directory as the one being pre-processed
	//         // #include "same_dir_file.glsl"
	//         const fs::path requesting_src_path{ requesting_source };
	//         assert(requesting_src_path.has_parent_path());
	//         return fs::path{ requesting_src_path.parent_path() / requested_source };
	//     } else {
	//         //
	//         // Files includes from other directories.
	//         // #include "core/pbr.common.glsl"
	//         // #include "vertex_format/vertex.pntt.glsl"
	//         const std::string_view start_path{ requesting_source };
	//         const auto cut_pos = start_path.find("shaders");
	//         if (cut_pos == std::string_view::npos) {
	//             return tl::unexpected{
	//                 "Shader folder structure needs to be vulkan/shaders/project/shader.[frag|vert|tess|comp]"sv
	//             };
	//         }
	//
	//         fs::path included_file_path{ start_path.substr(0, cut_pos) };
	//         included_file_path /= "shaders";
	//         included_file_path /= requested_source;
	//
	//         return included_file_path;
	//     }
	// }(requested_source, requesting_source);

	const tl::optional<fs::path> included_file_path =
		[this](const char* requested_file, const char* requesting_source) -> tl::optional<fs::path> {
		const fs::path requested_src_path{requested_file};

		if (!requested_src_path.has_parent_path()) {
			//
			// Files included from the same directory as the one being pre-processed
			// #include "same_dir_file.glsl"
			const fs::path requesting_src_path{requesting_source};
			assert(requesting_src_path.has_parent_path());
			return fs::path{requesting_src_path.parent_path() / requested_file};
		}

		for (const std::filesystem::path& header_loc : _include_dirs) {
			const std::filesystem::path p{header_loc / requested_file};
			if (std::filesystem::exists(p)) {
				return tl::optional<fs::path>{p};
			}
		}

		return tl::nullopt;
	}(requested_source, requesting_source);

	if (!included_file_path) {
		_include_result = ShaderIncludesResolver::fail_include("included shader not found!");
		return &_include_result;
	}

	std::error_code err_code{};
	mio::mmap_source mapped_file{mio::make_mmap_source(included_file_path->generic_string(), err_code)};
	if (err_code) {
		XR_LOG_CRITICAL(
			"Failed to mmap file %s, error %s", included_file_path->generic_string().c_str(), err_code.message().c_str()
		);
		_include_result = ShaderIncludesResolver::fail_include("mmap failure");
		return &_include_result;
	}

	auto [itr_entry, was_inserted] = _resolved_includes.try_emplace(
		requested_source,
		IncludedShaderSource{shaderc_include_result{}, std::move(mapped_file), included_file_path->generic_string()}
	);

	assert(was_inserted);

	itr_entry->second.include_result = shaderc_include_result{
		.source_name		= itr_entry->second.path.c_str(),
		.source_name_length = itr_entry->second.path.length(),
		.content			= itr_entry->second.mapped_file.data(),
		.content_length		= itr_entry->second.mapped_file.length(),
	};

	XR_LOG_TRACE("shader includer resolved %s to %s", requested_source, itr_entry->second.path.c_str());
	return &itr_entry->second.include_result;
}

void ShaderIncludesResolver::ReleaseInclude(shaderc_include_result*) {
	//
	// nothing to do, everything is released when this object is destroyed
}

struct ShaderModuleCreateParams {
	VkDevice device;
	std::span<const std::filesystem::path> shader_include_dirs;
	std::string_view source_code;
	std::string_view shader_tag;
	ShaderTraits shader_traits;
	const ShaderBuildOptions* build_options;
	xray::base::MemoryArena* arena_perm;
};

tl::optional<ShaderModuleWithSpirVBlob> create_shader_module_from_string(const ShaderModuleCreateParams& params) {
	shaderc::CompileOptions compile_opts{};
	compile_opts.SetGenerateDebugInfo();
	compile_opts.SetIncluder(std::make_unique<ShaderIncludesResolver>(params.shader_include_dirs));
	compile_opts.SetOptimizationLevel(
		params.build_options->compile_options & ShaderBuildOptions::Compile_EnabledOptimizations
			? shaderc_optimization_level::shaderc_optimization_level_size
			: shaderc_optimization_level::shaderc_optimization_level_zero
	);
	if (params.build_options->compile_options & ShaderBuildOptions::Compile_GenerateDebugInfo) {
		compile_opts.SetGenerateDebugInfo();
	}
	if (params.build_options->compile_options & ShaderBuildOptions::Compile_WarningsToErrors) {
		compile_opts.SetWarningsAsErrors();
	}
	if (params.build_options->compile_options & ShaderBuildOptions::Compile_SuppressWarnings) {
		compile_opts.SetSuppressWarnings();
	}

	for (const auto& [macro_name, macro_val] : params.build_options->defines) {
		compile_opts.AddMacroDefinition(
			macro_name.data(),
			macro_name.size(),
			macro_val.empty() ? nullptr : macro_val.data(),
			macro_val.empty() ? 0 : macro_val.size()
		);
	}

	shaderc::Compiler compiler{};
	const shaderc::PreprocessedSourceCompilationResult preprocessed_result{compiler.PreprocessGlsl(
		params.source_code.data(),
		params.source_code.size(),
		params.shader_traits.kind,
		params.shader_tag.data(),
		compile_opts
	)};

	if (preprocessed_result.GetCompilationStatus() != shaderc_compilation_status_success) {
		XR_LOG_CRITICAL(
			"Shader preprocess error: %s\nShader code:\n%s",
			preprocessed_result.GetErrorMessage().c_str(),
			params.source_code.data()
		);
		return tl::nullopt;
	}

	if (params.build_options->compile_options & ShaderBuildOptions::Compile_DumpShaderCode) {
		XR_LOG_DEBUG("Shader code:\n%s", preprocessed_result.cbegin());
	}

	const shaderc::SpvCompilationResult compilation_result{
		params.build_options->entry_point.empty() ? shaderc::Compiler{}.CompileGlslToSpv(
														preprocessed_result.cbegin(),
														preprocessed_result.cend() - preprocessed_result.cbegin(),
														params.shader_traits.kind,
														params.shader_tag.data()
													)
												  :

												  shaderc::Compiler{}.CompileGlslToSpv(
													  preprocessed_result.cbegin(),
													  preprocessed_result.cend() - preprocessed_result.cbegin(),
													  params.shader_traits.kind,
													  params.shader_tag.data(),
													  params.build_options->entry_point.data(),
													  compile_opts
												  )
	};

	if (compilation_result.GetCompilationStatus() != shaderc_compilation_status_success) {
		XR_LOG_CRITICAL("%s", compilation_result.GetErrorMessage().c_str());
		XR_LOG_INFO("Dumping shader code:\n%s", params.source_code.data());
		return tl::nullopt;
	}

	const VkShaderModuleCreateInfo shader_module_create_info = {
		.sType	  = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext	  = nullptr,
		.flags	  = 0,
		.codeSize = (static_cast<size_t>(compilation_result.cend() - compilation_result.cbegin())) * 4,
		.pCode	  = compilation_result.cbegin()
	};

	VkShaderModule shader_module{};
	vkCreateShaderModule(params.device, &shader_module_create_info, nullptr, &shader_module);

	return tl::make_optional<ShaderModuleWithSpirVBlob>(
		xrUniqueVkShaderModule{
			shader_module,
			VkResourceDeleter_VkShaderModule{params.device},
		},
		xray::base::containers::vector<uint32_t>{
			compilation_result.cbegin(),
			compilation_result.cend(),
			*params.arena_perm,
		}
	);
}

struct ShaderModuleCreateFromFileParams {
	VkDevice device;
	std::filesystem::path file_path;
	std::span<const std::filesystem::path> shader_include_dirs;
	ShaderTraits shader_traits;
	const ShaderBuildOptions* build_options;
	xray::base::MemoryArena* arena_perm;
};

tl::optional<ShaderModuleWithSpirVBlob> create_shader_module_from_file(const ShaderModuleCreateFromFileParams& params) {
	const auto path_gs{params.file_path.generic_string()};

	std::error_code err_code{};
	const mio::mmap_source shader_file{mio::make_mmap_source(path_gs.c_str(), err_code)};
	if (err_code) {
		XR_LOG_ERR(
			"Failed to read shader file %s, error %#08x - %s",
			path_gs.c_str(),
			err_code.value(),
			err_code.message().c_str()
		);
		return {};
	}

	return create_shader_module_from_string(
		ShaderModuleCreateParams{
			.device				 = params.device,
			.shader_include_dirs = params.shader_include_dirs,
			.source_code		 = string_view{shader_file.data(), shader_file.size()},
			.shader_tag			 = path_gs,
			.shader_traits		 = params.shader_traits,
			.build_options		 = params.build_options,
			.arena_perm			 = params.arena_perm,
		}
	);
}

struct SpirVReflectionResult {
	xray::base::containers::unordered_map<uint32_t, xray::base::containers::vector<VkDescriptorSetLayoutBinding>>
		descriptor_sets_layout_bindings;
	xray::base::containers::vector<VkPushConstantRange> push_constants;
	tl::optional<std::pair<uint32_t, xray::base::containers::vector<VkVertexInputAttributeDescription>>> vertex_inputs;
};

tl::optional<SpirVReflectionResult> parse_spirv_binary(
	VkDevice device, const span<const uint32_t> spirv_binary, base::MemoryArena* arena_perm
) {
	spv_reflect::ShaderModule shader_module{
		spirv_binary.size() * 4,
		static_cast<const void*>(spirv_binary.data()),
		SPV_REFLECT_MODULE_FLAG_NO_COPY,
	};

	const SpvReflectResult reflect_result = shader_module.GetResult();
	if (reflect_result != SPV_REFLECT_RESULT_SUCCESS) {
		XR_LOG_ERR("Failed to reflect shader module");
		return tl::nullopt;
	}

	uint32_t descriptor_sets_count{};
	if (spvReflectEnumerateDescriptorSets(&shader_module.GetShaderModule(), &descriptor_sets_count, nullptr) !=
		SPV_REFLECT_RESULT_SUCCESS) {
		XR_LOG_ERR("SPIR-V reflect error: failed to enumerate descriptor sets!");
		return tl::nullopt;
	}

	base::ScratchPadArena scratchPad = base::ThreadLocalContext::acquire_scratchpad({arena_perm});
	base::containers::vector<SpvReflectDescriptorSet*> descriptor_sets{descriptor_sets_count, scratchPad};
	if (spvReflectEnumerateDescriptorSets(
			&shader_module.GetShaderModule(), &descriptor_sets_count, descriptor_sets.data()
		) != SPV_REFLECT_RESULT_SUCCESS) {
		XR_LOG_ERR("SPIR-V reflect error: failed to enumerate descriptor sets!");
		return tl::nullopt;
	}

	base::containers::unordered_map<uint32_t, base::containers::vector<VkDescriptorSetLayoutBinding>> dsets{
		*arena_perm,
	};

	for (uint32_t idx = 0; idx < descriptor_sets_count; ++idx) {
		const SpvReflectDescriptorSet* reflected_set = descriptor_sets[idx];

		base::containers::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings{*arena_perm};
		descriptor_set_layout_bindings.reserve(reflected_set->binding_count);

		for (uint32_t binding_idx = 0; binding_idx < reflected_set->binding_count; ++binding_idx) {
			const SpvReflectDescriptorBinding* reflected_binding = reflected_set->bindings[binding_idx];
			if (reflected_binding->accessed == 0) {
				continue;
			}

			const uint32_t descriptor_count = [reflected_binding]() {
				uint32_t descriptor_count = 0;
				if (reflected_binding->array.dims_count != 0) {
					for (uint32_t idx = 0; idx < reflected_binding->array.dims_count; ++idx) {
						descriptor_count += reflected_binding->array.dims[idx];
					}

					return descriptor_count == 0 ? 0xFFFFFFFFu : descriptor_count;
				} else {
					return reflected_binding->count;
				}
			}();

			const VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
				.binding			= reflected_binding->binding,
				.descriptorType		= static_cast<VkDescriptorType>(reflected_binding->descriptor_type),
				.descriptorCount	= descriptor_count,
				.stageFlags			= static_cast<VkShaderStageFlags>(shader_module.GetShaderStage()),
				.pImmutableSamplers = nullptr,
			};

			descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
		}

		dsets.emplace(reflected_set->set, std::move(descriptor_set_layout_bindings));
	}

	uint32_t push_constants_count{};
	spvReflectEnumeratePushConstantBlocks(&shader_module.GetShaderModule(), &push_constants_count, nullptr);

	base::containers::vector<SpvReflectBlockVariable*> push_constants{push_constants_count, scratchPad};
	spvReflectEnumeratePushConstantBlocks(
		&shader_module.GetShaderModule(), &push_constants_count, push_constants.data()
	);

	base::containers::vector<VkPushConstantRange> push_constant_ranges{*arena_perm};

	for (uint32_t idx = 0; idx < push_constants_count; ++idx) {
		const SpvReflectBlockVariable* reflect_push_const = push_constants[idx];
		XR_LOG_INFO(
			"Push constant %s, size = %u, padded size %u, stage = %u, offset = %u, word offset = %u, absolute "
			"offset = %u",
			reflect_push_const->name ? reflect_push_const->name : "unnamed",
			reflect_push_const->size,
			reflect_push_const->padded_size,
			(U32)shader_module.GetShaderStage(),
			reflect_push_const->offset,
			reflect_push_const->word_offset.offset,
			reflect_push_const->absolute_offset
		);

		push_constant_ranges.emplace_back(
			// do I force VK_SHADER_STAGE_ALL here to simplify logic ??
			// static_cast<VkShaderStageFlagBits>(shader_module.GetShaderStage()),
			VK_SHADER_STAGE_ALL,
			reflect_push_const->offset,
			reflect_push_const->size
		);
	}

	if (shader_module.GetShaderStage() & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
		uint32_t input_vars_count{};
		spvReflectEnumerateInputVariables(&shader_module.GetShaderModule(), &input_vars_count, nullptr);

		base::containers::vector<SpvReflectInterfaceVariable*> input_vars{scratchPad};
		input_vars.resize(input_vars_count);
		spvReflectEnumerateInputVariables(&shader_module.GetShaderModule(), &input_vars_count, input_vars.data());

		base::containers::vector<VkVertexInputAttributeDescription> input_attribute_descriptions{*arena_perm};

		for (uint32_t idx = 0; idx < input_vars_count; ++idx) {
			const SpvReflectInterfaceVariable* reflected_variable = input_vars[idx];

			if (reflected_variable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) continue;

			if (reflected_variable->storage_class != SpvStorageClassInput) {
				continue;
			}

			input_attribute_descriptions.emplace_back(
				reflected_variable->location,
				0,	// binding
				static_cast<VkFormat>(reflected_variable->format),
				0
			);

			XR_LOG_INFO("reflected variable: %s, type", reflected_variable->name);
		}

		std::sort(
			input_attribute_descriptions.begin(),
			input_attribute_descriptions.end(),
			[](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) {
				return a.location < b.location;
			}
		);

		uint32_t stride{};

		for (VkVertexInputAttributeDescription& attr : input_attribute_descriptions) {
			uint32_t vk_format_bytes_size(const VkFormat format);
			attr.offset = stride;
			stride += vk_format_bytes_size(attr.format);
		}

		return tl::make_optional<SpirVReflectionResult>(
			std::move(dsets),
			std::move(push_constant_ranges),
			tl::optional<std::pair<uint32_t, base::containers::vector<VkVertexInputAttributeDescription>>>{
				std::pair{stride, std::move(input_attribute_descriptions)}
			}
		);
	} else {
		return tl::make_optional<SpirVReflectionResult>(std::move(dsets), std::move(push_constant_ranges), tl::nullopt);
	}
}

VulkanPipelineBuilder& VulkanPipelineBuilder::input_state(
	const std::span<const VertexInputAttributeDescriptor> vtx_input_atts
) {
	std::unordered_map<uint32_t, uint32_t> bindings{};

	static constexpr const VkFormat vulkan_formats[][8]{
		//
		// single component
		VK_FORMAT_R8_SINT,
		VK_FORMAT_R8_UINT,
		VK_FORMAT_R16_SINT,
		VK_FORMAT_R16_UINT,
		VK_FORMAT_R32_SINT,
		VK_FORMAT_R32_UINT,
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_UNDEFINED,

		// 2 components
		VK_FORMAT_R8G8_SINT,
		VK_FORMAT_R8G8_UINT,
		VK_FORMAT_R16G16_SINT,
		VK_FORMAT_R16G16_UINT,
		VK_FORMAT_R32G32_SINT,
		VK_FORMAT_R32G32_UINT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_UNDEFINED,

		// 3 components
		VK_FORMAT_R8G8B8_SINT,
		VK_FORMAT_R8G8B8_UINT,
		VK_FORMAT_R16G16B16_SINT,
		VK_FORMAT_R16G16B16_UINT,
		VK_FORMAT_R32G32B32_SINT,
		VK_FORMAT_R32G32B32_UINT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_UNDEFINED,

		// 4 components
		VK_FORMAT_R8G8B8A8_SINT,
		VK_FORMAT_R8G8B8A8_UINT,
		VK_FORMAT_R16G16B16A16_SINT,
		VK_FORMAT_R16G16B16A16_UINT,
		VK_FORMAT_R32G32B32A32_SINT,
		VK_FORMAT_R32G32B32A32_UINT,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_FORMAT_UNDEFINED,
	};

	assert(_vertex_attribute_description.empty() && "input state already set");
	assert(_vertex_binding_description.empty() && "input binding state already set");

	for (const VertexInputAttributeDescriptor& ad : vtx_input_atts) {
		assert(ad.component_type * ad.component_count < std::size(vulkan_formats));
		_vertex_attribute_description.emplace_back(
			ad.location, ad.binding, vulkan_formats[ad.component_count][ad.component_type], ad.component_offset
		);
		bindings[ad.binding] += ad.component_count * component_type_size(ad.component_type);
	}

	for (const auto [binding_index, binding_stride] : bindings) {
		_vertex_binding_description.emplace_back(binding_index, binding_stride, VK_VERTEX_INPUT_RATE_VERTEX);
	}

	return *this;
}

tl::expected<VulkanPipeline, VulkanError> VulkanPipelineBuilder::create(
	const VulkanRenderer& renderer,
	const VulkanPipelineKind pipeline_kind,
	const swl::variant<VulkanPipelineCreateData, VulkanPipelineTemplate> create_data
) {
	VkDevice device = renderer.device();

	base::ScratchPadArena scratchpad{_arena_perm};

	if (pipeline_kind == VulkanPipelineKind::Graphics && !_stage_modules.contains(ShaderStage::Vertex)) {
		XR_LOG_CRITICAL("Missing vertex shader stage!");
		return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
	}

	base::containers::vector<ShaderModuleWithSpirVBlob> shader_modules{scratchpad};
	shader_modules.reserve(_stage_modules.size());

	base::containers::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_info{scratchpad};
	shader_stage_create_info.reserve(_stage_modules.size());

	const std::initializer_list<uint32_t> shader_stages_graphics{
		static_cast<uint32_t>(VK_SHADER_STAGE_VERTEX_BIT),
		static_cast<uint32_t>(VK_SHADER_STAGE_GEOMETRY_BIT),
		static_cast<uint32_t>(VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	const std::initializer_list<uint32_t> shader_stages_compute{
		static_cast<uint32_t>(VK_SHADER_STAGE_COMPUTE_BIT),
	};

	//
	// build and reflect shader modules
	for (const uint32_t stage :
		 pipeline_kind == VulkanPipelineKind::Graphics ? shader_stages_graphics : shader_stages_compute) {
		if (!_stage_modules.contains(stage)) continue;

		const auto& shader_source = _stage_modules[stage];
		tl::optional<ShaderModuleWithSpirVBlob> shader_with_spirv{
			[r = &renderer, s = &shader_source, stage, device, this]() {
				if (const std::string_view* sv = swl::get_if<std::string_view>(&s->code_or_file_path)) {
					return create_shader_module_from_string(
						ShaderModuleCreateParams{
							.device				 = device,
							.shader_include_dirs = r->shader_include_directories(),
							.source_code		 = *sv,
							.shader_tag			 = "string_view_shader",
							.shader_traits = *shader_traits_from_vk_stage(static_cast<VkShaderStageFlagBits>(stage)),
							.build_options = s,
							.arena_perm	   = _arena_perm,
						}
					);
				} else {
					return create_shader_module_from_file(
						ShaderModuleCreateFromFileParams{
							.device				 = device,
							.file_path			 = *swl::get_if<filesystem::path>(&s->code_or_file_path),
							.shader_include_dirs = r->shader_include_directories(),
							.shader_traits = *shader_traits_from_vk_stage(static_cast<VkShaderStageFlagBits>(stage)),
							.build_options = s,
							.arena_perm	   = _arena_perm,
						}
					);
				}
			}()
		};

		if (!shader_with_spirv) {
			XR_LOG_CRITICAL("Shader module failure, cannot proceed with pipeline creation");
			return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
		}

		shader_stage_create_info.emplace_back(
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			nullptr,
			0,
			static_cast<VkShaderStageFlagBits>(stage),
			base::raw_ptr(shader_with_spirv->module),
			"main",
			nullptr
		);

		shader_with_spirv.take().map([&shader_modules](ShaderModuleWithSpirVBlob sm) {
			shader_modules.emplace_back(std::move(sm));
		});
	}

	assert(_stage_modules.size() == shader_modules.size());

	//
	// element stride, vertex attributes collection
	tl::optional<pair<uint32_t, base::containers::vector<VkVertexInputAttributeDescription>>> vertex_input_state;

	auto pipeline_layout = [&, p_tag = _tag_name]() -> tl::expected<VulkanPipeline::pipeline_layout_t, VulkanError> {
		//
		// reflect all compiled shader modules and extract info
		base::containers::vector<SpirVReflectionResult> reflected_shaders{*_arena_perm};
		reflected_shaders.reserve(shader_modules.size());

		for (const ShaderModuleWithSpirVBlob& smb : shader_modules) {
			tl::optional<SpirVReflectionResult> reflect_result{parse_spirv_binary(device, smb.spirv, _arena_perm)};
			if (!reflect_result) {
				XR_LOG_CRITICAL("Failed to reflect SPIR-V binary!");
				return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
			}

			reflected_shaders.push_back(std::move(*reflect_result.take()));
		}

		//
		// yoink the vertex inputs from the vertex shader
		if (pipeline_kind == VulkanPipelineKind::Graphics) {
			for (SpirVReflectionResult& reflection : reflected_shaders) {
				if (!reflection.vertex_inputs) continue;

				//
				// if this was a vertex shader reflection object extract the vertex inputs
				reflection.vertex_inputs.take().map(
					[&vertex_input_state](
						pair<uint32_t, base::containers::vector<VkVertexInputAttributeDescription>> ia
					) { vertex_input_state = std::move(ia); }
				);
			}
		}

		if (const VulkanPipelineTemplate* pipeline_template = swl::get_if<VulkanPipelineTemplate>(&create_data)) {
			return VulkanPipeline::BindlessLayout{
				pipeline_template->layout,
				pipeline_template->descriptor_set_layouts,
			};
		}

		using pipeline_layout_definition_table_t =
			base::containers::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>;

		base::containers::vector<VkPushConstantRange> push_constant_ranges{*_arena_perm};
		pipeline_layout_definition_table_t pipeline_layout_deftable{*_arena_perm};

		const VulkanPipelineCreateData pcd = swl::visit(
			base::VariantVisitor{
				[](const VulkanPipelineCreateData& pcd) { return pcd; },
				[](const VulkanPipelineTemplate p_template) {
					//
					// should not reach here ...
					return VulkanPipelineCreateData{};
				},
			},
			create_data
		);

		for (SpirVReflectionResult& reflection : reflected_shaders) {
			//
			// does not handle multiple bindings for the same set
			for (const auto& [set_id, set_bindings] : reflection.descriptor_sets_layout_bindings) {
				const VkDescriptorSetLayoutBinding& first_binding = set_bindings[0];

				const uint32_t fixed_descriptor_count = [&]() {
					//
					// unsized resource arrays will have a descriptorCount set to 0xFFFFFFFFu,
					// they will be set to the limit passed in here
					uint32_t descriptor_count = 1;
					switch (first_binding.descriptorType) {
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
						case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
							descriptor_count = pcd.uniform_descriptors;
							break;

						case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
							descriptor_count = pcd.storage_buffer_descriptors;
							break;

						case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
							descriptor_count = pcd.combined_image_sampler_descriptors;
							break;

						case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
						case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
							descriptor_count = pcd.image_descriptors;
							break;

						default:
							XR_LOG_ERR(
								"Descriptor type %d is unsized array but there is no size specified for it in the "
								"pipeline creation data. Defaulting to 1.",
								std::to_underlying(first_binding.descriptorType)
							);
							break;
					}

					return descriptor_count;
				}();

				if (auto set_itr = pipeline_layout_deftable.find(set_id); set_itr != end(pipeline_layout_deftable)) {
					if (set_itr->second.descriptorType == set_bindings[0].descriptorType) {
						set_itr->second.stageFlags |= set_bindings[0].stageFlags;
						set_itr->second.descriptorCount =
							std::max(set_itr->second.descriptorCount, fixed_descriptor_count);
					} else {
						XR_LOG_ERR(
							"Sets alias the same slot %u but the descriptor types are not compatibile (%u vs. %u)",
							set_id,
							static_cast<uint32_t>(set_itr->second.descriptorType),
							static_cast<uint32_t>(set_bindings[0].descriptorType)
						);
					}
				} else {
					pipeline_layout_deftable[set_id]				 = set_bindings[0];
					pipeline_layout_deftable[set_id].descriptorCount = fixed_descriptor_count;
				}
			}

			push_constant_ranges = std::move(reflection.push_constants);
		}

		XR_LOG_INFO("Definition table: %zu", pipeline_layout_deftable.size());

		// for (const pair<const U32, VkDescriptorSetLayoutBinding>& set_with_binding : pipeline_layout_deftable) {
		// 	XR_LOG_INFO(
		// 		"Set %u, type %s, count %u, stage %s ",
		// 		set_with_binding.first,
		// 		vk::to_string(static_cast<vk::DescriptorType>(set_with_binding.second.descriptorType)).c_str(),
		// 		(uint32_t)set_with_binding.second.descriptorCount,
		// 		vk::to_string(static_cast<vk::ShaderStageFlags>(set_with_binding.second.stageFlags)).c_str()
		// 	);
		// }

		U32 max_set_id = 0;
		for (const pair<const U32, VkDescriptorSetLayoutBinding>& dslb : pipeline_layout_deftable) {
			max_set_id = std::max(max_set_id, dslb.first);
		}

		vector<VkDescriptorSetLayout> desc_set_layouts{};
		for (U32 set_id = 0; set_id < max_set_id + 1; ++set_id) {
			const VkDescriptorSetLayout set_layout = [device, p_tag, t = &pipeline_layout_deftable, r = &renderer](
														 const uint32_t set_id
													 ) -> VkDescriptorSetLayout {
				if (auto itr_set = t->find(set_id); itr_set != end(*t)) {
					const VkDescriptorBindingFlags binding_flags{VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};
					const VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{
						.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
						.pNext		   = nullptr,
						.bindingCount  = 1,
						.pBindingFlags = &binding_flags,
					};

					const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
						.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						.pNext		  = &binding_flags_create_info,
						.flags		  = 0,
						.bindingCount = 1,
						.pBindings	  = &itr_set->second,
					};

					VkDescriptorSetLayout set_layout{};

					vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, &set_layout);

					if (!p_tag.empty()) {
						char scratch_buffer[256];
						base::format_to_n(scratch_buffer, "p_{}_set_{}_layout", p_tag, set_id);
						r->dbg_set_object_name(set_layout, scratch_buffer);
					}

					return set_layout;
				} else {
					return nullptr;
				}
			}(set_id);
			desc_set_layouts.push_back(set_layout);
		}

		//
		// Plug descriptor set layout holes.
		// For example a vertex shader might have layout (set = 0, binding = ...)
		// and the fragment shader might have layout (set = 4, binding = ...)
		// For sets 1 to 3 we need to create a "null" descriptor set layout with no bindings
		// and assign it to those slots when creating the pipeline layout.
		if (std::find(desc_set_layouts.cbegin(), desc_set_layouts.cend(), nullptr) != desc_set_layouts.cend()) {
			const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
				.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext		  = nullptr,
				.flags		  = 0,
				.bindingCount = 0,
				.pBindings	  = nullptr,
			};

			VkDescriptorSetLayout set_layout{};

			vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, &set_layout);

			if (!set_layout) {
				return XR_MAKE_VULKAN_ERROR(VK_ERROR_UNKNOWN);
			}

			for (size_t idx = 0; idx < desc_set_layouts.size(); ++idx) {
				if (desc_set_layouts[idx]) continue;

				desc_set_layouts[idx] = set_layout;
			}
		}

		const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
			.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext					= nullptr,
			.flags					= 0,
			.setLayoutCount			= static_cast<uint32_t>(desc_set_layouts.size()),
			.pSetLayouts			= desc_set_layouts.empty() ? nullptr : desc_set_layouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
			.pPushConstantRanges	= push_constant_ranges.empty() ? nullptr : push_constant_ranges.data(),
		};

		VkPipelineLayout pipeline_layout{};
		const VkResult layout_create_res =
			vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &pipeline_layout);

		if (layout_create_res != VK_SUCCESS) {
			return XR_MAKE_VULKAN_ERROR(layout_create_res);
		}
		if (!p_tag.empty()) {
			char scratch_buffer[256];
			base::format_to_n(scratch_buffer, "p_{}_layout", p_tag);
			renderer.dbg_set_object_name(pipeline_layout, scratch_buffer);
		}

		return VulkanPipeline::OwnedLayout{pipeline_layout, std::move(desc_set_layouts)};
	}();

	if (!pipeline_layout) {
		return tl::unexpected{pipeline_layout.error()};
	}

	//
	// dump some debug info
	// std::pmr::monotonic_buffer_resource buffer_resource{ 4096 };
	// std::pmr::polymorphic_allocator<char> palloc{ &buffer_resource };
	// std::pmr::string dbg_str{ palloc };
	base::containers::basic_string<char> dbg_str{*_arena_perm};
	dbg_str.reserve(8192);

	XRAY_SCOPE_EXIT noexcept {
		dbg_str.append(1, '\0');
		XR_LOG_INFO(
			"Pipeline type %s - creation info:\n%s",
			dbg_str.c_str(),
			pipeline_kind == VulkanPipelineKind::Graphics ? "Graphics" : "Compute"
		);
	};

	if (pipeline_kind == VulkanPipelineKind::Graphics) {
		format_to(dbg_str, "Vertex input description (stride %u) = {{\n", vertex_input_state->first);

		for (const VkVertexInputAttributeDescription& vtx_desc : vertex_input_state->second) {
			format_to(
				dbg_str,
				"\n={{\n\t.location = %u\n\t.format = %u\n\t.offset = %u\n\t.binding = %u\n}},",
				vtx_desc.location,
				(U32)vtx_desc.format,
				vtx_desc.offset,
				vtx_desc.binding
			);
		}
		dbg_str.append("\n}\n");
	}

	if (pipeline_kind == VulkanPipelineKind::Compute) {
		const VkComputePipelineCreateInfo pipeline_create_info{
			.sType	= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.pNext	= nullptr,
			.flags	= VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT,
			.stage	= shader_stage_create_info[0],
			.layout = swl::visit(
				base::VariantVisitor{
					[](const VulkanPipeline::OwnedLayout& owl) { return owl.layout; },
					[](const VulkanPipeline::BindlessLayout& bl) { return bl.layout; },
				},
				*pipeline_layout
			),
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex	= 0,
		};

		VkPipeline compute_pipeline{};
		const VkResult pipeline_create_result =
			vkCreateComputePipelines(device, nullptr, 1, &pipeline_create_info, nullptr, &compute_pipeline);

		if (pipeline_create_result != VK_SUCCESS) {
			return XR_MAKE_VULKAN_ERROR(pipeline_create_result);
		}

		if (!_tag_name.empty()) {
			char scratch_buffer[256];
			base::format_to_n(scratch_buffer, "p_%s", _tag_name.data());
			renderer.dbg_set_object_name(compute_pipeline, scratch_buffer);
		}

		return VulkanPipeline{
			compute_pipeline,
			std::move(*pipeline_layout),
		};
	}

	const VkVertexInputBindingDescription vertex_input_binding_description = {
		.binding   = 0,
		.stride	   = vertex_input_state->first,
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};

	// TODO: xxyyzz
	const VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {
		.sType						   = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext						   = nullptr,
		.flags						   = 0,
		.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_state->second.empty() ? 0 : 1),
		.pVertexBindingDescriptions = vertex_input_state->second.empty() ? nullptr : &vertex_input_binding_description,
		.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_state->second.size()),
		.pVertexAttributeDescriptions =
			vertex_input_state->second.empty() ? nullptr : vertex_input_state->second.data(),
	};

	const VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext					= nullptr,
		.flags					= 0,
		.topology				= _input_assembly.topology,
		.primitiveRestartEnable = _input_assembly.restart_enabled
	};

	{
		const auto ia = &input_assembly_create_info;
		format_to(
			dbg_str,
			"InputAssembly = {{\n\t.topology = %u\n\t.primitiveRestartEnabled = %s\n}}\n",
			(U32)ia->topology,
			ia->primitiveRestartEnable ? "true" : "false"
		);
	}

	const VkViewport dummy_viewport = {
		.x		  = 0,
		.y		  = 0,
		.width	  = 64,
		.height	  = 64,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	const VkRect2D dummy_scissor = {};

	const VkPipelineViewportStateCreateInfo viewport_state_create_info = {
		.sType		   = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext		   = nullptr,
		.flags		   = 0,
		.viewportCount = 1,
		.pViewports	   = &dummy_viewport,
		.scissorCount  = 1,
		.pScissors	   = &dummy_scissor
	};

	const VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
		.sType					 = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext					 = nullptr,
		.flags					 = 0,
		.depthClampEnable		 = false,
		.rasterizerDiscardEnable = false,
		.polygonMode			 = _raster.poly_mode,
		.cullMode				 = _raster.cull_mode,
		.frontFace				 = _raster.front_face,
		.depthBiasEnable		 = false,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp			 = 0.0f,
		.depthBiasSlopeFactor	 = 0.0f,
		.lineWidth				 = _raster.line_width
	};

	const VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
		.sType				   = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext				   = nullptr,
		.flags				   = 0,
		.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable   = false,
		.minSampleShading	   = 0.0f,
		.pSampleMask		   = nullptr,
		.alphaToCoverageEnable = false,
		.alphaToOneEnable	   = false
	};

	const VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {
		.sType				   = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext				   = nullptr,
		.flags				   = 0,
		.depthTestEnable	   = _depth_stencil.depth_test_enable,
		.depthWriteEnable	   = _depth_stencil.depth_write_enable,
		.depthCompareOp		   = _depth_stencil.depth_op,
		.depthBoundsTestEnable = false,
		.stencilTestEnable	   = false,
		.front				   = {},
		.back				   = {},
		.minDepthBounds		   = _depth_stencil.min_depth,
		.maxDepthBounds		   = _depth_stencil.max_depth
	};

	const VkPipelineColorBlendStateCreateInfo colorblend_state_create_info = {
		.sType			 = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext			 = nullptr,
		.flags			 = 0,
		.logicOpEnable	 = false,
		.logicOp		 = VK_LOGIC_OP_CLEAR,
		.attachmentCount = 1,
		.pAttachments	 = &_colorblend,
		.blendConstants	 = {1.0f, 1.0f, 1.0f, 1.0f}

	};

	const VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
		.sType			   = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext			   = nullptr,
		.flags			   = 0,
		.dynamicStateCount = static_cast<uint32_t>(_dynstate.size()),
		.pDynamicStates	   = _dynstate.empty() ? nullptr : _dynstate.data()
	   };

	const auto [view_mask, color_attachments, depth_attachment, stencil_attachment] =
		renderer.pipeline_render_create_info();

	const VkPipelineRenderingCreateInfo pipeline_render_create_inf = {
		.sType					 = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.pNext					 = nullptr,
		.viewMask				 = view_mask,
		.colorAttachmentCount	 = static_cast<uint32_t>(color_attachments.size()),
		.pColorAttachmentFormats = color_attachments.data(),
		.depthAttachmentFormat	 = depth_attachment,
		.stencilAttachmentFormat = stencil_attachment,
	};

	const VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
		.sType				 = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext				 = &pipeline_render_create_inf,
		.flags				 = 0,
		.stageCount			 = static_cast<uint32_t>(shader_stage_create_info.size()),
		.pStages			 = shader_stage_create_info.data(),
		.pVertexInputState	 = &pipeline_vertex_input_state_create_info,
		.pInputAssemblyState = &input_assembly_create_info,
		.pTessellationState	 = nullptr,
		.pViewportState		 = &viewport_state_create_info,
		.pRasterizationState = &rasterization_state_create_info,
		.pMultisampleState	 = &multisample_state_create_info,
		.pDepthStencilState	 = &depth_stencil_create_info,
		.pColorBlendState	 = &colorblend_state_create_info,
		.pDynamicState		 = &dynamic_state_create_info,
		.layout				 = swl::visit(
			 base::VariantVisitor{
				 [](const VulkanPipeline::OwnedLayout& owl) { return owl.layout; },
				 [](const VulkanPipeline::BindlessLayout& bl) { return bl.layout; },
			 },
			 *pipeline_layout
		 ),
		.renderPass			= VK_NULL_HANDLE,
		.subpass			= 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex	= 0
	};

	VkPipeline pipeline{};
	const VkResult pipeline_create_result =
		vkCreateGraphicsPipelines(device, nullptr, 1, &graphics_pipeline_create_info, nullptr, &pipeline);

	if (pipeline_create_result != VK_SUCCESS) {
		return XR_MAKE_VULKAN_ERROR(pipeline_create_result);
	}

	if (!_tag_name.empty()) {
		char scratch_buffer[256];
		base::format_to_n(scratch_buffer, "p_%s", _tag_name.data());
		renderer.dbg_set_object_name(pipeline, scratch_buffer);
	}

	return VulkanPipeline{
		pipeline,
		std::move(*pipeline_layout),
	};
}

VulkanPipeline::~VulkanPipeline() {
	//
	// TODO: destroy resource
}

void VulkanPipeline::release_resources(VkDevice device, const VkAllocationCallbacks* alloc_cb) {
	using namespace xray::base;

	swl::visit(
		VariantVisitor{
			[device, alloc_cb](OwnedLayout& owned_layout) {
				for (VkDescriptorSetLayout dsl : owned_layout.set_layouts) {
					vkDestroyDescriptorSetLayout(device, dsl, alloc_cb);
					vkDestroyPipelineLayout(device, owned_layout.layout, alloc_cb);
				}
			},
			[](BindlessLayout&) {},
		},
		_layout
	);
}

std::span<const VkDescriptorSetLayout> VulkanPipeline::descriptor_sets_layouts() const noexcept {
	using namespace xray::base;

	return swl::visit(
		VariantVisitor{
			[](const OwnedLayout& owl) { return std::span{owl.set_layouts}; },
			[](const BindlessLayout& bl) { return std::span{bl.set_layouts}; },
		},
		_layout
	);
}

VkPipelineLayout VulkanPipeline::layout() const noexcept {
	using namespace xray::base;

	return swl::visit(
		VariantVisitor{
			[](const OwnedLayout& owl) { return owl.layout; },
			[](const BindlessLayout& bl) { return bl.layout; },
		},
		_layout
	);
}

}  // namespace xray::rendering
