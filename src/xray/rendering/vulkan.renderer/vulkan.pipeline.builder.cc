#include "xray/rendering/vulkan.renderer/vulkan.pipeline.builder.hpp"

#include <span>
#include <vector>
#include <unordered_map>
#include <memory_resource>

#include <fmt/core.h>
#include <itlib/small_vector.hpp>
#include <tl/optional.hpp>
#include <swl/variant.hpp>
#include <mio/mmap.hpp>
#include <Lz/Lz.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_to_string.hpp>
#include <shaderc/shaderc.hpp>
#include <spirv_reflect.h>

#include "xray/base/logger.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/rangeless/fn.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

using namespace std;
namespace fn = rangeless::fn;
using fn::operators::operator%;
using fn::operators::operator%=;

template<typename T>
using small_vec_2 = itlib::small_vector<T, 2>;

template<typename T>
using small_vec_4 = itlib::small_vector<T, 4>;

namespace xray::rendering {

struct ShaderTraits
{
    VkShaderStageFlagBits stage;
    shaderc_shader_kind kind;
};

tl::optional<ShaderTraits>
shader_traits_from_shader_file(const std::filesystem::path& shader_file)
{
    assert(shader_file.has_extension());

    if (shader_file.extension() == ".vert")
        return ShaderTraits{ VK_SHADER_STAGE_VERTEX_BIT, shaderc_shader_kind::shaderc_vertex_shader };

    if (shader_file.extension() == ".geom")
        return ShaderTraits{ VK_SHADER_STAGE_GEOMETRY_BIT, shaderc_shader_kind::shaderc_geometry_shader };

    if (shader_file.extension() == ".frag")
        return ShaderTraits{ VK_SHADER_STAGE_FRAGMENT_BIT, shaderc_shader_kind::shaderc_fragment_shader };

    return tl::nullopt;
}

tl::optional<ShaderTraits>
shader_traits_from_vk_stage(const VkShaderStageFlagBits vks)
{
    switch (vks) {
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            return ShaderTraits{ vks, shaderc_shader_kind::shaderc_fragment_shader };
            break;

        case VK_SHADER_STAGE_VERTEX_BIT:
            return ShaderTraits{ vks, shaderc_shader_kind::shaderc_vertex_shader };
            break;

        case VK_SHADER_STAGE_GEOMETRY_BIT:
            return ShaderTraits{ vks, shaderc_shader_kind::shaderc_geometry_shader };
            break;

        default:
            break;
    }

    return tl::nullopt;
}

struct ShaderModuleWithSpirVBlob
{
    xrUniqueVkShaderModule module;
    vector<uint32_t> spirv;
};

tl::optional<ShaderModuleWithSpirVBlob>
create_shader_module_from_string(VkDevice device,
                                 const std::string_view source_code,
                                 const char* shader_tag,
                                 const ShaderTraits shader_traits,
                                 const bool optimize)
{
    shaderc::CompileOptions compile_opts{};
    compile_opts.SetGenerateDebugInfo();
    compile_opts.SetOptimizationLevel(optimize ? shaderc_optimization_level::shaderc_optimization_level_size
                                               : shaderc_optimization_level::shaderc_optimization_level_zero);

    const shaderc::SpvCompilationResult compilation_result{ shaderc::Compiler{}.CompileGlslToSpv(
        source_code.data(), source_code.size(), shader_traits.kind, shader_tag) };

    if (compilation_result.GetCompilationStatus() != shaderc_compilation_status_success) {
        XR_LOG_CRITICAL("{}", compilation_result.GetErrorMessage());
        XR_LOG_INFO("Dumping shader code:\n{}", source_code);
        return tl::nullopt;
    }

    const VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = (static_cast<size_t>(compilation_result.cend() - compilation_result.cbegin())) * 4,
        .pCode = compilation_result.cbegin()
    };

    VkShaderModule shader_module{};
    WRAP_VULKAN_FUNC(vkCreateShaderModule, device, &shader_module_create_info, nullptr, &shader_module);

    return tl::make_optional<ShaderModuleWithSpirVBlob>(
        xrUniqueVkShaderModule{
            shader_module,
            VkResourceDeleter_VkShaderModule{ device },
        },
        vector<uint32_t>{
            compilation_result.cbegin(),
            compilation_result.cend(),
        });
}

tl::optional<ShaderModuleWithSpirVBlob>
create_shader_module_from_file(VkDevice device, const std::filesystem::path& fs_path, const bool optimize)
{
    const auto shader_traits = shader_traits_from_shader_file(fs_path);
    if (!shader_traits) {
        XR_LOG_ERR("Cannot determine shader stage base on file extension: {}", fs_path.generic_string());
        return {};
    }

    const auto path_gs{ fs_path.generic_string() };

    std::error_code err_code{};
    const mio::mmap_source shader_file{ mio::make_mmap_source(path_gs.c_str(), err_code) };
    if (err_code) {
        XR_LOG_ERR("Failed to read shader file {}, error {:#x} - {}", path_gs, err_code.value(), err_code.message());
        return {};
    }

    return create_shader_module_from_string(
        device, string_view{ shader_file.data(), shader_file.size() }, path_gs.c_str(), *shader_traits, optimize);
}

struct SpirVReflectionResult
{
    std::unordered_map<uint32_t, vector<VkDescriptorSetLayoutBinding>> descriptor_sets_layout_bindings;
    std::vector<VkPushConstantRange> push_constants;
    tl::optional<std::pair<uint32_t, vector<VkVertexInputAttributeDescription>>> vertex_inputs;
};

tl::optional<SpirVReflectionResult>
parse_spirv_binary(VkDevice device, const span<const uint32_t> spirv_binary)
{
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

    small_vec_4<SpvReflectDescriptorSet*> descriptor_sets{ descriptor_sets_count };
    if (spvReflectEnumerateDescriptorSets(&shader_module.GetShaderModule(),
                                          &descriptor_sets_count,
                                          descriptor_sets.data()) != SPV_REFLECT_RESULT_SUCCESS) {
        XR_LOG_ERR("SPIR-V reflect error: failed to enumerate descriptor sets!");
        return tl::nullopt;
    }

    unordered_map<uint32_t, vector<VkDescriptorSetLayoutBinding>> dsets;

    for (uint32_t idx = 0; idx < descriptor_sets_count; ++idx) {
        const SpvReflectDescriptorSet* reflected_set = descriptor_sets[idx];

        vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings{};
        descriptor_set_layout_bindings.reserve(reflected_set->binding_count);

        for (uint32_t binding_idx = 0; binding_idx < reflected_set->binding_count; ++binding_idx) {
            const SpvReflectDescriptorBinding* reflected_binding = reflected_set->bindings[binding_idx];

            const VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
                .binding = reflected_binding->binding,
                .descriptorType = static_cast<VkDescriptorType>(reflected_binding->descriptor_type),
                .descriptorCount = span{ reflected_binding->array.dims,
                                         reflected_binding->array.dims + reflected_binding->array.dims_count } %
                                   fn::foldl(1u, [](uint32_t a, const uint32_t i) { return a * i; }),
                .stageFlags = VK_SHADER_STAGE_ALL,
                .pImmutableSamplers = nullptr,
            };

            descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
        }

        dsets.emplace(idx, std::move(descriptor_set_layout_bindings));
    }

    uint32_t push_constants_count{};
    spvReflectEnumeratePushConstantBlocks(&shader_module.GetShaderModule(), &push_constants_count, nullptr);

    small_vec_4<SpvReflectBlockVariable*> push_constants{ push_constants_count };
    spvReflectEnumeratePushConstantBlocks(
        &shader_module.GetShaderModule(), &push_constants_count, push_constants.data());

    vector<VkPushConstantRange> push_constant_ranges{};

    for (uint32_t idx = 0; idx < push_constants_count; ++idx) {
        const SpvReflectBlockVariable* reflect_push_const = push_constants[idx];
        XR_LOG_INFO("Push constant {}, size = {}, padded size {}, stage = {}",
                    reflect_push_const->name ? reflect_push_const->name : "unnamed",
                    reflect_push_const->size,
                    reflect_push_const->padded_size,
                    vk::to_string(static_cast<vk::ShaderStageFlags>(shader_module.GetShaderStage())));
        push_constant_ranges.emplace_back(static_cast<VkShaderStageFlagBits>(shader_module.GetShaderStage()),
                                          reflect_push_const->offset,
                                          reflect_push_const->size);
    }

    if (shader_module.GetShaderStage() == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
        uint32_t input_vars_count{};
        spvReflectEnumerateInputVariables(&shader_module.GetShaderModule(), &input_vars_count, nullptr);

        small_vec_4<SpvReflectInterfaceVariable*> input_vars{};
        input_vars.resize(input_vars_count);
        spvReflectEnumerateInputVariables(&shader_module.GetShaderModule(), &input_vars_count, input_vars.data());

        vector<VkVertexInputAttributeDescription> input_attribute_descriptions;

        for (uint32_t idx = 0; idx < input_vars_count; ++idx) {
            const SpvReflectInterfaceVariable* reflected_variable = input_vars[idx];

            if (reflected_variable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
                continue;

            if (reflected_variable->storage_class != SpvStorageClassInput) {
                continue;
            }

            input_attribute_descriptions.emplace_back(reflected_variable->location,
                                                      0, // binding
                                                      static_cast<VkFormat>(reflected_variable->format),
                                                      0);

            XR_LOG_INFO("reflected variable: {}, type", reflected_variable->name);
        }

        ranges::sort(input_attribute_descriptions,
                     [](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) {
                         return a.location < b.location;
                     });

        uint32_t stride{};

        for (VkVertexInputAttributeDescription& attr : input_attribute_descriptions) {
            uint32_t vk_format_bytes_size(const VkFormat format);
            attr.offset = stride;
            stride += vk_format_bytes_size(attr.format);
        }

        const VkVertexInputBindingDescription vertex_input_binding_description = {
            .binding = 0, .stride = stride, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };

        const VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_input_binding_description,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(input_attribute_descriptions.size()),
            .pVertexAttributeDescriptions = input_attribute_descriptions.data(),
        };

        return tl::make_optional<SpirVReflectionResult>(
            std::move(dsets),
            std::move(push_constant_ranges),
            tl::optional<std::pair<uint32_t, vector<VkVertexInputAttributeDescription>>>{
                std::pair{ stride, std::move(input_attribute_descriptions) } });
    } else {
        return tl::make_optional<SpirVReflectionResult>(std::move(dsets), std::move(push_constant_ranges), tl::nullopt);
    }
}

tl::optional<GraphicsPipeline>
GraphicsPipelineBuilder::create(const VulkanRenderer& renderer)
{
    VkDevice device = renderer.device();

    if (!_stage_modules.contains(ShaderStage::Vertex)) {
        XR_LOG_CRITICAL("Missing vertex shader stage!");
        return tl::nullopt;
    }

    small_vec_4<ShaderModuleWithSpirVBlob> shader_modules;
    vector<VkPipelineShaderStageCreateInfo> shader_stage_create_info;

    for (const pair<uint32_t, ShaderModuleSource>& e : _stage_modules) {
        const auto& [shader_stage, shader_source] = e;
        tl::optional<ShaderModuleWithSpirVBlob> shader_with_spirv{
            [s = &shader_source, shader_stage, device, shader_opt = _optimize_shaders]() {
                if (const std::string_view* sv = swl::get_if<std::string_view>(s)) {

                    return create_shader_module_from_string(
                        device,
                        *sv,
                        "string_view_shader",
                        *shader_traits_from_vk_stage(static_cast<VkShaderStageFlagBits>(shader_stage)),
                        shader_opt);
                } else {
                    return create_shader_module_from_file(device, *swl::get_if<filesystem::path>(s), false);
                }
            }()
        };

        if (!shader_with_spirv) {
            XR_LOG_CRITICAL("Shader module failure, cannot proceed with pipeline creation");
            return tl::nullopt;
        }

        shader_stage_create_info.emplace_back(VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                              nullptr,
                                              0,
                                              static_cast<VkShaderStageFlagBits>(shader_stage),
                                              base::raw_ptr(shader_with_spirv->module),
                                              "main",
                                              nullptr);

        shader_with_spirv.take().map(
            [&shader_modules](ShaderModuleWithSpirVBlob sm) { shader_modules.emplace_back(std::move(sm)); });
    }

    assert(_stage_modules.size() == shader_modules.size());

    vector<xrUniqueVkDescriptorSetLayout> descriptor_set_layouts{};
    vector<VkPushConstantRange> push_constant_ranges{};
    tl::optional<pair<uint32_t, vector<VkVertexInputAttributeDescription>>> vertex_input_state;

    for (const ShaderModuleWithSpirVBlob& smb : shader_modules) {
        tl::optional<SpirVReflectionResult> reflect_result{ parse_spirv_binary(device, smb.spirv) };
        if (!reflect_result) {
            XR_LOG_CRITICAL("Failed to reflect SPIR-V binary!");
            return tl::nullopt;
        }

        reflect_result.take()
            .and_then([&descriptor_set_layouts, device, &push_constant_ranges](SpirVReflectionResult reflection) {
                for (const pair<uint32_t, vector<VkDescriptorSetLayoutBinding>>& set_with_bindings :
                     reflection.descriptor_sets_layout_bindings) {

                    const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                        .pNext = nullptr,
                        .flags = 0,
                        .bindingCount = static_cast<uint32_t>(set_with_bindings.second.size()),
                        .pBindings = set_with_bindings.second.data()
                    };

                    VkDescriptorSetLayout set_layout{};
                    WRAP_VULKAN_FUNC(
                        vkCreateDescriptorSetLayout, device, &descriptor_set_layout_create_info, nullptr, &set_layout);
                    descriptor_set_layouts.emplace_back(set_layout, VkResourceDeleter_VkDescriptorSetLayout{ device });
                }

                push_constant_ranges.insert(
                    push_constant_ranges.end(), cbegin(reflection.push_constants), cend(reflection.push_constants));

                return reflection.vertex_inputs;
            })
            .map([&vertex_input_state](pair<uint32_t, vector<VkVertexInputAttributeDescription>> ia) {
                assert(!vertex_input_state.has_value());

                vertex_input_state = std::move(ia);
            });
    }

    std::pmr::monotonic_buffer_resource buffer_resource{ 4096 };
    std::pmr::polymorphic_allocator<char> palloc{ &buffer_resource };
    std::pmr::string dbg_str{ palloc };

    XRAY_SCOPE_EXIT
    {
        dbg_str.append(1, '\0');
        XR_LOG_INFO("Pipeline creation info:\n{}", dbg_str);
    };

    fmt::format_to(std::back_inserter(dbg_str), "Vertex input description {} = {{\n", vertex_input_state->first);
    for (const VkVertexInputAttributeDescription& vtx_desc : vertex_input_state->second) {
        fmt::format_to(std::back_inserter(dbg_str),
                       "\n={{\n\t.location = {}\n\t.format = {}\n\t.offset = {}\n\t.binding = {}\n}},",
                       vtx_desc.location,
                       vk::to_string(static_cast<vk::Format>(vtx_desc.format)),
                       vtx_desc.offset,
                       vtx_desc.binding);
    }

    dbg_str.append("\n}\n");

    xrUniqueVkPipelineLayout pipeline_layout{
        [&]() {
            const small_vec_4<VkDescriptorSetLayout> dsl =
                lz::chain(descriptor_set_layouts)
                    .map([](const xrUniqueVkDescriptorSetLayout& layout) { return base::raw_ptr(layout); })
                    .to<small_vec_4<VkDescriptorSetLayout>>();

            const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .setLayoutCount = static_cast<uint32_t>(dsl.size()),
                .pSetLayouts = dsl.data(),
                .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
                .pPushConstantRanges = push_constant_ranges.data()
            };

            VkPipelineLayout pipeline_layout{};
            WRAP_VULKAN_FUNC(vkCreatePipelineLayout, device, &pipeline_layout_create_info, nullptr, &pipeline_layout);

            return pipeline_layout;
        }(),
        VkResourceDeleter_VkPipelineLayout{ device },
    };

    if (!pipeline_layout) {
        XR_LOG_CRITICAL("Failed to create pipeline layout!");
        return tl::nullopt;
    }

    assert(vertex_input_state.has_value());

    const VkVertexInputBindingDescription vertex_input_binding_description = {
        .binding = 0,
        .stride = vertex_input_state->first,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    const VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_state->second.empty() ? 0 : 1),
        .pVertexBindingDescriptions = vertex_input_state->second.empty() ? nullptr : &vertex_input_binding_description,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_state->second.size()),
        .pVertexAttributeDescriptions =
            vertex_input_state->second.empty() ? nullptr : vertex_input_state->second.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = _input_assembly.topology,
        .primitiveRestartEnable = _input_assembly.restart_enabled
    };

    {
        const auto ia = &input_assembly_create_info;
        fmt::format_to(std::back_inserter(dbg_str),
                       "InputAssembly = {{\n\t.topology = {}\n\t.primitiveRestartEnabled = {}\n}}\n",
                       vk::to_string(static_cast<vk::PrimitiveTopology>(ia->topology)),
                       ia->primitiveRestartEnable ? "true" : "false");
    }

    const VkViewport dummy_viewport = {
        .x = 0,
        .y = 0,
        .width = 64,
        .height = 64,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D dummy_scissor = {};

    const VkPipelineViewportStateCreateInfo viewport_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &dummy_viewport,
        .scissorCount = 1,
        .pScissors = &dummy_scissor
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = false,
        .rasterizerDiscardEnable = false,
        .polygonMode = _raster.poly_mode,
        .cullMode = _raster.cull_mode,
        .frontFace = _raster.front_face,
        .depthBiasEnable = false,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = _raster.line_width
    };

    {
        const auto r = &rasterization_state_create_info;
        fmt::format_to(std::back_inserter(dbg_str),
                       "Rasterizer state = {{\n"
                       "\t.depthClampEnable = {}\n"
                       "\t.rasterizerDiscardEnable = {}\n"
                       "\t.polygonMode = {}\n"
                       "\t.cullMode = {}\n"
                       "\t.frontFace = {}\n"
                       "}}\n",
                       r->depthClampEnable,
                       r->rasterizerDiscardEnable,
                       vk::to_string(static_cast<vk::PolygonMode>(r->polygonMode)),
                       vk::to_string(static_cast<vk::CullModeFlags>(r->cullMode)),
                       vk::to_string(static_cast<vk::FrontFace>(r->frontFace))

        );
    }

    const VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = false,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = false,
        .alphaToOneEnable = false
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = _depth_stencil.depth_test_enable,
        .depthWriteEnable = _depth_stencil.depth_write_enable,
        .depthCompareOp = _depth_stencil.depth_op,
        .depthBoundsTestEnable = false,
        .stencilTestEnable = false,
        .front = {},
        .back = {},
        .minDepthBounds = _depth_stencil.min_depth,
        .maxDepthBounds = _depth_stencil.max_depth
    };

    {
        const auto d = &depth_stencil_create_info;
        fmt::format_to(std::back_inserter(dbg_str),
                       "Depth stencil state = {{\n"
                       "\t.depthBoundsTestEnable = {}\n"
                       "\t.depthWriteEnable = {}\n"
                       "\t.depthCompareOp = {}\n"
                       "\t.minDepthBounds = {}\n"
                       "\t.maxDepthBounds = {}\n"
                       "}}\n",
                       d->depthBoundsTestEnable,
                       d->depthWriteEnable,
                       vk::to_string(static_cast<vk::CompareOp>(d->depthCompareOp)),
                       d->minDepthBounds,
                       d->maxDepthBounds);
    }

    const VkPipelineColorBlendStateCreateInfo colorblend_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = false,
        .logicOp = VK_LOGIC_OP_CLEAR,
        .attachmentCount = 1,
        .pAttachments = &_colorblend,
        .blendConstants = { 1.0f, 1.0f, 1.0f, 1.0f }

    };

    {
        const auto cb = &colorblend_state_create_info;
        fmt::format_to(std::back_inserter(dbg_str),
                       "ColorblendStateCreateInfo = {{\n\t.logicOpEnable = {}\n\t.logicOp = {}\n\t.attachmentCount = "
                       "{}\n\t.pAttachments = {{\n",
                       cb->logicOpEnable ? "true" : "false",
                       vk::to_string(static_cast<vk::LogicOp>(cb->logicOp)),
                       cb->attachmentCount);

        for (uint32_t i = 0; i < cb->attachmentCount; ++i) {
            const auto* a = cb->pAttachments + i;
            fmt::format_to(std::back_inserter(dbg_str),
                           "\t\t{{ .blendEnable = {}"
                           "\n\t\t .srcColorBlendFactor = {}"
                           "\n\t\t .dstColorBlendFactor = {}"
                           "\n\t\t .colorBlendOp = {}"
                           "\n\t\t .srcAlphaBlendFactor = {}"
                           "\n\t\t .dstAlphaBlendFactor = {}"
                           "\n\t\t .alphaBlendOp = {}"
                           "\n\t\t .colorWriteMask = {}\n\t\t}},\n",
                           a->blendEnable ? "true" : "false",
                           vk::to_string(static_cast<vk::BlendFactor>(a->srcColorBlendFactor)),
                           vk::to_string(static_cast<vk::BlendFactor>(a->dstColorBlendFactor)),
                           vk::to_string(static_cast<vk::BlendOp>(a->colorBlendOp)),
                           vk::to_string(static_cast<vk::BlendFactor>(a->srcAlphaBlendFactor)),
                           vk::to_string(static_cast<vk::BlendFactor>(a->dstAlphaBlendFactor)),
                           vk::to_string(static_cast<vk::BlendOp>(a->alphaBlendOp)),
                           a->colorWriteMask);
        }

        dbg_str.append("}\n");
    }

    const VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(_dynstate.size()),
        .pDynamicStates = _dynstate.empty() ? nullptr : _dynstate.data()
    };

    {
        dbg_str.append("DynamicState = {\n");
        for (const VkDynamicState ds : _dynstate) {
            fmt::format_to(std::back_inserter(dbg_str), "\t{},\n", vk::to_string(static_cast<vk::DynamicState>(ds)));
        }
        dbg_str.append("}\n");
    }

    const auto [view_mask, color_attachments, depth_attachment, stencil_attachment] =
        renderer.pipeline_render_create_info();

    const VkPipelineRenderingCreateInfo pipeline_render_create_inf = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = nullptr,
        .viewMask = view_mask,
        .colorAttachmentCount = static_cast<uint32_t>(color_attachments.size()),
        .pColorAttachmentFormats = color_attachments.data(),
        .depthAttachmentFormat = depth_attachment,
        .stencilAttachmentFormat = stencil_attachment,
    };

    {
        const auto r = &pipeline_render_create_inf;
        fmt::format_to(std::back_inserter(dbg_str),
                       "PipelineRenderingCreateInfo = {{\n\t.viewMask = {},\n\t.colorAttactmentCount = "
                       "{},\n\t.pColorAttachmentFormats = {{",
                       r->viewMask,
                       r->colorAttachmentCount);

        for (const VkFormat fmt : color_attachments) {
            fmt::format_to(std::back_inserter(dbg_str), "\n\t\t{},", vk::to_string(static_cast<vk::Format>(fmt)));
        }
        fmt::format_to(std::back_inserter(dbg_str),
                       "\n\t}},\n\t.depthAtrtachmentFormat = {},\n\t.stencilAttachmentFormat = {},\n}}\n",
                       vk::to_string(static_cast<vk::Format>(r->depthAttachmentFormat)),
                       vk::to_string(static_cast<vk::Format>(r->stencilAttachmentFormat)));
    }

    const VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &pipeline_render_create_inf,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(shader_stage_create_info.size()),
        .pStages = shader_stage_create_info.data(),
        .pVertexInputState = &pipeline_vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_create_info,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pDepthStencilState = &depth_stencil_create_info,
        .pColorBlendState = &colorblend_state_create_info,
        .pDynamicState = &dynamic_state_create_info,
        .layout = base::raw_ptr(pipeline_layout),
        .renderPass = VK_NULL_HANDLE,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0
    };

    xrUniqueVkPipeline pipeline{
        [&]() {
            VkPipeline pipeline{};
            WRAP_VULKAN_FUNC(
                vkCreateGraphicsPipelines, device, nullptr, 1, &graphics_pipeline_create_info, nullptr, &pipeline);
            return pipeline;
        }(),
        VkResourceDeleter_VkPipeline{ device },
    };

    return tl::make_optional<GraphicsPipeline>(
        std::move(pipeline), std::move(pipeline_layout), std::move(descriptor_set_layouts));
}

} // namespace xray::rendering
