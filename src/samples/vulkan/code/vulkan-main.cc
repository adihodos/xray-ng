//
// Copyright (c) Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// \file main.cc

#include "build.config.hpp"
#include "xray/xray.hpp"

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <sys/mman.h>
#include "xray/base/syscall_wrapper.hpp"
#else
#endif

#include <cstdint>
#include <filesystem>
#include <vector>

#include <tracy/Tracy.hpp>

#include <fmt/core.h>
#include <fmt/std.h>
#include <fmt/ranges.h>
#include <concurrencpp/concurrencpp.h>
#include <Lz/Lz.hpp>
#include <swl/variant.hpp>
#include <mio/mmap.hpp>
#include <rfl/json.hpp>
#include <rfl.hpp>
#include <itlib/small_vector.hpp>
#include <atomic_queue/atomic_queue.h>

#include <noise/noise.h>
#include <noise/noiseutils.h>

#include "xray/base/app_config.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/delegate.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/base/xray.misc.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/containers/arena.string.hpp"
#include "xray/base/containers/arena.unorderered_map.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/variant.helpers.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/colors/rgb_variants.hpp"
#include "xray/rendering/colors/color_cast_rgb_variants.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/geometry.importer.gltf.hpp"
#include "xray/rendering/geometry/heightmap.generator.hpp"
#include "xray/rendering/procedural.hpp"
#include "xray/rendering/vertex_format/vertex.format.pbr.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.queue.submit.token.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/user.interface.backend.hpp"
#include "xray/ui/user.interface.backend.vulkan.hpp"
#include "xray/ui/user_interface_render_context.hpp"
#include "xray/ui/window.hpp"

#include "xray/math/orientation.hpp"
#include "xray/scene/scene.description.hpp"
#include "xray/scene/scene.definition.hpp"

#include "demo_base.hpp"
#include "init_context.hpp"
#include "triangle/triangle.hpp"
#include "bindless.pipeline.config.hpp"

using namespace xray;
using namespace xray::base;
using namespace xray::math;
using namespace xray::ui;
using namespace xray::rendering;
using namespace xray::scene;

using namespace std;

xray::base::ConfigSystem* xr_app_config{ nullptr };

struct GlobalMemoryConfig
{
    uint32_t medium_arenas{ 8 };
    uint32_t large_arenas{ 4 };
    uint32_t small_arenas{ 16 };
    uint32_t large_arena_size{ 64 };
    uint32_t medium_arena_size{ 32 };
    uint32_t small_arena_size{ 16 };
};

namespace GlobalMemoryBuffers {
uint8_t SmallArenas[16][16 * 1024 * 1024];
uint8_t MediumArenas[8][32 * 1024 * 1024];
uint8_t LargeArenas[4][64 * 1024 * 1024];
};

struct LargeArenaTag
{};
struct MediumArenaTag
{};
struct SmallArenaTag
{};

template<typename TagType>
struct ScopedMemoryArena
{
    explicit ScopedMemoryArena(std::span<uint8_t> s) noexcept
        : arena{ s }
    {
    }

    ScopedMemoryArena(uint8_t* backing_buffer, size_t backing_buffer_length) noexcept
        : arena{ backing_buffer, backing_buffer_length }
    {
    }

    ~ScopedMemoryArena();

    MemoryArena arena;
};

using ScopedSmallArenaType = ScopedMemoryArena<SmallArenaTag>;
using ScopedMediumArenaType = ScopedMemoryArena<MediumArenaTag>;
using ScopedLargeArenaType = ScopedMemoryArena<LargeArenaTag>;

class GlobalMemorySystem
{
  private:
    GlobalMemorySystem()
    {
        for (size_t idx = 0; idx < std::size(GlobalMemoryBuffers::SmallArenas); ++idx) {
            free_small.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::SmallArenas[idx]));
        }

        for (size_t idx = 0; idx < std::size(GlobalMemoryBuffers::MediumArenas); ++idx) {
            free_medium.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::MediumArenas[idx]));
        }

        for (size_t idx = 0; idx < std::size(GlobalMemoryBuffers::LargeArenas); ++idx) {
            free_large.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::LargeArenas[idx]));
        }
    }

  public:
    static GlobalMemorySystem* instance() noexcept
    {
        static GlobalMemorySystem the_one_and_only{};
        return &the_one_and_only;
    }

    template<typename TagType>
    ScopedMemoryArena<TagType> grab_arena()
    {
        if constexpr (std::is_same_v<TagType, LargeArenaTag>) {
            void* mem_ptr{};
            if (!this->free_large.try_pop(mem_ptr)) {
                // TODO: handle memory exhausted
            }

            return ScopedMemoryArena<TagType>{
                static_cast<uint8_t*>(mem_ptr),
                std::size(GlobalMemoryBuffers::LargeArenas[0]),
            };
        } else if constexpr (std::is_same_v<TagType, MediumArenaTag>) {
            void* mem_ptr{};
            if (!free_medium.try_pop(mem_ptr)) {
                // TODO: handle memory exhausted
            }
            return ScopedMemoryArena<TagType>{
                static_cast<uint8_t*>(mem_ptr),
                std::size(GlobalMemoryBuffers::MediumArenas[0]),
            };
        } else if constexpr (std::is_same_v<TagType, SmallArenaTag>) {
            void* mem_ptr{};
            if (!free_small.try_pop(mem_ptr)) {
                // TODO: handle memory exhausted
            }
            return ScopedMemoryArena<TagType>{
                static_cast<uint8_t*>(mem_ptr),
                std::size(GlobalMemoryBuffers::SmallArenas[0]),
            };
        } else {
            static_assert(false, "Unhandled arena type");
        }
    }

    template<typename TagType>
    void release_arena(MemoryArena& arena)
    {
        if constexpr (std::is_same_v<TagType, LargeArenaTag>) {
            free_large.push(arena.buf);
        } else if constexpr (std::is_same_v<TagType, MediumArenaTag>) {
            free_medium.push(arena.buf);
        } else if constexpr (std::is_same_v<TagType, SmallArenaTag>) {
            free_small.push(arena.buf);
        } else {
            static_assert(false, "Unhandled arena type");
        }
    }

    auto grab_large_arena() noexcept { return grab_arena<LargeArenaTag>(); }
    auto grab_medium_arena() noexcept { return grab_arena<MediumArenaTag>(); }
    auto grab_small_arena() noexcept { return grab_arena<SmallArenaTag>(); }

    ~GlobalMemorySystem()
    {
        // for (size_t i = 0; i < std::size(_reserved_blocks); ++i)
        //     munmap(_reserved_blocks[i], 64 * 1024 * 1024);
    }

  private:
    atomic_queue::AtomicQueue<void*, 16> free_small;
    atomic_queue::AtomicQueue<void*, 16> free_medium;
    atomic_queue::AtomicQueue<void*, 16> free_large;

    GlobalMemorySystem(const GlobalMemorySystem&) = delete;
    GlobalMemorySystem& operator=(const GlobalMemorySystem&) = delete;
};

template<typename TagType>
ScopedMemoryArena<TagType>::~ScopedMemoryArena()
{
    GlobalMemorySystem::instance()->release_arena<TagType>(this->arena);
}

namespace app {

struct RegisteredDemo
{
    std::string_view short_desc;
    std::string_view detailed_desc;
    cpp::delegate<unique_pointer<DemoBase>(const init_context_t&)> create_demo_fn;
};

template<typename... Ts>
std::vector<RegisteredDemo>
register_demos()
{
    std::vector<RegisteredDemo> demos;
    demos.resize(sizeof...(Ts));
    ((demos.push_back(RegisteredDemo{ Ts::short_desc(), Ts::detailed_desc(), cpp::bind<&Ts::create>() })), ...);

    return demos;
}

enum class demo_type : int32_t
{
    none,
    colored_circle,
    fractal,
    texture_array,
    mesh,
    bufferless_draw,
    lighting_directional,
    lighting_point,
    procedural_city,
    instanced_drawing,
    geometric_shapes,
    // terrain_basic
};

template<typename T, size_t N = 4>
using SmallVec = itlib::small_vector<T, N>;

concurrencpp::result<FontsLoadBundle>
task_load_fonts(concurrencpp::executor_tag, concurrencpp::thread_executor*)
{
    XR_LOG_INFO("[[TASK]] Load fonts");
    timer_highp exec_timer{};
    exec_timer.start();

    namespace fs = std::filesystem;

    FontsLoadBundle font_pkgs;

    for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(xr_app_config->font_root())) {
        if (!dir_entry.is_regular_file() || !dir_entry.path().has_extension())
            continue;

        const fs::path file_ext{ dir_entry.path().extension() };
        if (file_ext != ".ttf" && file_ext != ".otf") {
            continue;
        }

        std::error_code err{};
        mio::mmap_source font_data{ mio::make_mmap_source(dir_entry.path().generic_string(), err) };
        if (err) {
            XR_LOG_INFO("Font file {} could not be loaded {}", dir_entry.path().generic_string(), err.message());
            continue;
        }

        font_pkgs.info.emplace_back(dir_entry.path(), 24.0f);
        font_pkgs.data.emplace_back(std::move(font_data));
    };

    exec_timer.end();
    XR_LOG_INFO("[[TASK]] Font load task done, time {}", exec_timer.elapsed_millis());

    co_return font_pkgs;
}

struct GeometryResourceTaskParams
{
    std::string_view resource_tag;
    concurrencpp::shared_result<VulkanRenderer*> renderer_result;
    std::span<const std::span<const uint8_t>> vertex_data;
    std::span<const std::span<const uint32_t>> index_data;
};

struct MiscError
{
    std::string what;
};

#define PROGRAM_ERRORS_LIST                                                                                            \
    PROGRAM_ERRORS_LIST_ENTRY(xray::rendering::VulkanError),                                                           \
        PROGRAM_ERRORS_LIST_ENTRY(xray::rendering::GeometryImportError),                                               \
        PROGRAM_ERRORS_LIST_ENTRY(xray::scene::SceneError), PROGRAM_ERRORS_LIST_ENTRY(MiscError)

#define PROGRAM_ERRORS_LIST_ENTRY(e) e
using ProgramError = swl::variant<PROGRAM_ERRORS_LIST>;

template<typename U, typename E>
auto
translate_error(const tl::expected<U, E>& exp) -> ProgramError
{
    assert(!exp);
    if constexpr (std::is_same_v<E, ProgramError>) {
        return exp.error();
    } else if constexpr (xray::base::is_swl_variant_v<E>) {
        return xray::base::variant_transform<PROGRAM_ERRORS_LIST>(exp.error());
    } else {
        return ProgramError{ exp.error() };
    }
}

#define XR_COR_PROPAGATE_ERROR(e)                                                                                      \
    do {                                                                                                               \
        if (!e) {                                                                                                      \
            co_return tl::make_unexpected(translate_error(e));                                                         \
        }                                                                                                              \
    } while (0)

#define XR_PROPAGATE_ERROR(e)                                                                                          \
    do {                                                                                                               \
        if (!e) {                                                                                                      \
            return tl::make_unexpected(translate_error(e));                                                            \
        }                                                                                                              \
    } while (0)

concurrencpp::result<tl::expected<GraphicsPipelineResources, VulkanError>>
task_create_graphics_pipelines(concurrencpp::executor_tag,
                               concurrencpp::thread_pool_executor*,
                               concurrencpp::shared_result<VulkanRenderer*> renderer_result)
{
    timer_highp exec_timer{};
    exec_timer.start();

    //
    // TODO: Pipeline cache maybe ?!
    VulkanRenderer* renderer = co_await renderer_result;

    ScopedMediumArenaType perm = GlobalMemorySystem::instance()->grab_medium_arena();
    ScopedMediumArenaType temp = GlobalMemorySystem::instance()->grab_medium_arena();

    //
    // graphics pipelines
    const std::pair<containers::string, containers::string> std_shader_defs[] = {
        { containers::string{ "__VERT_SHADER__", temp.arena }, containers::string{ temp.arena } },
        { containers::string{ "__FRAG_SHADER__", temp.arena }, containers::string{ temp.arena } },
    };
    tl::expected<GraphicsPipeline, VulkanError> p_ads_color{
        GraphicsPipelineBuilder{ &perm.arena, &temp.arena }
            .add_shader(ShaderStage::Vertex,
                        ShaderBuildOptions{
                            .code_or_file_path = xr_app_config->shader_path("triangle/ads.basic.glsl"),
                            .defines = to_cspan(std_shader_defs[0]),
                            .compile_options = ShaderBuildOptions::Compile_GenerateDebugInfo,
                        })
            .add_shader(ShaderStage::Fragment,
                        ShaderBuildOptions{
                            .code_or_file_path = xr_app_config->shader_path("triangle/ads.basic.glsl"),
                            .defines = to_cspan(std_shader_defs[1]),
                            .compile_options = ShaderBuildOptions::Compile_GenerateDebugInfo,
                        })
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .rasterization_state({
                .poly_mode = VK_POLYGON_MODE_FILL,
                .cull_mode = VK_CULL_MODE_BACK_BIT,
                .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .line_width = 1.0f,
            })
            .create_bindless(*renderer),
    };
    XR_VK_COR_PROPAGATE_ERROR(p_ads_color);

    const std::pair<containers::string, containers::string> textured_defs_vs[] = {
        { containers::string{ "__VERT_SHADER__", temp.arena }, containers::string{ temp.arena } },
        { containers::string{ "__ADS_TEXTURED__", temp.arena }, containers::string{ temp.arena } },
    };

    const std::pair<containers::string, containers::string> textured_defs_fs[] = {
        { containers::string{ "__FRAG_SHADER__", temp.arena }, containers::string{ temp.arena } },
        { containers::string{ "__ADS_TEXTURED__", temp.arena }, containers::string{ temp.arena } },
    };
    tl::expected<GraphicsPipeline, VulkanError> p_ads_textured{
        GraphicsPipelineBuilder{ &perm.arena, &temp.arena }
            .add_shader(ShaderStage::Vertex,
                        ShaderBuildOptions{
                            .code_or_file_path = xr_app_config->shader_path("triangle/ads.basic.glsl"),
                            .defines = textured_defs_vs,
                            .compile_options = ShaderBuildOptions::Compile_GenerateDebugInfo |
                                               ShaderBuildOptions::Compile_DumpShaderCode,
                        })
            .add_shader(ShaderStage::Fragment,
                        ShaderBuildOptions{
                            .code_or_file_path = xr_app_config->shader_path("triangle/ads.basic.glsl"),
                            .defines = textured_defs_fs,
                            .compile_options = ShaderBuildOptions::Compile_GenerateDebugInfo |
                                               ShaderBuildOptions::Compile_DumpShaderCode,
                        })
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .rasterization_state({
                .poly_mode = VK_POLYGON_MODE_FILL,
                .cull_mode = VK_CULL_MODE_BACK_BIT,
                .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                .line_width = 1.0f,
            })
            .create_bindless(*renderer),
    };
    XR_VK_COR_PROPAGATE_ERROR(p_ads_textured);

    tl::expected<GraphicsPipeline, VulkanError> p_pbr_color{
        GraphicsPipelineBuilder{ &perm.arena, &temp.arena }
            .add_shader(ShaderStage::Vertex,
                        ShaderBuildOptions{
                            .code_or_file_path = xr_app_config->shader_path("triangle/tri.vert"),
                            .defines = to_cspan(std_shader_defs[0]),
                            .compile_options = ShaderBuildOptions::Compile_GenerateDebugInfo |
                                               ShaderBuildOptions::Compile_DumpShaderCode,
                        })
            .add_shader(ShaderStage::Fragment,
                        ShaderBuildOptions{
                            .code_or_file_path = xr_app_config->shader_path("triangle/tri.vert"),
                            .defines = to_cspan(std_shader_defs[1]),
                            .compile_options = ShaderBuildOptions::Compile_GenerateDebugInfo |
                                               ShaderBuildOptions::Compile_DumpShaderCode,
                        })
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .rasterization_state({
                .poly_mode = VK_POLYGON_MODE_FILL,
                .cull_mode = VK_CULL_MODE_BACK_BIT,
                .front_face = VK_FRONT_FACE_CLOCKWISE,
                .line_width = 1.0f,
            })
            .create_bindless(*renderer),
    };
    XR_VK_COR_PROPAGATE_ERROR(p_pbr_color);

    exec_timer.end();
    XR_LOG_INFO("[[TASK]] Graphics pipeline done, time {}", exec_timer.elapsed_millis());

    co_return tl::expected<GraphicsPipelineResources, VulkanError>{
        tl::in_place,
        std::move(*p_ads_color),
        std::move(*p_ads_textured),
        std::move(*p_pbr_color),
    };
}

using GltfResourceTaskError = swl::variant<VulkanError, GeometryImportError>;

concurrencpp::result<tl::expected<tuple<GltfGeometry, GltfMaterialsData>, GltfResourceTaskError>>
task_create_gltf_resources(concurrencpp::executor_tag,
                           concurrencpp::thread_pool_executor*,
                           concurrencpp::shared_result<VulkanRenderer*> renderer_result,
                           const std::span<const scene::GltfGeometryDescription> gltf_geometry_defs)
{
    timer_highp exec_timer{};
    exec_timer.start();

    vector<GltfGeometryEntry> gltf_geometries;
    vector<VkDrawIndexedIndirectCommand> indirect_draw_templates;
    vector<LoadedGeometry> loaded_gltfs;
    vec2ui32 global_vertex_index_count{ vec2ui32::stdc::zero };
    vector<vec2ui32> per_obj_vertex_index_counts;
    vector<ExtractedMaterialsWithImageSourcesBundle> mtls_data;

    for (const GltfGeometryDescription& gltf : gltf_geometry_defs) {
        XR_LOG_INFO("Loading model {}", xr_app_config->model_path(gltf.path));
        auto gltf_geometry = xray::rendering::LoadedGeometry::from_file(xr_app_config->model_path(gltf.path));
        XR_VK_COR_PROPAGATE_ERROR(gltf_geometry);

        const vec2ui32 obj_vtx_idx_count = gltf_geometry->compute_vertex_index_count();
        indirect_draw_templates.push_back(VkDrawIndexedIndirectCommand{
            .indexCount = obj_vtx_idx_count.y,
            .instanceCount = 1,
            .firstIndex = global_vertex_index_count.y,
            .vertexOffset = (int32_t)global_vertex_index_count.x,
            .firstInstance = 0,
        });

        gltf_geometries.push_back(GltfGeometryEntry{
            .name = gltf.name,
            .hashed_name = GeometryHandleType{ FNV::fnv1a(gltf.name) },
            .vertex_index_count = obj_vtx_idx_count,
            .buffer_offsets = global_vertex_index_count,
        });

        mtls_data.push_back(gltf_geometry->extract_materials(0));
        loaded_gltfs.push_back(std::move(*gltf_geometry));
        global_vertex_index_count += obj_vtx_idx_count;
        per_obj_vertex_index_counts.push_back(obj_vtx_idx_count);
    }

    const vec2ui32 buffer_bytes =
        global_vertex_index_count * vec2ui32{ (uint32_t)sizeof(VertexPBR), (uint32_t)sizeof(uint32_t) };

    VulkanRenderer* renderer = co_await renderer_result;

    auto vertex_buffer =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = "[[gltf]] vertex buffer",
                                 .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 .bytes = buffer_bytes.x,
                             });
    XR_VK_COR_PROPAGATE_ERROR(vertex_buffer);

    auto index_buffer =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = "[[gltf]] index buffer",
                                 .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 .bytes = buffer_bytes.y,
                             });
    XR_VK_COR_PROPAGATE_ERROR(index_buffer);

    const uintptr_t copy_offset = renderer->reserve_staging_buffer_memory(buffer_bytes.x + buffer_bytes.y);
    uintptr_t staging_buffer_ptr = renderer->staging_buffer_memory() + copy_offset;

    SmallVec<VkBufferCopy> copy_regions_vertex;
    SmallVec<VkBufferCopy> copy_regions_index;
    vec2ui32 dst_offsets{ vec2ui32::stdc::zero };
    for (size_t idx = 0, count = loaded_gltfs.size(); idx < count; ++idx) {
        LoadedGeometry* g = &loaded_gltfs[idx];
        const vec2ui32* obj_cnt = &per_obj_vertex_index_counts[idx];

        g->extract_data(
            (void*)(staging_buffer_ptr), (void*)(staging_buffer_ptr + obj_cnt->x * sizeof(VertexPBR)), { 0, 0 }, 0);

        const vec2ui32 bytes_consumed =
            (*obj_cnt) * vec2ui32{ (uint32_t)sizeof(VertexPBR), (uint32_t)sizeof(uint32_t) };

        copy_regions_vertex.push_back(VkBufferCopy{
            .srcOffset = copy_offset + dst_offsets.x + dst_offsets.y,
            .dstOffset = dst_offsets.x,
            .size = bytes_consumed.x,
        });

        copy_regions_index.push_back(VkBufferCopy{
            .srcOffset = copy_offset + dst_offsets.x + dst_offsets.y + bytes_consumed.x,
            .dstOffset = dst_offsets.y,
            .size = bytes_consumed.y,
        });

        dst_offsets += bytes_consumed;
        staging_buffer_ptr += bytes_consumed.x + bytes_consumed.y;
    }

    auto cmd_buffer = renderer->create_job(QueueType::Transfer);
    XR_VK_COR_PROPAGATE_ERROR(cmd_buffer);

    vkCmdCopyBuffer(*cmd_buffer,
                    renderer->staging_buffer(),
                    vertex_buffer->buffer_handle(),
                    (uint32_t)copy_regions_vertex.size(),
                    copy_regions_vertex.data());
    vkCmdCopyBuffer(*cmd_buffer,
                    renderer->staging_buffer(),
                    index_buffer->buffer_handle(),
                    (uint32_t)copy_regions_index.size(),
                    copy_regions_index.data());

    auto buffers_submit = renderer->submit_job(*cmd_buffer, QueueType::Transfer);
    XR_VK_COR_PROPAGATE_ERROR(buffers_submit);

    //
    // materials (images + material definitions)
    vector<VulkanImage> images;
    vector<QueueSubmitWaitToken> img_wait_tokens;
    vector<PBRMaterialDefinition> pbr_materials;
    char scratch_buffer[1024];
    uint32_t total_image_count{};

    for (std::tuple<GltfGeometryEntry&, ExtractedMaterialsWithImageSourcesBundle&> zipped :
         lz::zip(gltf_geometries, mtls_data)) {

        auto& [this_gltf, this_gltf_materials] = zipped;

        for (size_t idx = 0, count = this_gltf_materials.image_sources.size(); idx < count; ++idx) {
            const ExtractedImageData* img_data = &this_gltf_materials.image_sources[idx];
            auto out = fmt::format_to_n(
                scratch_buffer, size(scratch_buffer), "Tex {}", this_gltf_materials.image_sources[idx].tag);
            *out.out = 0;

            //
            // submit each texture separatly
            auto transfer_job = renderer->create_job(QueueType ::Transfer);
            XR_VK_COR_PROPAGATE_ERROR(transfer_job);

            const VkFormat img_format = [](const ExtractedImageData& imgdata) {
                switch (imgdata.bits) {
                    case 8:
                        return VK_FORMAT_R8G8B8A8_UNORM;
                        break;
                    case 16:
                        return VK_FORMAT_R16G16B16A16_UNORM;
                        break;
                    case 32:
                        return VK_FORMAT_R32G32B32A32_UINT;
                        break;
                    default:
                        assert(false && "Unhandled");
                        return VK_FORMAT_R8G8B8A8_UNORM;
                        break;
                }
            }(*img_data);

            auto img = VulkanImage::from_memory(*renderer,
                                                VulkanImageCreateInfo{
                                                    .tag_name = scratch_buffer,
                                                    .wpkg = *transfer_job,
                                                    .type = VK_IMAGE_TYPE_2D,
                                                    .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                                                    .memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                    .format = img_format,
                                                    .width = img_data->width,
                                                    .height = img_data->height,
                                                    .pixels = { img_data->pixels },
                                                });

            XR_VK_COR_PROPAGATE_ERROR(img);
            images.push_back(std::move(*img));

            auto wait_token = renderer->submit_job(*transfer_job, QueueType ::Transfer);
            XR_VK_COR_PROPAGATE_ERROR(wait_token);
            img_wait_tokens.push_back(std::move(*wait_token));
        }

        //
        // set materials buffer offset for this GLTF
        this_gltf.materials_buffer.x = static_cast<uint32_t>(pbr_materials.size());

        lz::chain(this_gltf_materials.materials)
            .transformTo(back_inserter(pbr_materials), [total_image_count](const ExtractedMaterialDefinition& emdef) {
                return PBRMaterialDefinition{
                    .base_color_factor = emdef.base_color_factor,
                    .base_color = emdef.base_color + total_image_count,
                    .metallic = emdef.metallic + total_image_count,
                    .normal = emdef.normal + total_image_count,
                    .metallic_factor = emdef.metallic_factor,
                    .roughness_factor = emdef.roughness_factor,
                };
            });

        //
        // set materials count
        this_gltf.materials_buffer.y += static_cast<uint32_t>(pbr_materials.size() - this_gltf.materials_buffer.x);
        total_image_count += static_cast<uint32_t>(this_gltf_materials.image_sources.size());
    }

    //
    // adjust indices for the images in the material definitions
    const uint32_t image_slot_offset = renderer->bindless_sys().reserve_image_slots(total_image_count);
    for (PBRMaterialDefinition& pbr_mtl : pbr_materials) {
        pbr_mtl.normal += image_slot_offset;
        pbr_mtl.metallic += image_slot_offset;
        pbr_mtl.base_color += image_slot_offset;
    }

    //
    // PBR materials sbo
    auto sbo_transfer_job = renderer->create_job(QueueType ::Transfer);
    XR_VK_COR_PROPAGATE_ERROR(sbo_transfer_job);
    auto pbr_material_sbo =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = "[[gltf]] PBR materials SBO",
                                 .job_cmd_buf = *sbo_transfer_job,
                                 .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 .bytes = container_bytes_size(pbr_materials),
                                 .frames = 1,
                                 .initial_data = { to_bytes_span(pbr_materials) },
                             });
    XR_VK_COR_PROPAGATE_ERROR(pbr_material_sbo);
    auto sbo_wait_token = renderer->submit_job(*sbo_transfer_job, QueueType::Transfer);
    XR_VK_COR_PROPAGATE_ERROR(sbo_wait_token);

    exec_timer.end();
    XR_LOG_INFO("[[TASK]] - gltf geometry resources done, {} ms", exec_timer.elapsed_millis());

    co_return tl::expected<tuple<GltfGeometry, GltfMaterialsData>, VulkanError>{
        tl::in_place,
        GltfGeometry{
            std::move(gltf_geometries),
            std::move(indirect_draw_templates),
            std::move(*vertex_buffer),
            std::move(*index_buffer),
        },
        GltfMaterialsData{
            .images = std::move(images),
            .sbo_materials = std::move(*pbr_material_sbo),
            .reserved_image_slot_start = image_slot_offset,
        },
    };
}

struct ProceduralGeometryRenderResources
{
    VulkanBuffer vertexbuffer;
    VulkanBuffer indexbuffer;
};

concurrencpp::result<tl::expected<ProceduralGeometryRenderResources, ProgramError>>
task_create_procedural_geometry_render_resources(concurrencpp::executor_tag,
                                                 concurrencpp::thread_pool_executor*,
                                                 GeometryResourceTaskParams params)
{
    timer_highp exec_timer{};
    exec_timer.start();

    const size_t vertex_bytes =
        lz::chain(params.vertex_data).map([](const std::span<const uint8_t> sv) { return sv.size_bytes(); }).sum();
    const size_t index_bytes =
        lz::chain(params.index_data).map([](const std::span<const uint32_t> si) { return si.size_bytes(); }).sum();

    std::array<char, 256> scratch_buffer;
    auto out = fmt::format_to_n(
        scratch_buffer.data(), scratch_buffer.size() - 1, "[[{}]] - vertex buffer", params.resource_tag);
    *out.out = 0;

    VulkanRenderer* renderer = co_await params.renderer_result;

    auto vertexbuffer =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = scratch_buffer.data(),
                                 .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 .bytes = vertex_bytes,
                             });

    XR_COR_PROPAGATE_ERROR(vertexbuffer);

    out = fmt::format_to_n(
        scratch_buffer.data(), scratch_buffer.size() - 1, "[[{}]] - index buffer", params.resource_tag);
    *out.out = 0;

    auto indexbuffer =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = scratch_buffer.data(),
                                 .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 .bytes = index_bytes,
                             });

    XR_COR_PROPAGATE_ERROR(indexbuffer);

    uintptr_t staging_buff_offset = renderer->reserve_staging_buffer_memory(vertex_bytes + index_bytes);
    itlib::small_vector<VkBufferCopy> cpy_regions_vtx;

    uintptr_t dst_buff_offset = 0;
    for (const std::span<const uint8_t> sv : params.vertex_data) {
        memcpy(reinterpret_cast<void*>(renderer->staging_buffer_memory() + staging_buff_offset),
               sv.data(),
               sv.size_bytes());
        cpy_regions_vtx.push_back(VkBufferCopy{
            .srcOffset = staging_buff_offset,
            .dstOffset = dst_buff_offset,
            .size = sv.size_bytes(),
        });

        staging_buff_offset += sv.size_bytes();
        dst_buff_offset += sv.size_bytes();
    }

    const size_t index_copy_rgn = cpy_regions_vtx.size();

    uintptr_t idx_buffer_offset{};
    for (const std::span<const uint32_t> si : params.index_data) {
        memcpy(reinterpret_cast<void*>(renderer->staging_buffer_memory() + staging_buff_offset),
               si.data(),
               si.size_bytes());
        cpy_regions_vtx.push_back(VkBufferCopy{
            .srcOffset = staging_buff_offset,
            .dstOffset = idx_buffer_offset,
            .size = si.size_bytes(),
        });

        staging_buff_offset += si.size_bytes();
        idx_buffer_offset += si.size_bytes();
    }

    auto cmd_buf = renderer->create_job(QueueType::Transfer);
    XR_COR_PROPAGATE_ERROR(cmd_buf);

    vkCmdCopyBuffer(*cmd_buf,
                    renderer->staging_buffer(),
                    vertexbuffer->buffer_handle(),
                    (uint32_t)index_copy_rgn,
                    cpy_regions_vtx.data());
    vkCmdCopyBuffer(*cmd_buf,
                    renderer->staging_buffer(),
                    indexbuffer->buffer_handle(),
                    (uint32_t)cpy_regions_vtx.size() - index_copy_rgn,
                    &cpy_regions_vtx[index_copy_rgn]);

    auto wait_token = renderer->submit_job(*cmd_buf, QueueType::Transfer);
    XR_COR_PROPAGATE_ERROR(wait_token);

    exec_timer.end();
    XR_LOG_INFO("[[TASK]] - procedural geometry resources, {} ms", exec_timer.elapsed_millis());

    co_return tl::expected<ProceduralGeometryRenderResources, VulkanError>{
        tl::in_place,
        std::move(*vertexbuffer),
        std::move(*indexbuffer),
    };
}

template<typename T>
using ScratchPadVector = std::vector<T, MemoryArenaAllocator<T>>;

concurrencpp::result<tl::expected<NonGltfMaterialsData, ProgramError>>
task_create_non_gltf_materials(concurrencpp::executor_tag,
                               concurrencpp::thread_pool_executor*,
                               concurrencpp::shared_result<VulkanRenderer*> renderer_result,
                               const std::span<const scene::MaterialDescription> material_descriptions)
{
    using namespace xray::scene;

    timer_highp exec_timer{};
    exec_timer.start();

    ScopedSmallArenaType task_mem_arena = GlobalMemorySystem::instance()->grab_small_arena();

    ScratchPadVector<vec4f> color_pixels{ task_mem_arena.arena }; // store 1024 colors max
    vector<ColoredMaterial> colored_materials;
    ScratchPadVector<std::filesystem::path> texture_files{ task_mem_arena.arena };
    vector<TexturedMaterial> textured_materials;

    for (const MaterialDescription& md : material_descriptions) {
        rfl::visit(
            [&](const auto& s) {
                using Name = typename std::decay_t<decltype(s)>::Name;

                if constexpr (std::is_same<Name, rfl::Literal<"colored">>()) {
                    const auto [name, ambient_color, diffuse_color, specular_color] = s.value();
                    colored_materials.emplace_back(name, FNV::fnv1a(name), (uint32_t)color_pixels.size());
                    color_pixels.insert(color_pixels.end(), { ambient_color, diffuse_color, specular_color });
                } else if constexpr (std::is_same<Name, rfl::Literal<"textured">>()) {
                    const auto& [name, ambient_tex, diffuse_tex, specular_tex] = s.value();

                    auto add_material_to_collection_fn = [](ScratchPadVector<std::filesystem::path>& material_coll,
                                                            const std::filesystem::path& material) {
                        uint32_t material_id{};
                        //
                        // already inserted
                        if (auto itr = find(cbegin(material_coll), cend(material_coll), material);
                            itr != cend(material_coll)) {
                            material_id = static_cast<uint32_t>(distance(cbegin(material_coll), itr));
                        } else {
                            //
                            // not present, so add
                            material_id = static_cast<uint32_t>(material_coll.size());
                            material_coll.push_back(material);
                        }

                        return material_id;
                    };

                    const uint32_t ambient_mtl_id = add_material_to_collection_fn(texture_files, ambient_tex);
                    const uint32_t diffuse_mtl_id = add_material_to_collection_fn(texture_files, diffuse_tex);
                    const uint32_t specular_mtl_id = add_material_to_collection_fn(texture_files, specular_tex);

                    textured_materials.emplace_back(
                        name, FNV::fnv1a(name), ambient_mtl_id, diffuse_mtl_id, specular_mtl_id);

                } else {
                    static_assert(rfl::always_false_v<MaterialDescription>, "Not all cases were covered.");
                }
            },
            md);
    }

    if (color_pixels.size() != 32 * 32) {
        color_pixels.resize(32 * 32, vec4f{ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    VulkanRenderer* renderer = co_await renderer_result;
    ScratchPadVector<QueueSubmitWaitToken> pending_jobs{ task_mem_arena.arena };

    XRAY_SCOPE_EXIT noexcept
    {
        ScratchPadVector<VkFence> fences{ task_mem_arena.arena };
        lz::chain(pending_jobs).transformTo(back_inserter(fences), [](const QueueSubmitWaitToken& token) {
            return token.fence();
        });
        WRAP_VULKAN_FUNC(
            vkWaitForFences, renderer->device(), static_cast<uint32_t>(fences.size()), fences.data(), true, 0xffffffff);
    };

    vector<VulkanImage> loaded_textures;
    for (const std::filesystem::path& p : texture_files) {
        char scratch_buffer[512];
        auto out =
            fmt::format_to_n(scratch_buffer, std::size(scratch_buffer), "texture_{}", p.filename().generic_string());
        *out.out = 0;

        //
        // submit each texture separatly
        auto transfer_job = renderer->create_job(QueueType ::Transfer);
        XR_COR_PROPAGATE_ERROR(transfer_job);

        auto tex =
            VulkanImage::from_file(*renderer,
                                   VulkanImageLoadInfo{
                                       .tag_name = scratch_buffer,
                                       .cmd_buf = *transfer_job,
                                       .path = xr_app_config->texture_path(p),
                                       .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                       .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                       .tiling = VK_IMAGE_TILING_OPTIMAL,
                                   });
        XR_COR_PROPAGATE_ERROR(tex);

        loaded_textures.push_back(std::move(*tex));

        auto wait_token = renderer->submit_job(*transfer_job, QueueType ::Transfer);
        XR_COR_PROPAGATE_ERROR(wait_token);
        pending_jobs.emplace_back(std::move(*wait_token));
    }

    //
    // create the null + color texture
    tl::expected<SmallVec<VulkanImage, 2>, VulkanError> special_textures =
        [&]() -> tl::expected<SmallVec<VulkanImage, 2>, VulkanError> {
        auto transfer_job = renderer->create_job(QueueType::Transfer);
        XR_VK_PROPAGATE_ERROR(transfer_job);

        ScratchPadArena scratch_pad{ &task_mem_arena.arena };

        ScratchPadVector<uint8_t> null_texture_pixels{ size_t{ 256 * 256 * 4 }, uint8_t{}, scratch_pad };
        xor_fill(256, 256, XorPatternType::Colored, std::span{ null_texture_pixels });

        const VulkanImageCreateInfo tex_data[] = {
            VulkanImageCreateInfo{
                .tag_name = "null_texture",
                .wpkg = *transfer_job,
                .type = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .width = 256,
                .height = 256,
                .pixels = { to_bytes_span(null_texture_pixels) },
            },
            VulkanImageCreateInfo{
                .tag_name = "color_texture",
                .wpkg = *transfer_job,
                .type = VK_IMAGE_TYPE_1D,
                .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                .width = 1024,
                .height = 1,
                .pixels = { to_bytes_span(color_pixels) },
            },
        };

        SmallVec<VulkanImage, 2> imgs;
        for (const VulkanImageCreateInfo& im : tex_data) {
            auto tex = VulkanImage::from_memory(*renderer, im);
            XR_VK_PROPAGATE_ERROR(tex);
            imgs.push_back(std::move(*tex));
        }

        auto wait_token = renderer->submit_job(*transfer_job, QueueType ::Transfer);
        XR_VK_PROPAGATE_ERROR(wait_token);

        pending_jobs.emplace_back(std::move(*wait_token));
        return imgs;
    }();

    XR_VK_COR_PROPAGATE_ERROR(special_textures);

    const uint32_t image_count = static_cast<uint32_t>(texture_files.size() + 1);
    const uint32_t img_slot = renderer->bindless_sys().reserve_image_slots(image_count);
    const uint32_t sbo_slot = renderer->bindless_sys().reserve_sbo_slots(2);

    struct GPUColoredMaterial
    {
        uint32_t texel;
    };

    ScratchPadVector<GPUColoredMaterial> gpu_colored_mtls{ task_mem_arena.arena };
    for (ColoredMaterial& cm : colored_materials) {
        gpu_colored_mtls.emplace_back(cm.texel);
    }

    struct GPUTexturedMaterial
    {
        uint32_t ambient;
        uint32_t diffuse;
        uint32_t specular;
    };
    ScratchPadVector<GPUTexturedMaterial> gpu_textured_mtls{ task_mem_arena.arena };
    for (const TexturedMaterial& tm : textured_materials) {
        //
        // colors texture takes first slot reserved from BindlessSys
        gpu_textured_mtls.emplace_back(
            tm.ambient + 1 + img_slot, tm.diffuse + 1 + img_slot, tm.specular + 1 + img_slot);
    }

    const tuple<string_view, span<const uint8_t>> material_sbos_create_data[] = {
        { "sbo_colored_materials", to_bytes_span(gpu_colored_mtls) },
        { "sbo_textured_materials", to_bytes_span(gpu_textured_mtls) },
    };

    ScratchPadVector<VulkanBuffer> material_sbos{ task_mem_arena.arena };
    for (const auto [sbo_name, sbo_data] : material_sbos_create_data) {
        auto sbo =
            VulkanBuffer::create(*renderer,
                                 VulkanBufferCreateInfo{
                                     .name_tag = sbo_name.data(),
                                     .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     .bytes = sbo_data.size_bytes(),
                                     .frames = 1,
                                 });

        XR_VK_COR_PROPAGATE_ERROR(sbo);

        const uintptr_t staging_buffer_offset = renderer->reserve_staging_buffer_memory(sbo_data.size_bytes());
        uintptr_t staging_ptr = renderer->staging_buffer_memory() + staging_buffer_offset;
        memcpy((void*)staging_ptr, sbo_data.data(), sbo_data.size_bytes());

        auto job = renderer->create_job(QueueType::Transfer);
        XR_VK_COR_PROPAGATE_ERROR(job);

        const VkBufferCopy copy_region{
            .srcOffset = staging_buffer_offset,
            .dstOffset = 0,
            .size = sbo_data.size_bytes(),
        };

        vkCmdCopyBuffer(*job, renderer->staging_buffer(), sbo->buffer_handle(), 1, &copy_region);
        auto wait_token = renderer->submit_job(*job, QueueType ::Transfer);
        XR_VK_COR_PROPAGATE_ERROR(wait_token);

        pending_jobs.push_back(std::move(*wait_token));
        material_sbos.emplace_back(std::move(*sbo));
    }

    exec_timer.end();
    XR_LOG_INFO("[[TASK]] - non gltf materials done , {}", exec_timer.elapsed_millis());

    XR_LOG_INFO("[[TASK]] - non gltf arena stats: allocations {}, largest block {}, high water {}, total bytes {}",
                task_mem_arena.arena.stats.allocations,
                task_mem_arena.arena.stats.largest_alloc,
                task_mem_arena.arena.stats.high_water,
                task_mem_arena.arena.stats.allocated);

    co_return tl::expected<NonGltfMaterialsData, ProgramError>{
        tl::in_place,
        std::move((*special_textures)[0]),
        std::move(colored_materials),
        std::move(textured_materials),
        std::move((*special_textures)[1]),
        std::move(loaded_textures),
        std::move(material_sbos[0]),
        std::move(material_sbos[1]),
        img_slot,
        sbo_slot,
    };
}

concurrencpp::result<tl::expected<SceneDefinition, ProgramError>>
main_task(concurrencpp::executor_tag,
          concurrencpp::thread_pool_executor* tpe,
          concurrencpp::shared_result<VulkanRenderer*> renderer_result)
{
    timer_highp exec_timer{};
    exec_timer.start();

    const auto loaded_scene = SceneDescription::from_file(xr_app_config->config_path("scenes/simple.scene.conf"));
    XR_COR_PROPAGATE_ERROR(loaded_scene);

    const SceneDescription* scenedes{ &loaded_scene.value() };

    //
    // spawn task for shader compilation
    auto pipelines_task_result = task_create_graphics_pipelines(concurrencpp::executor_tag{}, tpe, renderer_result);

    //
    // spawn task for non-gltf materials
    auto non_gltf_materials_task_result =
        task_create_non_gltf_materials(concurrencpp::executor_tag{},
                                       tpe,
                                       renderer_result,
                                       std::span<const scene::MaterialDescription>{ scenedes->materials });

    //
    // spawn task for GLTF data
    auto gltf_render_resources_result = task_create_gltf_resources(
        concurrencpp::executor_tag{}, tpe, renderer_result, std::span{ scenedes->gltf_geometries });

    //
    // process procedurally generated geomtry data
    vector<ProceduralGeometryEntry> procedural_geometries;
    SmallVec<geometry_data_t> gdata;
    vector<VkDrawIndexedIndirectCommand> draw_indirect_template;
    SmallVec<span<const uint8_t>> vertex_span;
    SmallVec<span<const uint32_t>> index_span;
    vec2ui32 vtx_idx_accum{};

    //
    // procedurally generated shapes
    for (const ProceduralGeometryDescription& pg : loaded_scene.value().procedural_geometries) {
        geometry_data_t geometry = rfl::visit(
            [&](const auto& s) {
                using Name = typename std::decay_t<decltype(s)>::Name;
                if constexpr (std::is_same<Name, rfl::Literal<"grid">>()) {
                    const GridParams gp = s.value();
                    geometry_data_t grid = geometry_factory::grid(gp);

                    noise::module::RidgedMulti ridged;
                    noise::module::Billow base_flat_terrain;
                    base_flat_terrain.SetFrequency(2.0);

                    noise::module::ScaleBias flat_terrain;
                    flat_terrain.SetSourceModule(0, base_flat_terrain);
                    flat_terrain.SetScale(0.125);
                    flat_terrain.SetBias(-0.75);

                    noise::module::Perlin terrain_type;
                    terrain_type.SetOctaveCount(6);
                    terrain_type.SetFrequency(0.5);
                    terrain_type.SetPersistence(0.25);

                    noise::module::Select terrain_selector;
                    terrain_selector.SetSourceModule(0, flat_terrain);
                    terrain_selector.SetSourceModule(1, ridged);
                    terrain_selector.SetControlModule(terrain_type);
                    terrain_selector.SetBounds(0.0f, 1000.0f);
                    terrain_selector.SetEdgeFalloff(0.125);

                    noise::module::Turbulence final_terrain;
                    final_terrain.SetSourceModule(0, terrain_selector);
                    final_terrain.SetFrequency(4.0);
                    final_terrain.SetPower(0.125);

                    noise::utils::NoiseMap heightmap;
                    noise::utils::NoiseMapBuilderPlane heightmap_builder;
                    heightmap_builder.SetDestNoiseMap(heightmap);
                    heightmap_builder.SetSourceModule(final_terrain);
                    heightmap_builder.SetDestSize((gp.cellsx + 1), (gp.cellsy + 1));
                    heightmap_builder.SetBounds(-3.0, 3.0, 1.0, 4.0);
                    heightmap_builder.Build();

                    vertex_pntt* verts = grid.vertex_data();
                    for (uint32_t z = 0; z < gp.cellsy + 1; ++z) {
                        for (uint32_t x = 0; x < gp.cellsx + 1; ++x) {
                            verts[z * (gp.cellsx + 1) + x].position.y = heightmap.GetValue(x, z);
                        }
                    }

                    return grid;
                } else if constexpr (std::is_same<Name, rfl::Literal<"cone">>()) {
                    return geometry_factory::conical_section(s.value());
                } else if constexpr (std::is_same<Name, rfl::Literal<"sphere">>()) {
                    return geometry_factory::geosphere(s.value());
                } else if constexpr (std::is_same<Name, rfl::Literal<"torus">>()) {
                    return geometry_factory::torus(s.value());
                } else if constexpr (std::is_same<Name, rfl::Literal<"ring">>()) {
                    return geometry_factory::ring_geometry(s.value());
                } else if constexpr (std::is_same<Name, rfl::Literal<"torus_knot">>()) {
                    return geometry_factory::torus_knot(s.value());
                } else {
                    static_assert(rfl::always_false_v<ProceduralGeometryDescription>, "Not all cases were covered.");
                }
            },
            pg.gen_params);

        vertex_span.push_back(to_bytes_span(geometry.vertex_span()));
        index_span.push_back(geometry.index_span());

        draw_indirect_template.push_back(VkDrawIndexedIndirectCommand{
            .indexCount = static_cast<uint32_t>(geometry.index_count),
            .instanceCount = 1,
            .firstIndex = vtx_idx_accum.y,
            .vertexOffset = static_cast<int32_t>(vtx_idx_accum.x),
            .firstInstance = 0,
        });

        procedural_geometries.emplace_back(pg.name,
                                           GeometryHandleType{ FNV::fnv1a(pg.name) },
                                           vec2ui32{ geometry.vertex_count, geometry.index_count },
                                           vtx_idx_accum);

        vtx_idx_accum.x += geometry.vertex_count;
        vtx_idx_accum.y += geometry.index_count;
        gdata.push_back(std::move(geometry));
    }

    //
    // spawn task to create Vulkan resources for procedural geometry
    auto procedural_render_resources_result =
        task_create_procedural_geometry_render_resources(concurrencpp::executor_tag{},
                                                         tpe,
                                                         GeometryResourceTaskParams{
                                                             .resource_tag = "generated geometry",
                                                             .renderer_result = renderer_result,
                                                             .vertex_data = std::span{ vertex_span },
                                                             .index_data = std::span{ index_span },
                                                         });

    vector<EntityDrawableComponent> scene_entities;
    for (const ProceduralEntityDescription& pe : loaded_scene.value().procedural_entities) {
        const GeometryHandleType geometry_id = GeometryHandleType{ FNV::fnv1a(pe.geometry) };
        auto itr_geometry =
            std::find_if(cbegin(procedural_geometries),
                         cend(procedural_geometries),
                         [geometry_id](const ProceduralGeometryEntry& pg) { return pg.hashed_name == geometry_id; });

        if (itr_geometry == std::cend(procedural_geometries)) {
            co_return tl::make_unexpected(
                SceneError{ .err = fmt::format("Entity {} uses undefined geometry {}", pe.name, pe.geometry) });
        }

        const uint32_t entity_id = FNV::fnv1a(pe.name);
        scene_entities.push_back(EntityDrawableComponent{
            .name = pe.name,
            .hashed_name = FNV::fnv1a(pe.name),
            .geometry_id = geometry_id,
            // assume color material for all since the task that creates the materials is not yet ready
            // it will be fix it later when the task results are ready
            .material_id = ColorMaterialType{ FNV::fnv1a(pe.material) },
            .orientation = pe.orientation.value_or(OrientationF32{}),
        });

        XR_LOG_INFO("Procedural entity {} {}", pe.name, pe.material);
    }

    assert(procedural_geometries.size() == gdata.size());
    assert(gdata.size() == draw_indirect_template.size());

    for (const GLTFEntityDescription& ee : scenedes->gltf_entities) {
        const uint32_t geometry_id = FNV::fnv1a(ee.gltf);
        scene_entities.push_back(EntityDrawableComponent{
            .name = ee.name,
            .hashed_name = FNV::fnv1a(ee.name),
            .geometry_id = GeometryHandleType{ FNV::fnv1a(ee.gltf) },
            .material_id = tl::nullopt,
            .orientation = ee.orientation.value_or(OrientationF32{}),
        });
    }

    VulkanRenderer* renderer = co_await renderer_result;
    const RenderBufferingSetup rbs = renderer->buffering_setup();

    auto instances_buffer =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = "Global instance data buffer",
                                 .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                 .bytes = (scene_entities.size() + scene_entities.size()) * sizeof(InstanceRenderInfo),
                                 .frames = rbs.buffers,
                             });
    XR_VK_COR_PROPAGATE_ERROR(instances_buffer);

    vector<VulkanBuffer> sbos_lights;
    const SmallVec<std::tuple<size_t, string_view>> sbo_lights_data{
        { container_bytes_size(scenedes->directional_lights), "[[sbo]] directional_lights" },
        { container_bytes_size(scenedes->point_lights), "[[sbo]] point_lights" },
        { container_bytes_size(scenedes->spot_lights), "[[sbo]] spot_lights" },
    };

    for (const auto [bytes_size, name_tag] : sbo_lights_data) {
        auto sbo = VulkanBuffer::create(*renderer,
                                        VulkanBufferCreateInfo{
                                            .name_tag = name_tag.data(),
                                            .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                            .bytes = bytes_size,
                                            .frames = renderer->buffering_setup().buffers,
                                        });
        XR_VK_COR_PROPAGATE_ERROR(sbo);
        sbos_lights.emplace_back(std::move(*sbo));
    }

    auto procedural_render_resource = co_await procedural_render_resources_result;
    XR_COR_PROPAGATE_ERROR(procedural_render_resource);

    auto gltf_render_resources = co_await gltf_render_resources_result;
    XR_COR_PROPAGATE_ERROR(gltf_render_resources);

    auto non_gltf_materials = co_await non_gltf_materials_task_result;
    XR_COR_PROPAGATE_ERROR(non_gltf_materials);

    lz::chain(scene_entities)
        .filter([](const EntityDrawableComponent& e) { return e.material_id.has_value(); })
        .forEach([m = &*non_gltf_materials](EntityDrawableComponent& e) {
            const uint32_t mtl_id = swl::visit(VariantVisitor{
                                                   [](ColorMaterialType cm) { return cm.value_of(); },
                                                   [](TexturedMaterialType tm) { return tm.value_of(); },
                                               },
                                               *e.material_id);

            if (const auto itr_color = ranges::find_if(
                    m->materials_textured, [mtl_id](const TexturedMaterial& tm) { return tm.hashed_name == mtl_id; });
                itr_color != ranges::cend(m->materials_textured)) {
                e.material_id = TexturedMaterialType{ itr_color->hashed_name };
                return;
            }
        });

    auto pipeline_resources = co_await pipelines_task_result;
    XR_COR_PROPAGATE_ERROR(pipeline_resources);

    exec_timer.end();
    XR_LOG_INFO("[[TASK]] - main done, {} ...", exec_timer.elapsed_millis());

    co_return SceneDefinition{
        .gltf = std::move(std::get<0>(*gltf_render_resources)),
        .procedural =
            ProceduralGeometry{
                .procedural_geometries = std::move(procedural_geometries),
                .draw_indirect_template = std::move(draw_indirect_template),
                .vertex_buffer = std::move(procedural_render_resource->vertexbuffer),
                .index_buffer = std::move(procedural_render_resource->indexbuffer),
            },
        .instances_buffer = std::move(*instances_buffer),
        .entities = std::move(scene_entities),
        .materials_nongltf = std::move(*non_gltf_materials),
        .materials_gltf = std::move(std::get<1>(*gltf_render_resources)),
        .pipelines = std::move(*pipeline_resources),
        .sbos_lights = std::move(sbos_lights),
        .directional_lights = std::move(scenedes->directional_lights),
        .point_lights = std::move(scenedes->point_lights),
        .spot_lights = std::move(scenedes->spot_lights),
    };
}

class GameMain
{
  private:
    struct PrivateConstructToken
    {
        explicit PrivateConstructToken() = default;
    };

  public:
    GameMain(PrivateConstructToken,
             unique_pointer<concurrencpp::runtime> co_runtime,
             xray::ui::window window,
             xray::base::unique_pointer<xray::rendering::VulkanRenderer> vkrenderer,
             xray::base::unique_pointer<xray::ui::user_interface> ui,
             xray::base::unique_pointer<xray::ui::UserInterfaceRenderBackend_Vulkan> ui_backend,
             xray::base::unique_pointer<xray::rendering::DebugDrawSystem> debug_draw,
             xray::rendering::BindlessUniformBufferResourceHandleEntryPair global_ubo,
             xray::base::unique_pointer<SceneDefinition> scenedef,
             xray::base::unique_pointer<SceneResources> sceneres,
             MemoryArena* arena_perm,
             MemoryArena* arena_temp)
        : _co_runtime{ std::move(co_runtime) }
        , _window{ std::move(window) }
        , _vkrenderer{ std::move(vkrenderer) }
        , _ui{ std::move(ui) }
        , _ui_backend{ std::move(ui_backend) }
        , _debug_draw{ std::move(debug_draw) }
        , _global_ubo{ global_ubo }
        , _scenedef{ std::move(scenedef) }
        , _sceneres{ std::move(sceneres) }
        , _arena_perm{ arena_perm }
        , _arena_temp{ arena_temp }
    {
        hookup_event_delegates();

        lz::chain(_registered_demos).forEach([](const RegisteredDemo& rd) {
            XR_LOG_INFO("Registered demo: {} - {}", rd.short_desc, rd.detailed_desc);
        });

        constexpr const char* combo_items[] = { "just", "some", "dummy", "items", "for the", "combo" };
        for (const char* e : combo_items) {
            _combo_items.insert(_combo_items.end(), e, e + strlen(e));
            _combo_items.push_back(0);
        }
        _combo_items.push_back(0);

        _timer.start();
    }

    GameMain(GameMain&& rhs) = default;
    ~GameMain();

    static tl::expected<GameMain, ProgramError> create(MemoryArena* arena_perm, MemoryArena* arena_temp);
    void run();

  private:
    bool demo_running() const noexcept { return _demo != nullptr; }
    void run_demo(const demo_type type);
    void hookup_event_delegates();
    void demo_quit();

    const RegisteredDemo& get_demo_info(const size_t idx) const
    {
        assert(idx < _registered_demos.size());
        return _registered_demos[idx];
    }

    /// \group Event handlers
    /// @{

    void loop_event(const xray::ui::window_loop_event& loop_evt);
    void event_handler(const xray::ui::window_event& wnd_evt);
    void poll_start(const xray::ui::poll_start_event&);
    void poll_end(const xray::ui::poll_end_event&);

    /// @}

  private:
    xray::base::unique_pointer<concurrencpp::runtime> _co_runtime;
    xray::ui::window _window;
    xray::base::unique_pointer<xray::rendering::VulkanRenderer> _vkrenderer;
    xray::base::unique_pointer<xray::ui::user_interface> _ui{};
    xray::base::unique_pointer<xray::ui::UserInterfaceRenderBackend_Vulkan> _ui_backend{};
    xray::base::unique_pointer<xray::rendering::DebugDrawSystem> _debug_draw{};
    xray::base::unique_pointer<DemoBase> _demo{};
    xray::rendering::rgb_color _clear_color{ xray::rendering::color_palette::material::deeppurple900 };
    xray::base::timer_highp _timer{};
    vector<RegisteredDemo> _registered_demos;
    vector<char> _combo_items{};
    xray::base::unique_pointer<SceneDefinition> _scenedef;
    xray::base::unique_pointer<SceneResources> _sceneres;
    xray::rendering::BindlessUniformBufferResourceHandleEntryPair _global_ubo;
    MemoryArena* _arena_perm{};
    MemoryArena* _arena_temp{};
    XRAY_NO_COPY(GameMain);
};

GameMain::~GameMain() {}

tl::expected<GameMain, ProgramError>
GameMain::create(MemoryArena* arena_perm, MemoryArena* arena_temp)
{
    using namespace xray::ui;
    using namespace xray::base;

    unique_pointer<concurrencpp::runtime> cor_runtime{ base::make_unique<concurrencpp::runtime>() };

    static ConfigSystem app_cfg{ "config/app_config.conf" };
    xr_app_config = &app_cfg;

    //
    // start font loading
    concurrencpp::result<FontsLoadBundle> font_pkg_load_result =
        task_load_fonts(concurrencpp::executor_tag{}, cor_runtime->thread_executor().get());

    concurrencpp::result_promise<VulkanRenderer*> renderer_promise{};

    //
    // start model and generated geometry
    concurrencpp::shared_result<VulkanRenderer*> renderer_result{ renderer_promise.get_result() };
    auto main_task_result =
        main_task(concurrencpp::executor_tag{}, cor_runtime->thread_pool_executor().get(), renderer_result);

    const window_params_t wnd_params{ "Vulkan Demo", 4, 5, 24, 8, 32, 0, 1, false };
    window main_window{ wnd_params };

    if (!main_window) {
        return tl::make_unexpected(MiscError{ .what = "Failed to initialize application window" });
    }

    const RendererConfig rcfg{ RendererConfig::from_file(xr_app_config->config_root() / "renderer.conf") };

    tl::optional<VulkanRenderer> opt_renderer
    {
        VulkanRenderer::create(
#if defined(XRAY_OS_IS_WINDOWS)
            WindowPlatformDataWin32{
                .module = main_window.native_module(),
                .window = main_window.native_window(),
                .width = static_cast<uint32_t>(main_window.width()),
                .height = static_cast<uint32_t>(main_window.height()),
            }
#else
            WindowPlatformDataXlib{
                .display = main_window.native_display(),
                .window = main_window.native_window(),
                .visual = main_window.native_visual(),
                .width = static_cast<uint32_t>(main_window.width()),
                .height = static_cast<uint32_t>(main_window.height()),
            }
#endif
            ,
            rcfg)
    };

    if (!opt_renderer) {
        return tl::make_unexpected(MiscError{ .what = "Failed to create Vulkan renderer" });
    }

    auto renderer = xray::base::make_unique<VulkanRenderer>(std::move(*opt_renderer));
    renderer->add_shader_include_directories({ xr_app_config->shader_root() });
    auto slot_null_tex = renderer->bindless_sys().reserve_image_slots(1);
    assert(slot_null_tex == 0);

    //
    // resume anyone waiting for the renderer
    renderer_promise.set_result(raw_ptr(renderer));

    ScopedMediumArenaType arena_perm0 = GlobalMemorySystem::instance()->grab_medium_arena();
    ScopedSmallArenaType arena_temp0 = GlobalMemorySystem::instance()->grab_small_arena();

    auto debug_draw = DebugDrawSystem::create(DebugDrawSystem::InitContext{
        .renderer = &*renderer,
        .arena_perm = &arena_perm0.arena,
        .arena_temp = &arena_temp0.arena,
    });
    XR_PROPAGATE_ERROR(debug_draw);

    const VulkanBufferCreateInfo bci{
        .name_tag = "UBO - FrameGlobalData",
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .bytes = sizeof(FrameGlobalData),
        .frames = renderer->buffering_setup().buffers,
        .initial_data = {},
    };

    tl::expected<VulkanBuffer, VulkanError> g_ubo =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = "UBO - FrameGlobalData",
                                 .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                 .bytes = sizeof(FrameGlobalData),
                                 .frames = renderer->buffering_setup().buffers,
                                 .initial_data = {},
                             });
    XR_PROPAGATE_ERROR(g_ubo);

    //
    //     RegisteredDemosList<FractalDemo, DirectionalLightDemo, procedural_city_demo,
    //     InstancedDrawingDemo>::registerDemo(
    //         _registeredDemos);
    //
    //     const string_view first_entry{ "Main page" };
    //     copy(cbegin(first_entry), cend(first_entry), back_inserter(_combo_items));
    //     _combo_items.push_back(0);
    //
    //     _registeredDemos >>= pipes::for_each([this](const DemoInfo& demo) {
    //         XR_LOG_DEBUG("Demo: {}\n{}", demo.shortDesc, demo.detailedDesc);
    //         const string_view demo_desc{ demo.shortDesc };
    //         copy(cbegin(demo_desc), cend(demo_desc), back_inserter(_combo_items));
    //         _combo_items.push_back(0);
    //     });
    //     _combo_items.push_back(0);
    //

    xray::base::unique_pointer<user_interface> ui{ xray::base::make_unique<xray::ui::user_interface>(
        std::move(font_pkg_load_result)) };

    tl::expected<UserInterfaceRenderBackend_Vulkan, VulkanError> vk_backend{
        UserInterfaceRenderBackend_Vulkan::create(
            *renderer, ui->render_backend_create_info(), &arena_perm0.arena, &arena_temp0.arena),
    };
    XR_PROPAGATE_ERROR(vk_backend);

    ui->set_global_font("TerminessNerdFontMono-Regular");

    const BindlessUniformBufferResourceHandleEntryPair g_ubo_handles =
        renderer->bindless_sys().add_chunked_uniform_buffer(std::move(*g_ubo), renderer->buffering_setup().buffers);

    tl::expected<SceneDefinition, ProgramError> scene_result = main_task_result.get();
    XR_PROPAGATE_ERROR(scene_result);

    SceneResources scene_resources{ SceneResources::from_scene(&*scene_result, raw_ptr(renderer)) };

    return tl::expected<GameMain, ProgramError>(
        tl::in_place,
        PrivateConstructToken{},
        std::move(cor_runtime),
        std::move(main_window),
        std::move(renderer),
        std::move(ui),
        xray::base::make_unique<UserInterfaceRenderBackend_Vulkan>(std::move(*vk_backend)),
        xray::base::make_unique<DebugDrawSystem>(std::move(*debug_draw)),
        g_ubo_handles,
        xray::base::make_unique<SceneDefinition>(std::move(*scene_result)),
        xray::base::make_unique<SceneResources>(std::move(scene_resources)),
        arena_perm,
        arena_temp);
}

void
GameMain::run()
{
    hookup_event_delegates();
    _window.message_loop();
    _vkrenderer->wait_device_idle();
}

void
GameMain::demo_quit()
{
    assert(demo_running());
    _vkrenderer->wait_device_idle();
    _demo = nullptr;
}

void
GameMain::poll_start(const xray::ui::poll_start_event&)
{
}

void
GameMain::poll_end(const xray::ui::poll_end_event&)
{
}

void
GameMain::hookup_event_delegates()
{
    _window.core.events.loop = cpp::bind<&GameMain::loop_event>(this);
    _window.core.events.poll_start = cpp::bind<&GameMain::poll_start>(this);
    _window.core.events.poll_end = cpp::bind<&GameMain::poll_end>(this);
    _window.core.events.window = cpp::bind<&GameMain::event_handler>(this);
}

void
GameMain::event_handler(const xray::ui::window_event& wnd_evt)
{
    _ui->input_event(wnd_evt);

    if (demo_running() && !_ui->wants_input()) {
        _demo->event_handler(wnd_evt);
        return;
    }

    if (is_input_event(wnd_evt)) {
        if (wnd_evt.event.key.keycode == xray::ui::KeySymbol::escape &&
            wnd_evt.event.key.type == event_action_type::press && !_ui->wants_input()) {
            _window.quit();
            return;
        }
    }
}

void
GameMain::loop_event(const xray::ui::window_loop_event& loop_event)
{
    ZoneScopedNCS("LoopEvent", 0xFF0000FF, 32);
    _arena_temp->free_all();

    static bool doing_ur_mom{ false };
    if (!doing_ur_mom && !_demo) {
        _demo = dvk::TriangleDemo::create(app::init_context_t{
            .surface_width = _window.width(),
            .surface_height = _window.height(),
            .cfg = xr_app_config,
            .ui = raw_ptr(_ui),
            .renderer = raw_ptr(_vkrenderer),
            .quit_receiver = cpp::bind<&GameMain::demo_quit>(this),
        });
        doing_ur_mom = true;
    }

    _timer.update_and_reset();
    const float delta = _timer.elapsed_millis();
    _ui->tick(delta);
    _ui->new_frame(loop_event.wnd_width, loop_event.wnd_height);

    const FrameRenderData frd{ _vkrenderer->begin_rendering() };

    _debug_draw->new_frame(frd.id);

    //
    // flush and bind the global descriptor table
    _vkrenderer->bindless_sys().flush_descriptors(*_vkrenderer);
    _vkrenderer->bindless_sys().bind_descriptors(*_vkrenderer, frd.cmd_buf);

    auto g_ubo_mapping = UniqueMemoryMapping::map_memory(_vkrenderer->device(),
                                                         _global_ubo.second.memory,
                                                         frd.id * _global_ubo.second.aligned_chunk_size,
                                                         _global_ubo.second.aligned_chunk_size);

    if (_demo) {
        _demo->loop_event(RenderEvent{
            .loop_event = loop_event,
            .frame_data = &frd,
            .renderer = raw_ptr(_vkrenderer),
            .ui = xray::base::raw_ptr(_ui),
            .g_ubo_data = g_ubo_mapping->as<FrameGlobalData>(),
            .dbg_draw = raw_ptr(_debug_draw),
            .sdef = raw_ptr(_scenedef),
            .sres = raw_ptr(_sceneres),
            .delta = delta,
            .arena_perm = _arena_perm,
            .arena_temp = _arena_temp,
        });
    } else {
        //
        // do main page UI
        if (ImGui::Begin("Run a demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
            int32_t selectedItem{};
            const bool wasClicked = ImGui::Combo("Available demos", &selectedItem, _combo_items.data());
            //
            if (wasClicked && selectedItem >= 1) {
                //                 const init_context_t initContext{ _window->width(),
                //                                                   _window->height(),
                //                                                   xr_app_config,
                //                                                   xray::base::raw_ptr(_ui),
                //                                                   make_delegate(*this, &main_app::demo_quit) };
                //
                //                 _registeredDemos[static_cast<size_t>(selectedItem - 1)]
                //                     .createFn(initContext)
                //                     .map([this](demo_bundle_t bundle) {
                //                         auto [demoObj, winEvtHandler, pollEvtHandler] = move(bundle);
                //                         this->_demo = std::move(demoObj);
                //                         this->_window->events.window = winEvtHandler;
                //                         this->_window->events.loop = pollEvtHandler;
                //                     });
            }
        }
        ImGui::End();

        _vkrenderer->clear_attachments(frd.cmd_buf, 1.0f, 0.0f, 1.0f);
    }

    _debug_draw->render(DebugDrawSystem::RenderContext{ .renderer = raw_ptr(_vkrenderer), .frd = &frd });

    const VkViewport viewport{
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(loop_event.wnd_width),
        .height = static_cast<float>(loop_event.wnd_height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(frd.cmd_buf, 0, 1, &viewport);

    const VkRect2D scissor{
        .offset = VkOffset2D{ 0, 0 },
        .extent =
            VkExtent2D{ static_cast<uint32_t>(loop_event.wnd_width), static_cast<uint32_t>(loop_event.wnd_height) },
    };

    vkCmdSetScissor(frd.cmd_buf, 0, 1, &scissor);

    //
    // move the UBO mapping into the lambda so that the data is flushed before the rendering starts
    _ui->draw()
        .map([this, frd, g_ubo = std::move(*g_ubo_mapping)](UserInterfaceRenderContext ui_render_ctx) mutable {
            FrameGlobalData* fg = g_ubo.as<FrameGlobalData>();
            fg->frame = frd.id;
            fg->ui = UIData{
                .scale = vec2f{ ui_render_ctx.scale_x, ui_render_ctx.scale_y },
                .translate = vec2f{ ui_render_ctx.translate_x, ui_render_ctx.translate_y },
                .textureid = ui_render_ctx.textureid,
            };

            return ui_render_ctx;
        })
        .map([this, frd](UserInterfaceRenderContext ui_render_ctx) {
            ZoneScopedNCS("UI Draw", tracy::Color::OrangeRed, 32);
            _ui_backend->render(ui_render_ctx, *_vkrenderer, frd);
        });

    _vkrenderer->end_rendering();
    FrameMark;
}

//
// void
// main_app::run_demo(const demo_type type)
// {
//     // auto make_demo_fn =
//     //   [this, w = _window](const demo_type dtype) -> unique_pointer<demo_base> {
//     //   const init_context_t init_context{
//     //     w->width(),
//     //     w->height(),
//     //     xr_app_config,
//     //     xray::base::raw_ptr(_ui),
//     //     make_delegate(*this, &main_app::demo_quit)};
//     //
//     //   switch (dtype) {
//     //
//     //     //    case demo_type::colored_circle:
//     //     //      return xray::base::make_unique<colored_circle_demo>();
//     //     //      break;
//     //
//     //   case demo_type::fractal:
//     //     return xray::base::make_unique<fractal_demo>(init_context);
//     //     break;
//     //
//     //   case demo_type::texture_array:
//     //     return xray::base::make_unique<texture_array_demo>(init_context);
//     //     break;
//     //
//     //   case demo_type::mesh:
//     //     return xray::base::make_unique<mesh_demo>(init_context);
//     //     break;
//     //
//     //     //    case demo_type::bufferless_draw:
//     //     //      return xray::base::make_unique<bufferless_draw_demo>();
//     //     //      break;
//     //
//     //   case demo_type::lighting_directional:
//     //     return xray::base::make_unique<directional_light_demo>(init_context);
//     //     break;
//     //
//     //     //    case demo_type::procedural_city:
//     //     //      return
//     //     //      xray::base::make_unique<procedural_city_demo>(init_context);
//     //     //      break;
//     //
//     //   case demo_type::instanced_drawing:
//     //     return xray::base::make_unique<instanced_drawing_demo>(init_context);
//     //     break;
//     //
//     //     //    case demo_type::geometric_shapes:
//     //     //      return
//     //     //      xray::base::make_unique<geometric_shapes_demo>(init_context);
//     //     //      break;
//     //
//     //     //    case demo_type::lighting_point:
//     //     //      return
//     //     xray::base::make_unique<point_light_demo>(&init_context);
//     //     //      break;
//     //
//     //     // case demo_type::terrain_basic:
//     //     //   return xray::base::make_unique<terrain_demo>(init_context);
//     //     //   break;
//     //
//     //   default:
//     //     break;
//     //   }
//     //
//     //   return nullptr;
//     // };
//     //
//     // _demo = make_demo_fn(type);
//     // if (!_demo) {
//     //   return;
//     // }
//     //
//     // _window->events.loop       = make_delegate(*_demo, &demo_base::loop_event);
//     // _window->events.poll_start = make_delegate(*_demo, &demo_base::poll_start);
//     // _window->events.poll_end   = make_delegate(*_demo, &demo_base::poll_end);
//     // _window->events.window     = make_delegate(*_demo,
//     // &demo_base::event_handler);
// }
//
// void
// debug_proc(GLenum source,
//            GLenum type,
//            GLuint id,
//            GLenum severity,
//            GLsizei /*length*/,
//            const GLchar* message,
//            const void* /*userParam*/)
// {
//
//     auto msg_source = [source]() {
//         switch (source) {
//             case gl::DEBUG_SOURCE_API:
//                 return "API";
//                 break;
//
//             case gl::DEBUG_SOURCE_APPLICATION:
//                 return "APPLICATION";
//                 break;
//
//             case gl::DEBUG_SOURCE_OTHER:
//                 return "OTHER";
//                 break;
//
//             case gl::DEBUG_SOURCE_SHADER_COMPILER:
//                 return "SHADER COMPILER";
//                 break;
//
//             case gl::DEBUG_SOURCE_THIRD_PARTY:
//                 return "THIRD PARTY";
//                 break;
//
//             case gl::DEBUG_SOURCE_WINDOW_SYSTEM:
//                 return "WINDOW SYSTEM";
//                 break;
//
//             default:
//                 return "UNKNOWN";
//                 break;
//         }
//     }();
//
//     const auto msg_type = [type]() {
//         switch (type) {
//             case gl::DEBUG_TYPE_ERROR:
//                 return "ERROR";
//                 break;
//
//             case gl::DEBUG_TYPE_DEPRECATED_BEHAVIOR:
//                 return "DEPRECATED BEHAVIOUR";
//                 break;
//
//             case gl::DEBUG_TYPE_MARKER:
//                 return "MARKER";
//                 break;
//
//             case gl::DEBUG_TYPE_OTHER:
//                 return "OTHER";
//                 break;
//
//             case gl::DEBUG_TYPE_PERFORMANCE:
//                 return "PERFORMANCE";
//                 break;
//
//             case gl::DEBUG_TYPE_POP_GROUP:
//                 return "POP GROUP";
//                 break;
//
//             case gl::DEBUG_TYPE_PORTABILITY:
//                 return "PORTABILITY";
//                 break;
//
//             case gl::DEBUG_TYPE_PUSH_GROUP:
//                 return "PUSH GROUP";
//                 break;
//
//             case gl::DEBUG_TYPE_UNDEFINED_BEHAVIOR:
//                 return "UNDEFINED BEHAVIOUR";
//                 break;
//
//             default:
//                 return "UNKNOWN";
//                 break;
//         }
//     }();
//
//     auto msg_severity = [severity]() {
//         switch (severity) {
//             case gl::DEBUG_SEVERITY_HIGH:
//                 return "HIGH";
//                 break;
//
//             case gl::DEBUG_SEVERITY_LOW:
//                 return "LOW";
//                 break;
//
//             case gl::DEBUG_SEVERITY_MEDIUM:
//                 return "MEDIUM";
//                 break;
//
//             case gl::DEBUG_SEVERITY_NOTIFICATION:
//                 return "NOTIFICATION";
//                 break;
//
//             default:
//                 return "UNKNOWN";
//                 break;
//         }
//     }();
//
//     XR_LOG_DEBUG("OpenGL debug message\n[MESSAGE] : {}\n[SOURCE] : {}\n[TYPE] : "
//                  "{}\n[SEVERITY] "
//                  ": {}\n[ID] : {}",
//                  message,
//                  msg_source,
//                  msg_type,
//                  msg_severity,
//                  id);
// }
//
// } // namespace app
//
}

struct ShaderComposer
{

    ShaderComposer& add_code(std::string_view code)
    {
        _src_code.append(code);
        return *this;
    }

    ShaderComposer& add_file(const std::filesystem::path& p)
    {
        fmt::format_to(back_inserter(_src_code), "#include {}", p);
        return *this;
    }

    ShaderComposer& add_define(std::string_view name, std::string_view value)
    {
        _defines.emplace_back(base::containers::string{ name, *_temp }, base::containers::string{ value, *_temp });
        return *this;
    }

    ShaderComposer& set_entry_point(std::string_view entrypoint)
    {
        _entrypoint = entrypoint;
        return *this;
    }

    ShaderComposer& set_compile_flags(const uint32_t flags)
    {
        _compile_options = flags;
        return *this;
    }

    explicit ShaderComposer(MemoryArena* a)
        : _src_code{ *a }
        , _defines{ *a }
        , _entrypoint{ *a }
    {
    }

    xray::rendering::ShaderBuildOptions to_sbo() const
    {
        return ShaderBuildOptions{
            .code_or_file_path = std::string_view{ _src_code },
            .entry_point = _entrypoint,
            .defines = std::span{ _defines },
            .compile_options = _compile_options,
        };
    }

    xray::base::containers::string _src_code;
    xray::base::containers::vector<std::pair<xray::base::containers::string, xray::base::containers::string>> _defines;
    xray::base::containers::string _entrypoint;
    MemoryArena* _temp;
    uint32_t _compile_options{ ShaderBuildOptions::Compile_GenerateDebugInfo };
};

int
main(int argc, char** argv)
{
    TracySetProgramName("XrayNG");

    xray::base::setup_logging(LogLevel::Debug);

    XR_LOG_INFO("Xray source commit: {}, built on {}, user {}, machine {}, working directory {}",
                xray::build::config::commit_hash_str,
                xray::build::config::build_date_time,
                xray::build::config::user_info,
                xray::build::config::machine_info,
                std::filesystem::current_path().generic_string());

    XR_LOG_INFO("Alignof {}", alignof(vec4f));

    auto arena_large_perm = GlobalMemorySystem::instance()->grab_large_arena();
    auto arena_temp = GlobalMemorySystem::instance()->grab_medium_arena();

    {
        // ScratchPadArena sa{ &arena_temp.arena };
        // XR_LOG_INFO("Gen shader: {}",
        //             ShaderComposer{ &arena_temp.arena }
        //                 .add_code(
        //                     R"#(
        //         #version 460 core
        //
        //         #include "core/bindless.core.glsl"
        //         )#")
        //                 .add_file("monka/phong.glsl")
        //                 ._src_code);
    }

    app::GameMain::create(&arena_large_perm.arena, &arena_temp.arena)
        .map([&](app::GameMain runner) {
            runner.run();
            XR_LOG_INFO("Temp arena stats: high water {}, largest block {}",
                        arena_temp.arena.stats.high_water,
                        arena_temp.arena.stats.largest_alloc);
            XR_LOG_INFO("Shutting down ...");
        })
        .map_error([](app::ProgramError&& f) {
            XR_LOG_CRITICAL(
                "{} ...",
                swl::visit(xray::base::VariantVisitor{
                               [](const VulkanError& vkerr) {
                                   return fmt::format("Vulkan error {} {}:{} ({:#0x})",
                                                      vkerr.function,
                                                      vkerr.file,
                                                      vkerr.line,
                                                      vkerr.err_code);
                               },
                               [](const xray::scene::SceneError& serr) { return serr.err; },
                               [](const app::MiscError& msc) { return msc.what; },
                               [](const GeometryImportError& ge) { return std::string{ "geometry error" }; } },
                           f));
        });

    return EXIT_SUCCESS;
}
