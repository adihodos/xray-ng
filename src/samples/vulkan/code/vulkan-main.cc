//
// Copyright (c) 2011-2016 Adrian Hodos
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
#include <mutex>
#include <condition_variable>

#include <fmt/core.h>
#include <fmt/std.h>
#include <concurrencpp/concurrencpp.h>
#include <Lz/Lz.hpp>
#include <swl/variant.hpp>
#include <mio/mmap.hpp>
#include <oneapi/tbb/concurrent_priority_queue.h>

#include "xray/base/app_config.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/delegate.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/base/variant_visitor.hpp"
#include "xray/base/xray.misc.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/debug_draw.hpp"
#include "xray/rendering/geometry.hpp"
#include "xray/rendering/geometry.importer.gltf.hpp"
#include "xray/rendering/vertex_format/vertex.format.pbr.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.async.tasks.hpp"
#include "xray/ui/window.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/user.interface.backend.hpp"
#include "xray/ui/user.interface.backend.vulkan.hpp"
#include "xray/ui/user_interface_render_context.hpp"

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

struct VkAsyncThreadMessage_Setup
{
    VulkanRenderer* renderer;
    VkDevice device;
    VkQueue graphics;
    VkQueue transfer;
    uint32_t graphics_idx;
    uint32_t transfer_idx;
    VkCommandPool transfer_cmdpool;
};

struct VkAsyncThreadMessage_ProcessGeometry
{
    std::shared_ptr<dvk::LoadedGeometryBundle> geometry;
    std::string name_tag;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags memory_properties;
};

struct VkAsyncThreadMessage_ProcessImage
{};

struct VkAsyncThreadMessage_Quit
{};

using VkAsyncThreadMessage = swl::variant<VkAsyncThreadMessage_ProcessGeometry,
                                          VkAsyncThreadMessage_ProcessImage,
                                          VkAsyncThreadMessage_Setup,
                                          VkAsyncThreadMessage_Quit>;

struct VkAsyncUploadThread
{
    void main_loop();

    void send_message(VkAsyncThreadMessage msg)
    {
        {
            unique_lock lock{ msg_queue_mtx };
            msg_queue.push_back(std::move(msg));
        }
        msg_queue_cvar.notify_one();
    }

    void process_geometry_request(VkCommandBuffer cmd_buf, const VkAsyncThreadMessage_ProcessGeometry& g);
    void process_image_request(VkCommandBuffer cmd_buf, const VkAsyncThreadMessage_ProcessImage& img);

    VkCommandBuffer get_one_command_buffer();
    VkSemaphore get_one_semaphore();

    std::mutex msg_queue_mtx;
    std::condition_variable msg_queue_cvar;
    std::vector<VkAsyncThreadMessage> msg_queue;

    struct PendingGeometryOperation
    {
        VulkanBuffer vtx;
        VulkanBuffer idx;
        std::vector<VulkanImage> images;
        shared_ptr<dvk::LoadedGeometryBundle> g;
    };

    using PendingOpData = swl::variant<PendingGeometryOperation, VulkanImage>;

    struct PendingOp
    {
        VkCommandBuffer cmd_buf;
        VkSemaphore semaphore;
        std::vector<PendingOpData> opdata;
    };

    struct VulkanState
    {
        VulkanRenderer* renderer;
        VkQueue transfer_queue;
        VkCommandPool cmdpool;
        uint32_t queue_idx;
        VulkanBuffer staging_buffer;
        UniqueMemoryMapping mapped_staging_buffer;
        uintptr_t free_offset{};
        vector<VkCommandBuffer> cmd_buffers_free;
        vector<VkCommandBuffer> cmd_buffers_pending;
        vector<VkSemaphore> semaphores_free;
        vector<VkSemaphore> semaphores_avail;
        vector<PendingOp> pending_ops;
    };

    unique_pointer<VulkanState> vkstate;
};

void
VkAsyncUploadThread::main_loop()
{
    vector<VkAsyncThreadMessage> drained_messages;
    vector<VkAsyncThreadMessage> pending_loads;

    for (bool quit = false; !quit;) {
        //
        // drain message queue
        {
            unique_lock<std::mutex> lock{ msg_queue_mtx };
            msg_queue_cvar.wait(lock, [this]() { return !msg_queue.empty(); });
            drained_messages = std::move(msg_queue);
        }

        for (VkAsyncThreadMessage& this_msg : drained_messages) {
            swl::visit(VariantVisitor{ [this](const VkAsyncThreadMessage_Setup& s) {
                                          XR_LOG_INFO("VkAsyncThread - Setup: device {:p}, transfer queue {:p}, id {}",
                                                      (const void*)s.device,
                                                      (const void*)s.transfer,
                                                      s.transfer_idx);

                                          assert(vkstate == nullptr);
                                          auto staging_bugger = VulkanBuffer::create(
                                              *s.renderer,
                                              VulkanBufferCreateInfo{
                                                  .name_tag = "Vulkan async staging buffer",
                                                  .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                  .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                  .bytes = 64 * 1024 * 1024,
                                              });

                                          if (!staging_bugger)
                                              return;

                                          auto mapped_staging_buffer =
                                              staging_bugger->mmap(s.renderer->device(), tl::nullopt);
                                          if (!mapped_staging_buffer)
                                              return;

                                          auto [queue, queue_index, queue_pool] = s.renderer->queue_data(1);
                                          vkstate = base::make_unique<VulkanState>(s.renderer,
                                                                                   queue,
                                                                                   queue_pool,
                                                                                   queue_index,
                                                                                   std::move(*staging_bugger),
                                                                                   std::move(*mapped_staging_buffer),
                                                                                   0);
                                      },
                                       [&](VkAsyncThreadMessage_ProcessGeometry& g) {
                                           XR_LOG_INFO("VkAsyncThread - process geometry message");
                                           pending_loads.push_back(std::move(g));
                                       },
                                       [&](VkAsyncThreadMessage_ProcessImage& i) {
                                           XR_LOG_INFO("VkAsyncThread - process image message");
                                           pending_loads.push_back(std::move(i));
                                       },
                                       [&](const VkAsyncThreadMessage_Quit) {
                                           XR_LOG_INFO("VkAsyncThread - quit message");
                                           quit = true;
                                       } },
                       this_msg);

            if (quit)
                break;
        }

        if (!quit && vkstate && !pending_loads.empty()) {
            VkCommandBuffer cmd_buff = get_one_command_buffer();

            const VkCommandBufferBeginInfo cmd_buf_begin_info{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext = nullptr,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                .pInheritanceInfo = nullptr,
            };
            vkBeginCommandBuffer(cmd_buff, &cmd_buf_begin_info);
            for (const VkAsyncThreadMessage& msg : pending_loads) {
                swl::visit(
                    VariantVisitor{
                        [&](const VkAsyncThreadMessage_ProcessGeometry& g) { process_geometry_request(cmd_buff, g); },
                        [&](const VkAsyncThreadMessage_ProcessImage& i) { process_image_request(cmd_buff, i); },
                        [](const VkAsyncThreadMessage_Quit) { assert(false && "should not be here"); },
                        [](const VkAsyncThreadMessage_Setup) { assert(false && "should not be here"); } },
                    msg);
            }
            vkEndCommandBuffer(cmd_buff);

            VkSemaphore sem = get_one_semaphore();
            const VkSubmitInfo submit_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                            .pNext = nullptr,
                                            .waitSemaphoreCount = 0,
                                            .pWaitSemaphores = nullptr,
                                            .pWaitDstStageMask = nullptr,
                                            .commandBufferCount = 1,
                                            .pCommandBuffers = &cmd_buff,
                                            .signalSemaphoreCount = 1,
                                            .pSignalSemaphores = &sem };
            const VkResult submit_result =
                WRAP_VULKAN_FUNC(vkQueueSubmit, vkstate->transfer_queue, 1, &submit_info, nullptr);
            assert(submit_result == VK_SUCCESS);

            pending_loads.clear();
        }
    }
}

void
VkAsyncUploadThread::process_geometry_request(VkCommandBuffer cmd_buff, const VkAsyncThreadMessage_ProcessGeometry& g)
{
    assert(vkstate);

    auto g_buffer =
        VulkanBuffer::create(*vkstate->renderer,
                             VulkanBufferCreateInfo{ .name_tag = g.name_tag.c_str(),
                                                     .usage = g.usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                     .memory_properties = g.memory_properties,
                                                     .bytes = container_bytes_size(g.geometry->vertex_mem) });

    if (!g_buffer) {
        return;
    }

    memcpy(vkstate->mapped_staging_buffer.as<uint8_t>() + vkstate->free_offset,
           g.geometry->vertex_mem.data(),
           container_bytes_size(g.geometry->vertex_mem));

    const VkBufferCopy copy_rgn_g{ .srcOffset = vkstate->free_offset,
                                   .dstOffset = 0,
                                   .size = container_bytes_size(g.geometry->vertex_mem) };

    vkstate->free_offset +=
        base::align<uintptr_t>(container_bytes_size(g.geometry->vertex_mem), vkstate->staging_buffer.alignment);

    vkCmdCopyBuffer(cmd_buff, vkstate->staging_buffer.buffer_handle(), g_buffer->buffer_handle(), 1, &copy_rgn_g);

    const auto i_buffer_bytes = container_bytes_size(g.geometry->index_mem);
    auto i_buffer = VulkanBuffer::create(
        *vkstate->renderer,
        VulkanBufferCreateInfo{ .name_tag = g.name_tag.c_str(),
                                .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                .bytes = i_buffer_bytes });

    if (!i_buffer) {
        return;
    }

    memcpy(vkstate->mapped_staging_buffer.as<uint8_t>() + vkstate->free_offset,
           g.geometry->index_mem.data(),
           i_buffer_bytes);

    const VkBufferCopy copy_rgn_i{ .srcOffset = vkstate->free_offset, .dstOffset = 0, .size = i_buffer_bytes };

    vkstate->free_offset += base::align<uintptr_t>(i_buffer_bytes, vkstate->staging_buffer.alignment);
    vkCmdCopyBuffer(cmd_buff, vkstate->staging_buffer.buffer_handle(), i_buffer->buffer_handle(), 1, &copy_rgn_i);
}

void
VkAsyncUploadThread::process_image_request(VkCommandBuffer cmd_buff, const VkAsyncThreadMessage_ProcessImage& img)
{
    assert(vkstate);
}

VkCommandBuffer
VkAsyncUploadThread::get_one_command_buffer()
{
    assert(vkstate);
    if (vkstate->cmd_buffers_free.empty()) {
        //
        // no avail buffers, allocate more
        VkCommandBuffer buffers[16];
        const VkCommandBufferAllocateInfo alloc_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                         .pNext = nullptr,
                                                         .commandPool = vkstate->cmdpool,
                                                         .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                         .commandBufferCount = static_cast<uint32_t>(size(buffers)) };
        const VkResult alloc_cmdbuffs_res =
            WRAP_VULKAN_FUNC(vkAllocateCommandBuffers, vkstate->renderer->device(), &alloc_info, buffers);
        if (alloc_cmdbuffs_res != VK_SUCCESS) {
            return nullptr;
        }

        vkstate->cmd_buffers_free.insert(vkstate->cmd_buffers_free.end(), cbegin(buffers), cend(buffers));
    }

    VkCommandBuffer buf = vkstate->cmd_buffers_free.back();
    vkstate->cmd_buffers_free.pop_back();
    vkstate->cmd_buffers_pending.push_back(buf);
    return buf;
}

VkSemaphore
VkAsyncUploadThread::get_one_semaphore()
{
    VkSemaphore sem{};
    const VkSemaphoreCreateInfo sem_create_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                                 .pNext = nullptr,
                                                 .flags = 0 };
    vkCreateSemaphore(vkstate->renderer->device(), &sem_create_info, nullptr, &sem);

    return nullptr;
}

struct TaskSystem
{
    enum TaskGroups : uint32_t
    {
        Io,
        GpuWork,
        Max
    };

    concurrencpp::runtime ccpp_runtime{};
    VkAsyncUploadThread vulkan_async_worker;
    concurrencpp::result<void> vulkan_loader_task_handle{};

    TaskSystem()
    {
        vulkan_loader_task_handle =
            ccpp_runtime.thread_executor()->submit([this]() { vulkan_async_worker.main_loop(); });
    }

    ~TaskSystem()
    {
        vulkan_async_worker.send_message(VkAsyncThreadMessage{ VkAsyncThreadMessage_Quit{} });
        vulkan_loader_task_handle.wait();
    }

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
               concurrencpp::result<tl::expected<GeometryWithRenderData, VulkanError>> loadbundle,
               xray::rendering::BindlessUniformBufferResourceHandleEntryPair global_ubo)
        : _task_sys{ std::move(task_sys) }
        , _window{ std::move(window) }
        , _vkrenderer{ std::move(vkrenderer) }
        , _ui{ std::move(ui) }
        , _ui_backend{ std::move(ui_backend) }
        , _debug_draw{ std::move(debug_draw) } // , _registered_demos{ register_demos<dvk::TriangleDemo>() }
        , _loadbundle{ std::move(loadbundle) }
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
    concurrencpp::result<tl::expected<xray::rendering::GeometryWithRenderData, xray::rendering::VulkanError>>
        _loadbundle;
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

// concurrencpp::result<tl::expected<GeometryWithRenderData, VulkanError>>
// create_rendering_resources_task(concurrencpp::executor_tag,
//                                 concurrencpp::thread_pool_executor*,
//                                 VulkanRenderer* r,
//                                 xray::rendering::LoadedGeometry geometry)
// {
//     co_return [r, g = std::move(geometry)]() mutable -> tl::expected<GeometryWithRenderData, VulkanError> {
//         const vec2ui32 buffer_sizes = g.compute_vertex_index_count();
//         xray::rendering::ExtractedMaterialsWithImageSourcesBundle material_data = g.extract_materials(0);
//
//         const size_t image_bytes = lz::chain(material_data.image_sources)
//                                        .map([](const ExtractedImageData& img) { return img.pixels.size_bytes(); })
//                                        .sum();
//
//         const size_t material_def_bytes = material_data.materials.size() * sizeof(PBRMaterialDefinition);
//         const size_t vertex_bytes = buffer_sizes.x * sizeof(VertexPBR);
//         const size_t index_bytes = buffer_sizes.y * sizeof(uint32_t);
//
//         const size_t bytes_to_allocate =
//             align<size_t>(vertex_bytes + index_bytes + image_bytes + material_def_bytes,
//                           r->physical().properties.base.properties.limits.nonCoherentAtomSize);
//
//         uintptr_t staging_buffer_offset = r->reserve_staging_buffer_memory(bytes_to_allocate);
//
//         g.extract_data((void*)staging_buffer_offset, (void*)(staging_buffer_offset + vertex_bytes), { 0, 0 }, 0);
//
//         const auto [queue, queue_idx, queue_pool] = r->queue_data(1);
//         const VkCommandBufferAllocateInfo alloc_info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//                                                          .pNext = nullptr,
//                                                          .commandPool = queue_pool,
//                                                          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//                                                          .commandBufferCount = 1 };
//         VkCommandBuffer cmd_buffer{};
//         const VkResult alloc_cmdbuffs_res =
//             WRAP_VULKAN_FUNC(vkAllocateCommandBuffers, r->device(), &alloc_info, &cmd_buffer);
//         XR_VK_CHECK_RESULT(alloc_cmdbuffs_res);
//
//         const VkCommandBufferBeginInfo cmd_buf_begin_info{
//             .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//             .pNext = nullptr,
//             .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
//             .pInheritanceInfo = nullptr,
//         };
//         vkBeginCommandBuffer(cmd_buffer, &cmd_buf_begin_info);
//
//         auto vertex_buffer =
//             VulkanBuffer::create(*r,
//                                  VulkanBufferCreateInfo{
//                                      .name_tag = "vertex buffer",
//                                      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//                                      .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                      .bytes = vertex_bytes,
//                                  });
//
//         XR_VK_PROPAGATE_ERROR(vertex_buffer);
//
//         const VkBufferCopy copy_vtx{ .srcOffset = staging_buffer_offset, .dstOffset = 0, .size = vertex_bytes };
//         vkCmdCopyBuffer(cmd_buffer, r->staging_buffer(), vertex_buffer->buffer_handle(), 1, &copy_vtx);
//         staging_buffer_offset += vertex_bytes;
//
//         auto index_buffer = VulkanBuffer::create(
//             *r,
//             VulkanBufferCreateInfo{ .name_tag = "index buffer",
//                                     .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//                                     .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                     .bytes = index_bytes });
//         XR_VK_PROPAGATE_ERROR(index_buffer);
//
//         const VkBufferCopy copy_idx{ .srcOffset = staging_buffer_offset, .dstOffset = 0, .size = index_bytes };
//         vkCmdCopyBuffer(cmd_buffer, r->staging_buffer(), index_buffer->buffer_handle(), 1, &copy_idx);
//         staging_buffer_offset += index_bytes;
//
//         const vector<PBRMaterialDefinition> mtl_defs =
//             lz::chain(material_data.materials)
//                 .map([](const ExtractedMaterialDefinition& mtdef) { return PBRMaterialDefinition{}; })
//                 .toVector();
//
//         auto material_buffer = VulkanBuffer::create(
//             *r,
//             VulkanBufferCreateInfo{ .name_tag = "material buffer",
//                                     .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//                                     .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                     .bytes = 0 });
//         XR_VK_PROPAGATE_ERROR(material_buffer);
//
//         memcpy((void*)staging_buffer_offset, mtl_defs.data(), material_def_bytes);
//         const VkBufferCopy copy_mtls{ .srcOffset = staging_buffer_offset, .dstOffset = 0, .size = material_def_bytes
//         }; vkCmdCopyBuffer(cmd_buffer, r->staging_buffer(), material_buffer->buffer_handle(), 1, &copy_mtls);
//         staging_buffer_offset += material_def_bytes;
//
//         vector<VulkanImage> images;
//         vector<VkBufferImageCopy> img_copies;
//         for (const ExtractedImageData& img : material_data.image_sources) {
//             auto image = VulkanImage::from_memory(
//                 *r,
//                 VulkanImageCreateInfo{
//                     .tag_name = "some image",
//                     .type = VK_IMAGE_TYPE_2D,
//                     .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
//                     .memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                     .format = VK_FORMAT_R8G8B8A8_UNORM,
//                     .width = img.width,
//                     .height = img.height,
//                     .layers = 1,
//                 });
//
//             memcpy((void*)staging_buffer_offset, img.pixels.data(), img.pixels.size_bytes());
//
//             XR_VK_PROPAGATE_ERROR(image);
//             set_image_layout(cmd_buffer,
//                              image->image(),
//                              VK_IMAGE_LAYOUT_UNDEFINED,
//                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                              VkImageSubresourceRange{
//                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                                  .baseMipLevel = 0,
//                                  .levelCount = 1,
//                                  .baseArrayLayer = 0,
//                                  .layerCount = 1,
//                              });
//
//             const VkBufferImageCopy img_cpy{ .bufferOffset = staging_buffer_offset,
//                                              .bufferRowLength = 0,
//                                              .bufferImageHeight = 0,
//                                              .imageSubresource =
//                                                  VkImageSubresourceLayers{
//                                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                                                      .mipLevel = 0,
//                                                      .baseArrayLayer = 0,
//                                                      .layerCount = 1,
//                                                  },
//                                              .imageOffset = {},
//                                              .imageExtent = { img.width, img.height, 1 }
//
//             };
//
//             staging_buffer_offset += img.pixels.size_bytes();
//             vkCmdCopyBufferToImage(
//                 cmd_buffer, r->staging_buffer(), image->image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_cpy);
//             set_image_layout(cmd_buffer,
//                              image->image(),
//                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//
//                              VkImageSubresourceRange{
//                                  .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                                  .baseMipLevel = 0,
//                                  .levelCount = 1,
//                                  .baseArrayLayer = 0,
//                                  .layerCount = 1,
//                              });
//
//             images.push_back(std::move(*image));
//         }
//
//         vkEndCommandBuffer(cmd_buffer);
//         xrUniqueVkSemaphore sem_wait{ nullptr, VkResourceDeleter_VkSemaphore{ r->device() } };
//         const VkSemaphoreCreateInfo sem_create_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
//                                                      .pNext = nullptr,
//                                                      .flags = 0 };
//         vkCreateSemaphore(r->device(), &sem_create_info, nullptr, base::raw_ptr_ptr(sem_wait));
//
//         const VkSemaphore sms[] = { raw_ptr(sem_wait) };
//         const VkSubmitInfo submit_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
//                                         .pNext = nullptr,
//                                         .waitSemaphoreCount = 0,
//                                         .pWaitSemaphores = nullptr,
//                                         .pWaitDstStageMask = nullptr,
//                                         .commandBufferCount = 1,
//                                         .pCommandBuffers = &cmd_buffer,
//                                         .signalSemaphoreCount = 1,
//                                         .pSignalSemaphores = sms };
//         const VkResult submit_result = WRAP_VULKAN_FUNC(vkQueueSubmit, queue, 1, &submit_info, nullptr);
//         assert(submit_result == VK_SUCCESS);
//
//         const VkSemaphoreWaitInfo wait_info{
//             .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
//             .pNext = nullptr,
//             .flags = 0,
//             .semaphoreCount = 1,
//             .pSemaphores = sms,
//             .pValues = nullptr,
//         };
//         const VkResult wait_res = WRAP_VULKAN_FUNC(vkWaitSemaphores, r->device(), &wait_info, 0xffffffffffffffff);
//         XR_VK_CHECK_RESULT(wait_res);
//
//         return tl::expected<GeometryWithRenderData, VulkanError>{
//             tl::in_place,
//             std::move(g),
//             std::move(*vertex_buffer),
//             std::move(*index_buffer),
//             std::move(*material_buffer),
//             std::move(images),
//         };
//     }();
// }

void
MainRunner::run()
{
    hookup_event_delegates();
    _window.message_loop();
    _vkrenderer->wait_device_idle();
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

    concurrencpp::result<tl::expected<xray::rendering::LoadedGeometry, GeometryImportError>> geom_import_task =
        task_sys->ccpp_runtime.thread_pool_executor()->submit(
            []() { return xray::rendering::LoadedGeometry::from_file(xr_app_config->model_path("sa23/fury.glb")); });

    concurrencpp::result<FontsLoadBundle> font_pkg_load_result =
        MainRunner::load_font_packages(concurrencpp::executor_tag{}, task_sys->thread_exec());

    const window_params_t wnd_params{ "Vulkan Demo", 4, 5, 24, 8, 32, 0, 1, false };

    window main_window{ wnd_params };
    if (!main_window) {
        XR_LOG_ERR("Failed to initialize application window!");
        return tl::nullopt;
    }

    tl::optional<VulkanRenderer> opt_renderer{ VulkanRenderer::create(
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
        ) };

    if (!opt_renderer) {
        XR_LOG_CRITICAL("Failed to create Vulkan renderer!");
        return tl::nullopt;
    }

    auto renderer = xray::base::make_unique<VulkanRenderer>(std::move(*opt_renderer));
    renderer->add_shader_include_directories({ xr_app_config->shader_root() });

    auto loaded_geometry = geom_import_task.get();
    if (!loaded_geometry)
        return tl::nullopt;

    concurrencpp::result<tl::expected<GeometryWithRenderData, VulkanError>> geometry_load_task =
        RendererAsyncTasks::create_rendering_resources_task(concurrencpp::executor_tag{},
                                                            task_sys->thread_pool_exec(),
                                                            base::raw_ptr(renderer),
                                                            std::move(*loaded_geometry));

    const auto [tqueue, tidx, tcmdpool] = renderer->queue_data(1);
    const auto [gqueue, gidx, gcmdpool] = renderer->queue_data(0);
    // task_sys->vulkan_async_worker.send_message(VkAsyncThreadMessage_Setup{ .device = renderer->device(),
    //                                                                        .graphics = tqueue,
    //                                                                        .transfer = tqueue,
    //                                                                        .graphics_idx = gidx,
    //                                                                        .transfer_idx = tidx,
    //                                                                        .transfer_cmdpool = tcmdpool });

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

    return tl::make_optional<MainRunner>(
        PrivateConstructToken{},
        std::move(task_sys),
        std::move(main_window),
        std::move(renderer),
        std::move(ui),
        xray::base::make_unique<UserInterfaceRenderBackend_Vulkan>(std::move(*vk_backend)),
        xray::base::make_unique<DebugDrawSystem>(std::move(*debug_draw)),
        std::move(geometry_load_task),
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
        _demo = dvk::TriangleDemo::create(
            app::init_context_t{
                .surface_width = _window.width(),
                .surface_height = _window.height(),
                .cfg = xr_app_config,
                .ui = raw_ptr(_ui),
                .renderer = raw_ptr(_vkrenderer),
                .quit_receiver = cpp::bind<&MainRunner::demo_quit>(this),
            },
            std::move(_loadbundle));
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
        _demo->loop_event(RenderEvent{ loop_event,
                                       &frd,
                                       raw_ptr(_vkrenderer),
                                       xray::base::raw_ptr(_ui),
                                       g_ubo_mapping->as<FrameGlobalData>(),
                                       raw_ptr(_debug_draw) });
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
// class heightmap_generator
// {
//   public:
//     void generate(const int32_t width, const int32_t height)
//     {
//         this->seed(width, height);
//         smooth_terrain(32);
//         smooth_terrain(128);
//     }
//
//   private:
//     xray::math::vec3f make_point(const float x, const float z)
//     {
//         return { x, _rng.next_float(0.0f, 1.0f) > 0.3f ? std::abs(std::sin(x * z) * _roughness) : 0.0f, z };
//     }
//
//     float get_point(const int32_t x, const int32_t z) const
//     {
//         const auto xp = (x + _width) % _width;
//         const auto zp = (z + _height) % _height;
//
//         return _points[zp * _width + xp].y;
//     }
//
//     void set_point(const int32_t x, const int32_t z, const float value)
//     {
//         const auto xp = (x + _width) % _width;
//         const auto zp = (z + _height) % _height;
//
//         _points[zp * _width + xp].y = value;
//     }
//
//     void point_from_square(const int32_t x, const int32_t z, const int32_t size, const float value)
//     {
//         const auto hs = size / 2;
//         const auto a = get_point(x - hs, z - hs);
//         const auto b = get_point(x + hs, z - hs);
//         const auto c = get_point(x - hs, z + hs);
//         const auto d = get_point(x + hs, z + hs);
//
//         set_point(x, z, (a + b + c + d) / 4.0f + value);
//     }
//
//     void point_from_diamond(const int32_t x, const int32_t z, const int32_t size, const float value)
//     {
//         const auto hs = size / 2;
//         const auto a = get_point(x - hs, z);
//         const auto b = get_point(x + hs, z);
//         const auto c = get_point(x, z - hs);
//         const auto d = get_point(x, z + hs);
//
//         set_point(x, z, (a + b + c + d) / 4.0f + value);
//     }
//
//     void diamond_square(const int32_t step_size, const float scale)
//     {
//         const auto half_step = step_size / 2;
//
//         for (int32_t z = half_step; z < _height + half_step; z += half_step) {
//             for (int32_t x = half_step; x < _width + half_step; x += half_step) {
//                 point_from_square(x, z, step_size, _rng.next_float(0.0f, 1.0f) * scale);
//             }
//         }
//
//         for (int32_t z = 0; z < _height; z += step_size) {
//             for (int32_t x = 0; x < _width; x += step_size) {
//                 point_from_diamond(x + half_step, z, step_size, _rng.next_float(0.0f, 1.0f) * scale);
//                 point_from_diamond(x, z + half_step, step_size, _rng.next_float(0.0f, 1.0f) * scale);
//             }
//         }
//     }
//
//     void seed(const int32_t new_width, const int32_t new_height)
//     {
//         _width = new_width;
//         _height = new_height;
//
//         _points.clear();
//         for (int32_t z = 0; z < _height; ++z) {
//             for (int32_t x = 0; x < _width; ++x) {
//                 _points.push_back(
//                     { static_cast<float>(x), _rng.next_float(0.0f, 1.0f) * _roughness, static_cast<float>(z) });
//             }
//         }
//     }
//
//     void smooth_terrain(const int32_t pass_size)
//     {
//         auto sample_size = pass_size;
//         auto scale_factor = 1.0f;
//
//         while (sample_size > 1) {
//             diamond_square(sample_size, scale_factor);
//             sample_size /= 2;
//             scale_factor /= 2.0f;
//         }
//     }
//
//     float _roughness{ 255.0f };
//     int32_t _width;
//     int32_t _height;
//     xray::base::random_number_generator _rng;
//     std::vector<xray::math::vec3f> _points;
// };
}

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
