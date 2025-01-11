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

#include <cstdint>
#include <filesystem>
#include <vector>

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

#include "xray/base/app_config.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/delegate.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/base/random.hpp"
#include "xray/base/variant_visitor.hpp"
#include "xray/base/xray.misc.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/geometry.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/geometry.importer.gltf.hpp"
#include "xray/rendering/vertex_format/vertex.format.pbr.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.async.tasks.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pretty.print.hpp"
#include "xray/ui/window.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/user.interface.backend.hpp"
#include "xray/ui/user.interface.backend.vulkan.hpp"
#include "xray/ui/user_interface_render_context.hpp"

#include "xray/base/serialization/rfl.libconfig/config.save.hpp"
#include "xray/base/serialization/rfl.libconfig/config.load.hpp"
#include "xray/math/orientation.hpp"
#include "xray/math/scalar2_string_cast.hpp"
#include "xray/math/serialization/parser.scalar2.hpp"
#include "xray/math/serialization/parser.scalar3.hpp"
#include "xray/math/serialization/parser.scalar4.hpp"
#include "xray/math/serialization/parser.quaternion.hpp"
#include "xray/scene/light.types.hpp"
#include "xray/scene/scene.description.hpp"

#include "demo_base.hpp"
#include "init_context.hpp"
#include "triangle/triangle.hpp"
#include "bindless.pipeline.config.hpp"

using namespace xray;
using namespace xray::base;
using namespace xray::math;
using namespace xray::ui;
using namespace xray::rendering;
using namespace std;

xray::base::ConfigSystem* xr_app_config{ nullptr };

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

struct TaskSystem
{
    concurrencpp::runtime ccpp_runtime{};
    concurrencpp::result<void> vulkan_loader_task_handle{};

    TaskSystem() {}
    ~TaskSystem() {}

    concurrencpp::thread_executor* thread_exec() noexcept { return ccpp_runtime.thread_executor().get(); }

    concurrencpp::thread_pool_executor* thread_pool_exec() noexcept
    {
        return ccpp_runtime.thread_pool_executor().get();
    }
};

class MainRunner
{
  private:
    struct PrivateConstructToken
    {
        explicit PrivateConstructToken() = default;
    };

  public:
    MainRunner(PrivateConstructToken,
               unique_pointer<TaskSystem> task_sys,
               xray::ui::window window,
               xray::base::unique_pointer<xray::rendering::VulkanRenderer> vkrenderer,
               xray::base::unique_pointer<xray::ui::user_interface> ui,
               xray::base::unique_pointer<xray::ui::UserInterfaceRenderBackend_Vulkan> ui_backend,
               xray::base::unique_pointer<xray::rendering::DebugDrawSystem> debug_draw,

               // concurrencpp::result<tl::expected<GeometryWithRenderData, VulkanError>> loadbundle,
               // concurrencpp::result<tl::expected<GeneratedGeometryWithRenderData, VulkanError>> gen_objs,

               xray::rendering::BindlessUniformBufferResourceHandleEntryPair global_ubo)
        : _task_sys{ std::move(task_sys) }
        , _window{ std::move(window) }
        , _vkrenderer{ std::move(vkrenderer) }
        , _ui{ std::move(ui) }
        , _ui_backend{ std::move(ui_backend) }
        , _debug_draw{ std::move(debug_draw) } // , _registered_demos{ register_demos<dvk::TriangleDemo>() }

        // , _loadbundle{ std::move(loadbundle) }
        // , _genobjs{ std::move(gen_objs) }

        , _global_ubo{ global_ubo }
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

    MainRunner(MainRunner&& rhs) = default;
    ~MainRunner();

    static tl::optional<MainRunner> create();
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

    void draw(const xray::ui::window_loop_event& loop_evt);

    static concurrencpp::result<FontsLoadBundle> load_font_packages(concurrencpp::executor_tag,
                                                                    concurrencpp::thread_executor*);

    static concurrencpp::result<tl::expected<xray::rendering::LoadedGeometry, GeometryImportError>> load_geometry_task(
        concurrencpp::executor_tag,
        concurrencpp::thread_pool_executor*);

  private:
    xray::base::unique_pointer<TaskSystem> _task_sys;
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

    // concurrencpp::result<tl::expected<xray::rendering::GeometryWithRenderData, xray::rendering::VulkanError>>
    //     _loadbundle;
    // concurrencpp::result<tl::expected<xray::rendering::GeneratedGeometryWithRenderData,
    // xray::rendering::VulkanError>>
    //     _genobjs;

    xray::rendering::BindlessUniformBufferResourceHandleEntryPair _global_ubo;

    XRAY_NO_COPY(MainRunner);
};

MainRunner::~MainRunner() {}

concurrencpp::result<FontsLoadBundle>
MainRunner::load_font_packages(concurrencpp::executor_tag, concurrencpp::thread_executor*)
{
    XR_LOG_INFO("Load fonts IO work package added ...");
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

    XR_LOG_INFO("Load fonts IO work package done ...");
    co_return font_pkgs;
}

void
MainRunner::run()
{
    hookup_event_delegates();
    _window.message_loop();
    _vkrenderer->wait_device_idle();
}

struct EntityDrawableComponent
{
    std::string name;
    uint32_t hashed_name;
    uint32_t geometry_id;
    uint32_t material_id;
    OrientationF32 orientation;
};

struct ProceduralGeometryEntry
{
    std::string name;
    uint32_t hashed_name;
    vec2ui32 vertex_index_count;
    vec2ui32 buffer_offsets;
};

struct GeometryResourceTaskParams
{
    std::string_view resource_tag;
    concurrencpp::shared_result<VulkanRenderer*> renderer_result;
    std::span<const std::span<const uint8_t>> vertex_data;
    std::span<const std::span<const uint32_t>> index_data;
};

struct ProceduralGeometry
{
    vector<ProceduralGeometryEntry> procedural_geometries;
    vector<VkDrawIndexedIndirectCommand> draw_indirect_template;
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
};

struct SceneError
{
    std::string err;
};

struct GltfGeometryEntry
{
    std::string name;
    uint32_t hashed_name;
    vec2ui32 vertex_index_count;
    vec2ui32 buffer_offsets;
};

struct GltfGeometry
{
    std::vector<GltfGeometryEntry> entries;
    std::vector<VkDrawIndexedIndirectCommand> indirect_draw_templates;
    VulkanBuffer vertex_buffer;
    VulkanBuffer index_buffer;
};

using ProgramError = swl::variant<VulkanError, GeometryImportError, SceneError>;

#define XR_COR_PROPAGATE_ERROR(e)                                                                                      \
    do {                                                                                                               \
        if (!e) {                                                                                                      \
            co_return tl::make_unexpected(e.error());                                                                  \
        }                                                                                                              \
    } while (0)

concurrencpp::result<tl::expected<GltfGeometry, ProgramError>>
task_create_gltf_resources(concurrencpp::executor_tag,
                           concurrencpp::thread_pool_executor*,
                           concurrencpp::shared_result<VulkanRenderer*> renderer_result,
                           const std::span<const scene::GltfGeometryDescription> gltf_geometry_defs)
{
    using namespace xray::scene;

    vector<GltfGeometryEntry> gltf_geometries;
    vector<VkDrawIndexedIndirectCommand> indirect_draw_templates;
    vector<LoadedGeometry> loaded_gltfs;
    vec2ui32 global_vertex_index_count{ vec2ui32::stdc::zero };
    vector<vec2ui32> per_obj_vertex_index_counts;

    for (const GltfGeometryDescription& gltf : gltf_geometry_defs) {
        auto gltf_geometry = xray::rendering::LoadedGeometry::from_file(xr_app_config->model_path(gltf.path));
        XR_COR_PROPAGATE_ERROR(gltf_geometry);

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
            .hashed_name = FNV::fnv1a(gltf.name),
            .vertex_index_count = obj_vtx_idx_count,
            .buffer_offsets = global_vertex_index_count,
        });

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
                                 .name_tag = "vertex buffer (gltf)",
                                 .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 .bytes = buffer_bytes.x,
                             });
    XR_COR_PROPAGATE_ERROR(vertex_buffer);

    auto index_buffer =
        VulkanBuffer::create(*renderer,
                             VulkanBufferCreateInfo{
                                 .name_tag = "index buffer (gltf)",
                                 .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                 .bytes = buffer_bytes.y,
                             });
    XR_COR_PROPAGATE_ERROR(index_buffer);

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
    XR_COR_PROPAGATE_ERROR(cmd_buffer);

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
    XR_COR_PROPAGATE_ERROR(buffers_submit);

    XR_LOG_INFO("[[TASK]] - gltf geometry resources done...");

    co_return tl::expected<GltfGeometry, SceneError>{
        tl::in_place,
        std::move(gltf_geometries),
        std::move(indirect_draw_templates),
        std::move(*vertex_buffer),
        std::move(*index_buffer),
    };

    //
    // TODO: materials

    // vector<xray::rendering::ExtractedMaterialsWithImageSourcesBundle> material_data;
    // uint32_t image_bytes{};
    // uint32_t materials_bytes{};
    // for (const LoadedGeometry& ldgeom : loaded_gltfs) {
    //     material_data.emplace_back(ldgeom.extract_materials(0));
    //     const ExtractedMaterialsWithImageSourcesBundle* mtl_bundle = &material_data.back();
    //
    // }
    //
    // const size_t image_bytes = lz::chain(material_data.image_sources)
    //                                .map([](const ExtractedImageData& img) { return img.pixels.size_bytes(); })
    //                                .sum();
    //
    // const size_t material_def_bytes = material_data.materials.size() * sizeof(PBRMaterialDefinition);
}

struct GeneratedGeometryResources
{
    VulkanBuffer vertexbuffer;
    VulkanBuffer indexbuffer;
};

concurrencpp::result<tl::expected<GeneratedGeometryResources, ProgramError>>
task_create_procedural_geometry_render_resources(concurrencpp::executor_tag,
                                                 concurrencpp::thread_pool_executor*,
                                                 GeometryResourceTaskParams params)
{
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
                                 .name_tag = "[[{}]] - index buffer",
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

    XR_LOG_INFO("[[TASK]] - procedural geometry resources");
    co_return tl::expected<GeneratedGeometryResources, VulkanError>{
        tl::in_place,
        std::move(*vertexbuffer),
        std::move(*indexbuffer),
    };
}

struct ColoredMaterial
{
    std::string name;
    uint32_t hashed_name;
    uint32_t texel;
};

struct TexturedMaterial
{
    std::string name;
    uint32_t hashed_name;
    uint32_t ambient;
    uint32_t diffuse;
    uint32_t specular;
};

struct NonGltfMaterialsData
{
    std::vector<ColoredMaterial> materials_colored;
    std::vector<TexturedMaterial> materials_textured;
    VulkanImage color_texture;
    std::vector<VulkanImage> textures;
    VulkanBuffer sbo_materials_colored;
    VulkanBuffer sbo_materials_textured;
    GraphicsPipeline pipeline_ads_colored;
    uint32_t image_slot_start{};
    uint32_t sbo_slot_start{};
};

struct SceneDefinition
{
    GltfGeometry gltf;
    ProceduralGeometry procedural;
    VulkanBuffer instances_buffer;
    std::vector<EntityDrawableComponent> entities;
    NonGltfMaterialsData materials_nongltf;
};

struct SceneResources
{
    BindlessImageResourceHandleEntryPair color_tex;
    std::vector<BindlessImageResourceHandleEntryPair> materials_tex;
    BindlessStorageBufferResourceHandleEntryPair sbo_color_materials;
    BindlessStorageBufferResourceHandleEntryPair sbo_texture_materials;

    static SceneResources from_scene(SceneDefinition* sdef, VulkanRenderer* r)
    {
        BindlessSystem* bsys = &r->bindless_sys();

        auto def_sampler = bsys->get_sampler(
            VkSamplerCreateInfo{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .magFilter = VK_FILTER_LINEAR,
                .minFilter = VK_FILTER_LINEAR,
                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                .mipLodBias = 0.0f,
                .anisotropyEnable = false,
                .maxAnisotropy = 1.0f,
                .compareEnable = false,
                .compareOp = VK_COMPARE_OP_NEVER,
                .minLod = 0.0f,
                .maxLod = VK_LOD_CLAMP_NONE,
                .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalizedCoordinates = false,
            },
            *r);

        //
        // color texture added 1st so need to add 1 to slot
        vector<BindlessImageResourceHandleEntryPair> materials_tex{};
        for (uint32_t idx = 0, count = static_cast<uint32_t>(sdef->materials_nongltf.textures.size()); idx < count;
             ++idx) {
            materials_tex.push_back(bsys->add_image(std::move(sdef->materials_nongltf.textures[idx]),
                                                    *def_sampler,
                                                    sdef->materials_nongltf.image_slot_start + 1 + idx));
        }

        SceneResources scene_resources{
            .color_tex = bsys->add_image(std::move(sdef->materials_nongltf.color_texture),
                                         *def_sampler,
                                         sdef->materials_nongltf.image_slot_start),
            .materials_tex = std::move(materials_tex),
            .sbo_color_materials = bsys->add_storage_buffer(std::move(sdef->materials_nongltf.sbo_materials_colored),
                                                            sdef->materials_nongltf.sbo_slot_start + 0),
            .sbo_texture_materials = bsys->add_storage_buffer(std::move(sdef->materials_nongltf.sbo_materials_textured),
                                                              sdef->materials_nongltf.sbo_slot_start + 1),
        };

        //
        // transfer ownership to graphics queue
        r->queue_image_ownership_transfer(scene_resources.color_tex.first);
        for (const BindlessImageResourceHandleEntryPair& e : scene_resources.materials_tex) {
            r->queue_image_ownership_transfer(e.first);
        }

        XR_LOG_INFO("Scene resources created ...");
        return scene_resources;
    }
};

concurrencpp::result<tl::expected<NonGltfMaterialsData, ProgramError>>
task_create_non_gltf_materials(concurrencpp::executor_tag,
                               concurrencpp::thread_pool_executor*,
                               concurrencpp::shared_result<VulkanRenderer*> renderer_result,
                               const std::span<const scene::MaterialDescription> material_descriptions)
{
    using namespace xray::scene;

    vector<vec4f> color_pixels{}; // store 1024 colors max
    vector<ColoredMaterial> colored_materials;
    vector<std::filesystem::path> texture_files;
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

                    auto add_material_to_collection_fn = [](vector<std::filesystem::path>& material_coll,
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
    SmallVec<JobWaitToken> pending_jobs;

    XRAY_SCOPE_EXIT noexcept
    {
        const SmallVec<VkFence> fences{
            lz::chain(pending_jobs).map([](const JobWaitToken& token) { return token.fence(); }).to<SmallVec<VkFence>>()
        };

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
    // create the color texture
    tl::expected<VulkanImage, VulkanError> color_tex = [&]() -> tl::expected<VulkanImage, VulkanError> {
        auto transfer_job = renderer->create_job(QueueType::Transfer);
        XR_VK_PROPAGATE_ERROR(transfer_job);

        const auto pixels_span = to_bytes_span(color_pixels);
        auto color_tex = VulkanImage::from_memory(*renderer,
                                                  VulkanImageCreateInfo{
                                                      .tag_name = "color_texture",
                                                      .wpkg = *transfer_job,
                                                      .type = VK_IMAGE_TYPE_1D,
                                                      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                                                      .width = 1024,
                                                      .height = 1,
                                                      .pixels = std::span{ &pixels_span, 1 },
                                                  });

        XR_VK_PROPAGATE_ERROR(color_tex);

        auto wait_token = renderer->submit_job(*transfer_job, QueueType ::Transfer);
        XR_VK_PROPAGATE_ERROR(wait_token);

        pending_jobs.emplace_back(std::move(*wait_token));
        return color_tex;
    }();

    XR_VK_COR_PROPAGATE_ERROR(color_tex);

    const uint32_t image_count = static_cast<uint32_t>(texture_files.size() + 1);
    const uint32_t img_slot = renderer->bindless_sys().reserve_image_slots(image_count);
    const uint32_t sbo_slot = renderer->bindless_sys().reserve_sbo_slots(2);

    struct GPUColoredMaterial
    {
        uint32_t texel;
    };

    SmallVec<GPUColoredMaterial> gpu_colored_mtls;
    for (ColoredMaterial& cm : colored_materials) {
        gpu_colored_mtls.emplace_back(cm.texel);
    }

    struct GPUTexturedMaterial
    {
        uint32_t ambient;
        uint32_t diffuse;
        uint32_t specular;
    };
    SmallVec<GPUTexturedMaterial> gpu_textured_mtls;
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

    SmallVec<VulkanBuffer> material_sbos;
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

    //
    // graphics pipelines
    tl::expected<GraphicsPipeline, VulkanError> pipeline_ads_colored{
        GraphicsPipelineBuilder{}
            .add_shader(ShaderStage::Vertex, xr_app_config->shader_root() / "triangle/tri.vert")
            .add_shader(ShaderStage::Fragment, xr_app_config->shader_root() / "triangle/tri.frag")
            .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .rasterization_state({ .poly_mode = VK_POLYGON_MODE_FILL,
                                   .cull_mode = VK_CULL_MODE_BACK_BIT,
                                   .front_face = VK_FRONT_FACE_CLOCKWISE,
                                   .line_width = 1.0f })
            .create_bindless(*renderer),
    };
    XR_VK_COR_PROPAGATE_ERROR(pipeline_ads_colored);

    XR_LOG_INFO("[[TASK]] - non gltf materials done ...");

    co_return tl::expected<NonGltfMaterialsData, ProgramError>{
        tl::in_place,
        std::move(colored_materials),
        std::move(textured_materials),
        std::move(*color_tex),
        std::move(loaded_textures),
        std::move(material_sbos[0]),
        std::move(material_sbos[1]),
        std::move(*pipeline_ads_colored),
        img_slot,
        sbo_slot,
    };
}

concurrencpp::result<tl::expected<SceneDefinition, ProgramError>>
main_task(concurrencpp::executor_tag,
          concurrencpp::thread_pool_executor* tpe,
          concurrencpp::shared_result<VulkanRenderer*> renderer_result)
{
    using namespace xray::scene;
    const rfl::Result<SceneDescription> loaded_scene =
        rfl::libconfig::read<SceneDescription>(xr_app_config->config_path("scenes/simple.scene.conf"));

    if (!loaded_scene) {
        co_return tl::make_unexpected(
            SceneError{ .err = fmt::format("Failed to parse scene: {}", loaded_scene.error()->what()) });
    }

    const SceneDescription* scenedes{ &loaded_scene.value() };

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
        rfl::visit(
            [&](const auto& s) {
                using Name = typename std::decay_t<decltype(s)>::Name;
                if constexpr (std::is_same<Name, rfl::Literal<"grid">>()) {
                    const auto [cellsx, cellsy, width, height] = s.value();
                    XR_LOG_INFO(
                        "Generating grid {} -> cells {}x{}, dimensions {}x{}", pg.name, cellsx, cellsy, width, height);
                    geometry_data_t grid = geometry_factory::grid(cellsx, cellsy, width, height);

                    vertex_span.push_back(to_bytes_span(grid.vertex_span()));
                    index_span.push_back(grid.index_span());

                    draw_indirect_template.push_back(VkDrawIndexedIndirectCommand{
                        .indexCount = static_cast<uint32_t>(grid.index_count),
                        .instanceCount = 1,
                        .firstIndex = vtx_idx_accum.y,
                        .vertexOffset = static_cast<int32_t>(vtx_idx_accum.x),
                        .firstInstance = 0,
                    });

                    procedural_geometries.emplace_back(
                        pg.name, FNV::fnv1a(pg.name), vec2ui32{ grid.vertex_count, grid.index_count }, vtx_idx_accum);
                    gdata.push_back(std::move(grid));

                    vtx_idx_accum.x += grid.vertex_count;
                    vtx_idx_accum.y += grid.index_count;

                } else if constexpr (std::is_same<Name, rfl::Literal<"cone">>()) {
                    const auto [upper_radius, lower_radius, height, tess_factor_horz, tess_factor_vert] = s.value();
                    XR_LOG_INFO("Generating cone: upper radius {}, lower radius {}, height {}, tesselation "
                                "(vert/horz) {}/{}",
                                upper_radius,
                                lower_radius,
                                height,
                                tess_factor_vert,
                                tess_factor_horz);

                    geometry_data_t cone = geometry_factory::conical_section(
                        upper_radius, lower_radius, height, tess_factor_horz, tess_factor_vert);

                    vertex_span.push_back(to_bytes_span(cone.vertex_span()));
                    index_span.push_back(cone.index_span());

                    draw_indirect_template.push_back(VkDrawIndexedIndirectCommand{
                        .indexCount = static_cast<uint32_t>(cone.index_count),
                        .instanceCount = 1,
                        .firstIndex = vtx_idx_accum.y,
                        .vertexOffset = static_cast<int32_t>(vtx_idx_accum.x),
                        .firstInstance = 0,
                    });

                    procedural_geometries.emplace_back(
                        pg.name, FNV::fnv1a(pg.name), vec2ui32{ cone.vertex_count, cone.index_count }, vtx_idx_accum);
                    gdata.push_back(std::move(cone));

                    vtx_idx_accum.x += cone.vertex_count;
                    vtx_idx_accum.y += cone.index_count;

                } else if constexpr (std::is_same<Name, rfl::Literal<"torus">>()) {
                    const auto [outer_radius, inner_radius, rings, sides] = s.value();
                    XR_LOG_INFO("Generating torus: outer {}, inner {}, sides {}, rings {}",
                                outer_radius,
                                inner_radius,
                                sides,
                                rings);

                    geometry_data_t torus = geometry_factory::torus(outer_radius, inner_radius, sides, rings);

                    vertex_span.push_back(to_bytes_span(torus.vertex_span()));
                    index_span.push_back(torus.index_span());

                    draw_indirect_template.push_back(VkDrawIndexedIndirectCommand{
                        .indexCount = static_cast<uint32_t>(torus.index_count),
                        .instanceCount = 1,
                        .firstIndex = vtx_idx_accum.y,
                        .vertexOffset = static_cast<int32_t>(vtx_idx_accum.x),
                        .firstInstance = 0,
                    });

                    procedural_geometries.emplace_back(
                        pg.name, FNV::fnv1a(pg.name), vec2ui32{ torus.vertex_count, torus.index_count }, vtx_idx_accum);
                    gdata.push_back(std::move(torus));

                    vtx_idx_accum.x += torus.vertex_count;
                    vtx_idx_accum.y += torus.index_count;

                } else {
                    static_assert(rfl::always_false_v<ProceduralGeometryDescription>, "Not all cases were covered.");
                }
            },
            pg.gen_params);
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
        const uint32_t geometry_id = FNV::fnv1a(pe.geometry);
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
            .material_id = FNV::fnv1a(pe.material),
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
            .geometry_id = geometry_id,
            .material_id = 0,
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

    auto procedural_render_resource = co_await procedural_render_resources_result;
    XR_COR_PROPAGATE_ERROR(procedural_render_resource);

    auto gltf_render_resources = co_await gltf_render_resources_result;
    XR_COR_PROPAGATE_ERROR(gltf_render_resources);

    auto non_gltf_materials = co_await non_gltf_materials_task_result;
    XR_COR_PROPAGATE_ERROR(non_gltf_materials);

    XR_LOG_INFO("[[TASK]] - main done ...");

    co_return SceneDefinition{
        .gltf = std::move(*gltf_render_resources),
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
    };
}

tl::optional<MainRunner>
MainRunner::create()
{
    using namespace xray::ui;
    using namespace xray::base;

    xray::base::setup_logging(LogLevel::Debug);

    XR_LOG_INFO("Xray source commit: {}, built on {}, user {}, machine {}, working directory {}",
                xray::build::config::commit_hash_str,
                xray::build::config::build_date_time,
                xray::build::config::user_info,
                xray::build::config::machine_info,
                std::filesystem::current_path().generic_string());

    unique_pointer<TaskSystem> task_sys{ base::make_unique<TaskSystem>() };

    static ConfigSystem app_cfg{ "config/app_config.conf" };
    xr_app_config = &app_cfg;

    //
    // start font loading
    concurrencpp::result<FontsLoadBundle> font_pkg_load_result =
        MainRunner::load_font_packages(concurrencpp::executor_tag{}, task_sys->thread_exec());

    concurrencpp::result_promise<VulkanRenderer*> renderer_promise{};

    //
    // start model and generated geometry
    concurrencpp::shared_result<VulkanRenderer*> renderer_result{ renderer_promise.get_result() };
    auto main_task_result = main_task(concurrencpp::executor_tag{}, task_sys->thread_pool_exec(), renderer_result);

    const window_params_t wnd_params{ "Vulkan Demo", 4, 5, 24, 8, 32, 0, 1, false };
    window main_window{ wnd_params };

    if (!main_window) {
        XR_LOG_ERR("Failed to initialize application window!");
        return tl::nullopt;
    }

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
        )
    };

    if (!opt_renderer) {
        XR_LOG_CRITICAL("Failed to create Vulkan renderer!");
        return tl::nullopt;
    }

    auto renderer = xray::base::make_unique<VulkanRenderer>(std::move(*opt_renderer));
    renderer->add_shader_include_directories({ xr_app_config->shader_root() });

    //
    // resume anyone waiting for the renderer
    renderer_promise.set_result(raw_ptr(renderer));

    auto debug_draw = DebugDrawSystem::create(DebugDrawSystem::InitContext{ &*renderer });
    if (!debug_draw) {
        XR_LOG_CRITICAL("Failed to create debug draw backend");
        return tl::nullopt;
    }

    const VulkanBufferCreateInfo bci{
        .name_tag = "UBO - FrameGlobalData",
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .bytes = sizeof(FrameGlobalData),
        .frames = renderer->buffering_setup().buffers,
        .initial_data = {},
    };

    auto g_ubo{ VulkanBuffer::create(*renderer, bci) };
    if (!g_ubo)
        return tl::nullopt;

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

    tl::expected<UserInterfaceRenderBackend_Vulkan, VulkanError> vk_backend{ UserInterfaceRenderBackend_Vulkan::create(
        *renderer, ui->render_backend_create_info()) };

    if (!vk_backend) {
        XR_LOG_CRITICAL("Failed to create Vulkan render backed for UI!");
        return tl::nullopt;
    }

    ui->set_global_font("TerminessNerdFontMono-Regular");

    const BindlessUniformBufferResourceHandleEntryPair g_ubo_handles =
        renderer->bindless_sys().add_chunked_uniform_buffer(std::move(*g_ubo), renderer->buffering_setup().buffers);

    tl::expected<SceneDefinition, ProgramError> scene_result = main_task_result.get();
    if (!scene_result) {
        XR_LOG_ERR("Failed to create scene ");
        return tl::nullopt;
    }

    SceneResources scene_resources{ SceneResources::from_scene(&*scene_result, raw_ptr(renderer)) };

    return tl::make_optional<MainRunner>(
        PrivateConstructToken{},
        std::move(task_sys),
        std::move(main_window),
        std::move(renderer),
        std::move(ui),
        xray::base::make_unique<UserInterfaceRenderBackend_Vulkan>(std::move(*vk_backend)),
        xray::base::make_unique<DebugDrawSystem>(std::move(*debug_draw)),
        g_ubo_handles);
}

void
MainRunner::demo_quit()
{
    assert(demo_running());
    _vkrenderer->wait_device_idle();
    _demo = nullptr;
}

void
MainRunner::poll_start(const xray::ui::poll_start_event&)
{
}

void
MainRunner::poll_end(const xray::ui::poll_end_event&)
{
}

void
MainRunner::hookup_event_delegates()
{
    _window.core.events.loop = cpp::bind<&MainRunner::loop_event>(this);
    _window.core.events.poll_start = cpp::bind<&MainRunner::poll_start>(this);
    _window.core.events.poll_end = cpp::bind<&MainRunner::poll_end>(this);
    _window.core.events.window = cpp::bind<&MainRunner::event_handler>(this);
}

void
MainRunner::event_handler(const xray::ui::window_event& wnd_evt)
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
MainRunner::loop_event(const xray::ui::window_loop_event& loop_event)
{
    static bool doing_ur_mom{ false };
    if (!doing_ur_mom && !_demo) {
        // _demo = dvk::TriangleDemo::create(
        //     app::init_context_t{
        //         .surface_width = _window.width(),
        //         .surface_height = _window.height(),
        //         .cfg = xr_app_config,
        //         .ui = raw_ptr(_ui),
        //         .renderer = raw_ptr(_vkrenderer),
        //         .quit_receiver = cpp::bind<&MainRunner::demo_quit>(this),
        //     },
        //     std::move(_loadbundle),
        //     std::move(_genobjs));
        doing_ur_mom = true;
    }

    _timer.update_and_reset();
    _ui->tick(_timer.elapsed_millis());
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
            loop_event,
            &frd,
            raw_ptr(_vkrenderer),
            xray::base::raw_ptr(_ui),
            g_ubo_mapping->as<FrameGlobalData>(),
            raw_ptr(_debug_draw),
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
            _ui_backend->render(ui_render_ctx, *_vkrenderer, frd);
        });

    _vkrenderer->end_rendering();
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

// struct Scene
// {
//     struct Entity
//     {
//         int phys_handle;
//         int geom_handle;
//         int mtl_handle;
//     };
//
//     // std::vector<Orientation> instances_phys;
//
//     static void write_test_scene_definition(const std::filesystem::path& scene_file);
//     static tl::expected<Scene, SceneError> from_file(const std::filesystem::path& path,
//                                                      VulkanRenderer* r,
//                                                      app::TaskSystem* ts);
//
//     // TODO: maybe geometry factory can output the data directly into the staging buffers
//     static concurrencpp::result<tl::expected<GeneratedGeometryResources, VulkanError>>
//     create_generated_geometry_render_resources_task(concurrencpp::executor_tag,
//                                                     concurrencpp::thread_pool_executor* tpe,
//                                                     const GeometryResourceTaskParams params);
// };
//
// concurrencpp::result<tl::expected<GeneratedGeometryResources, VulkanError>>
// Scene::create_generated_geometry_render_resources_task(concurrencpp::executor_tag,
//                                                        concurrencpp::thread_pool_executor* tpe,
//                                                        const GeometryResourceTaskParams params)
// {
//     const size_t vertex_bytes =
//         lz::chain(params.vertex_data).map([](const std::span<const uint8_t> sv) { return sv.size_bytes(); }).sum();
//     const size_t index_bytes =
//         lz::chain(params.index_data).map([](const std::span<const uint32_t> si) { return si.size_bytes(); }).sum();
//
//     std::array<char, 256> scratch_buffer;
//     auto out = fmt::format_to_n(
//         scratch_buffer.data(), scratch_buffer.size() - 1, "[[{}]] - vertex buffer", params.resource_tag);
//     *out.out = 0;
//
//     auto vertexbuffer =
//         VulkanBuffer::create(*params.renderer,
//                              VulkanBufferCreateInfo{
//                                  .name_tag = scratch_buffer.data(),
//                                  .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//                                  .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                  .bytes = vertex_bytes,
//                              });
//
//     XR_VK_COR_PROPAGATE_ERROR(vertexbuffer);
//
//     out = fmt::format_to_n(
//         scratch_buffer.data(), scratch_buffer.size() - 1, "[[{}]] - index buffer", params.resource_tag);
//     *out.out = 0;
//
//     auto indexbuffer =
//         VulkanBuffer::create(*params.renderer,
//                              VulkanBufferCreateInfo{
//                                  .name_tag = "[[{}]] - index buffer",
//                                  .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//                                  .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                  .bytes = index_bytes,
//                              });
//
//     XR_VK_COR_PROPAGATE_ERROR(indexbuffer);
//
//     uintptr_t staging_buff_offset = params.renderer->reserve_staging_buffer_memory(vertex_bytes + index_bytes);
//     itlib::small_vector<VkBufferCopy> cpy_regions_vtx;
//
//     uintptr_t dst_buff_offset = 0;
//     for (const std::span<const uint8_t> sv : params.vertex_data) {
//         memcpy(reinterpret_cast<void*>(params.renderer->staging_buffer_memory() + staging_buff_offset),
//                sv.data(),
//                sv.size_bytes());
//         cpy_regions_vtx.push_back(VkBufferCopy{
//             .srcOffset = staging_buff_offset,
//             .dstOffset = dst_buff_offset,
//             .size = sv.size_bytes(),
//         });
//
//         staging_buff_offset += sv.size_bytes();
//         dst_buff_offset += sv.size_bytes();
//     }
//
//     const size_t index_copy_rgn = cpy_regions_vtx.size();
//
//     uintptr_t idx_buffer_offset{};
//     for (const std::span<const uint32_t> si : params.index_data) {
//         memcpy(reinterpret_cast<void*>(params.renderer->staging_buffer_memory() + staging_buff_offset),
//                si.data(),
//                si.size_bytes());
//         cpy_regions_vtx.push_back(VkBufferCopy{
//             .srcOffset = staging_buff_offset,
//             .dstOffset = idx_buffer_offset,
//             .size = si.size_bytes(),
//         });
//
//         staging_buff_offset += si.size_bytes();
//         idx_buffer_offset += si.size_bytes();
//     }
//
//     auto cmd_buf = params.renderer->create_job(QueueType::Transfer);
//     XR_VK_COR_PROPAGATE_ERROR(cmd_buf);
//
//     vkCmdCopyBuffer(*cmd_buf,
//                     params.renderer->staging_buffer(),
//                     vertexbuffer->buffer_handle(),
//                     (uint32_t)index_copy_rgn,
//                     cpy_regions_vtx.data());
//     vkCmdCopyBuffer(*cmd_buf,
//                     params.renderer->staging_buffer(),
//                     indexbuffer->buffer_handle(),
//                     (uint32_t)cpy_regions_vtx.size() - index_copy_rgn,
//                     &cpy_regions_vtx[index_copy_rgn]);
//
//     auto wait_token = params.renderer->submit_job(*cmd_buf, QueueType::Transfer);
//     XR_VK_COR_PROPAGATE_ERROR(wait_token);
//
//     co_return tl::expected<GeneratedGeometryResources, VulkanError>{ tl::in_place,
//                                                                      std::move(*vertexbuffer),
//                                                                      std::move(*indexbuffer) };
// }
//
// struct MaterialEntry
// {
//     std::string name;
//     uint32_t hashed_name;
// };
//
// tl::expected<Scene, SceneError>
// Scene::from_file(const std::filesystem::path& path, VulkanRenderer* r, app::TaskSystem* ts)
// {
//     // const rfl::Result<SceneDescription> loaded_scene = rfl::libconfig::read<SceneDescription>(path);
//     // if (!loaded_scene)
//     //     return tl::make_unexpected(SceneError{ .err = loaded_scene.error()->what() });
//
//     vector<ProceduralGeometryEntry> procedural_geometries;
//     SmallVec<geometry_data_t> gdata;
//     vector<VkDrawIndexedIndirectCommand> draw_indirect_template;
//     SmallVec<span<const uint8_t>> vertex_span;
//     SmallVec<span<const uint32_t>> index_span;
//     vec2ui32 vtx_idx_accum{};
//
//     //
//     // procedurally generated shapes
//     // for (const ProceduralGeometryDescription& pg : loaded_scene.value().procedural_geometries) {
//     //     rfl::visit(
//     //         [&](const auto& s) {
//     //             using Name = typename std::decay_t<decltype(s)>::Name;
//     //             if constexpr (std::is_same<Name, rfl::Literal<"grid">>()) {
//     //                 const auto [cellsx, cellsy, width, height] = s.value();
//     //                 XR_LOG_INFO(
//     //                     "Generating grid {} -> cells {}x{}, dimensions {}x{}", pg.name, cellsx, cellsy, width,
//     //                     height);
//     //                 geometry_data_t grid = geometry_factory::grid(cellsx, cellsy, width, height);
//     //
//     //                 vertex_span.push_back(to_bytes_span(grid.vertex_span()));
//     //                 index_span.push_back(grid.index_span());
//     //
//     //                 draw_indirect_template.push_back(VkDrawIndexedIndirectCommand{
//     //                     .indexCount = static_cast<uint32_t>(grid.index_count),
//     //                     .instanceCount = 1,
//     //                     .firstIndex = vtx_idx_accum.y,
//     //                     .vertexOffset = static_cast<int32_t>(vtx_idx_accum.x),
//     //                     .firstInstance = 0,
//     //                 });
//     //
//     //                 vtx_idx_accum.x += grid.vertex_count;
//     //                 vtx_idx_accum.y += grid.index_count;
//     //
//     //                 procedural_geometries.emplace_back(pg.name, FNV::fnv1a(pg.name));
//     //                 gdata.push_back(std::move(grid));
//     //
//     //             } else if constexpr (std::is_same<Name, rfl::Literal<"cone">>()) {
//     //                 const auto [upper_radius, lower_radius, height, tess_factor_horz, tess_factor_vert] =
//     s.value();
//     //                 XR_LOG_INFO(
//     //                     "Generating cone: upper radius {}, lower radius {}, height {}, tesselation (vert/horz)
//     //                     {}/{}", upper_radius, lower_radius, height, tess_factor_vert, tess_factor_horz);
//     //
//     //                 geometry_data_t cone = geometry_factory::conical_section(
//     //                     upper_radius, lower_radius, height, tess_factor_horz, tess_factor_vert);
//     //
//     //                 vertex_span.push_back(to_bytes_span(cone.vertex_span()));
//     //                 index_span.push_back(cone.index_span());
//     //
//     //                 draw_indirect_template.push_back(VkDrawIndexedIndirectCommand{
//     //                     .indexCount = static_cast<uint32_t>(cone.index_count),
//     //                     .instanceCount = 1,
//     //                     .firstIndex = vtx_idx_accum.y,
//     //                     .vertexOffset = static_cast<int32_t>(vtx_idx_accum.x),
//     //                     .firstInstance = 0,
//     //                 });
//     //
//     //                 vtx_idx_accum.x += cone.vertex_count;
//     //                 vtx_idx_accum.y += cone.index_count;
//     //
//     //                 procedural_geometries.emplace_back(pg.name, FNV::fnv1a(pg.name));
//     //                 gdata.push_back(std::move(cone));
//     //
//     //             } else if constexpr (std::is_same<Name, rfl::Literal<"torus">>()) {
//     //                 const auto [outer_radius, inner_radius, rings, sides] = s.value();
//     //                 XR_LOG_INFO("Generating torus: outer {}, inner {}, sides {}, rings {}",
//     //                             outer_radius,
//     //                             inner_radius,
//     //                             sides,
//     //                             rings);
//     //
//     //                 geometry_data_t torus = geometry_factory::torus(outer_radius, inner_radius, sides, rings);
//     //
//     //                 vertex_span.push_back(to_bytes_span(torus.vertex_span()));
//     //                 index_span.push_back(torus.index_span());
//     //
//     //                 draw_indirect_template.push_back(VkDrawIndexedIndirectCommand{
//     //                     .indexCount = static_cast<uint32_t>(torus.index_count),
//     //                     .instanceCount = 1,
//     //                     .firstIndex = vtx_idx_accum.y,
//     //                     .vertexOffset = static_cast<int32_t>(vtx_idx_accum.x),
//     //                     .firstInstance = 0,
//     //                 });
//     //
//     //                 vtx_idx_accum.x += torus.vertex_count;
//     //                 vtx_idx_accum.y += torus.index_count;
//     //
//     //                 procedural_geometries.emplace_back(pg.name, FNV::fnv1a(pg.name));
//     //                 gdata.push_back(std::move(torus));
//     //
//     //             } else {
//     //                 static_assert(rfl::always_false_v<ProceduralGeometryDescription>, "Not all cases were
//     covered.");
//     //             }
//     //         },
//     //         pg.gen_params);
//     // }
//     //
//     // auto procedural_render_resources =
//     //     create_generated_geometry_render_resources_task(concurrencpp::executor_tag{},
//     //                                                     ts->thread_pool_exec(),
//     //                                                     GeometryResourceTaskParams{
//     //                                                         .resource_tag = "generated geometry",
//     //                                                         .renderer = r,
//     //                                                         .vertex_data = std::span{ vertex_span },
//     //                                                         .index_data = std::span{ index_span },
//     //                                                     });
//     //
//     // vector<ProceduralEntity2> proc_entities;
//     // for (const ProceduralEntityDescription& pe : loaded_scene.value().procedural_entities) {
//     //     const uint32_t geometry_id = FNV::fnv1a(pe.geometry);
//     //     auto itr_geometry =
//     //         std::find_if(cbegin(procedural_geometries),
//     //                      cend(procedural_geometries),
//     //                      [geometry_id](const ProceduralGeometry& pg) { return pg.hashed_name == geometry_id; });
//     //
//     //     if (itr_geometry == std::cend(procedural_geometries)) {
//     //         return tl::make_unexpected(
//     //             SceneError{ .err = fmt::format("Entity {} uses undefined geometry {}", pe.name, pe.geometry) });
//     //     }
//     //
//     //     const uint32_t entity_id = FNV::fnv1a(pe.name);
//     //     proc_entities.push_back(ProceduralEntity2{
//     //         .name = pe.name,
//     //         .hashed_name = FNV::fnv1a(pe.name),
//     //         .geometry_id = geometry_id,
//     //         .material_id = FNV::fnv1a(pe.material),
//     //         .orientation = pe.orientation.value_or(OrientationF32{}),
//     //     });
//     //
//     //     XR_LOG_INFO("Procedural entity {} {}", pe.name, pe.material);
//     // }
//     //
//     // assert(procedural_geometries.size() == gdata.size());
//     // assert(gdata.size() == draw_indirect_template.size());
//     //
//     // auto res = procedural_render_resources.get();
//     //
//     // for (size_t i = 0, count = procedural_geometries.size(); i < count; ++i) {
//     //     const ProceduralGeometry* pg = &procedural_geometries[i];
//     //     XR_LOG_INFO("Procedural geometry {}({})", pg->name, pg->hashed_name);
//     // }
//     //
//     return tl::make_unexpected(SceneError{ .err = "Not implemented yet" });
// }

int
main(int argc, char** argv)
{
    app::MainRunner::create().map_or_else(
        [](app::MainRunner runner) {
            runner.run();
            XR_LOG_INFO("Shutting down ...");
        },
        []() { XR_LOG_CRITICAL("Failure, shutting down ..."); });

    return EXIT_SUCCESS;
}
