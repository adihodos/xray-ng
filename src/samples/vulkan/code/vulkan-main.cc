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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <thread>
#include <tuple>
#include <span>
#include <chrono>
#include <vector>

#include <oneapi/tbb/info.h>
#include <oneapi/tbb/task_arena.h>

#include <Lz/Lz.hpp>
#include <swl/variant.hpp>

#include "xray/base/app_config.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/delegate.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/random.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/user.interface.backend.hpp"
#include "xray/ui/user.interface.backend.vulkan.hpp"
#include "xray/ui/user_interface_render_context.hpp"
#include "xray/ui/window.hpp"

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

class MainRunner
{
  private:
    struct PrivateConstructToken
    {
        explicit PrivateConstructToken() = default;
    };

  public:
    MainRunner(PrivateConstructToken,
               xray::ui::window window,
               xray::rendering::VulkanRenderer vkrenderer,
               xray::base::unique_pointer<xray::ui::user_interface> ui,
               xray::base::unique_pointer<xray::ui::UserInterfaceRenderBackend_Vulkan> ui_backend,
               xray::rendering::BindlessUniformBufferResourceHandleEntryPair global_ubo)
        : _window{ std::move(window) }
        , _vkrenderer{ std::move(vkrenderer) }
        , _ui{ std::move(ui) }
        , _ui_backend{ std::move(ui_backend) }
        , _registered_demos{ register_demos<dvk::TriangleDemo>() }
        , _global_ubo{ global_ubo }
    {
        hookup_event_delegates();
        _timer.start();

        lz::chain(_registered_demos).forEach([](const RegisteredDemo& rd) {
            XR_LOG_INFO("Registered demo: {} - {}", rd.short_desc, rd.detailed_desc);
        });
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

  private:
    xray::ui::window _window;
    xray::rendering::VulkanRenderer _vkrenderer;
    xray::base::unique_pointer<xray::ui::user_interface> _ui{};
    xray::base::unique_pointer<xray::ui::UserInterfaceRenderBackend_Vulkan> _ui_backend{};
    xray::base::unique_pointer<DemoBase> _demo{};
    xray::rendering::rgb_color _clear_color{ xray::rendering::color_palette::material::deeppurple900 };
    xray::base::timer_highp _timer{};
    vector<RegisteredDemo> _registered_demos;
    vector<char> _combo_items{};
    xray::rendering::BindlessUniformBufferResourceHandleEntryPair _global_ubo;

    XRAY_NO_COPY(MainRunner);
};

MainRunner::~MainRunner() {}

void
MainRunner::run()
{
    hookup_event_delegates();
    _window.message_loop();
    _vkrenderer.wait_device_idle();
}

tl::optional<MainRunner>
MainRunner::create()
{
    using namespace xray::ui;
    using namespace xray::base;

    xray::base::setup_logging();

    XR_LOG_INFO("Xray source commit: {}, built on {}, user {}, machine {}",
                xray::build::config::commit_hash_str,
                xray::build::config::build_date_time,
                xray::build::config::user_info,
                xray::build::config::machine_info);

    const int num_threads = oneapi::tbb::info::default_concurrency();
    XR_LOG_INFO(
        "Default concurency {}\nWorking directory {}", num_threads, std::filesystem::current_path().generic_string());

    static ConfigSystem app_cfg{ "config/app_config.conf" };
    xr_app_config = &app_cfg;

    const window_params_t wnd_params{ "Vulkan Demo", 4, 5, 24, 8, 32, 0, 1, false };

    window main_window{ wnd_params };
    if (!main_window) {
        XR_LOG_ERR("Failed to initialize application window!");
        return tl::nullopt;
    }

    tl::optional<VulkanRenderer> renderer{ VulkanRenderer::create(
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

    if (!renderer) {
        XR_LOG_CRITICAL("Failed to create Vulkan renderer!");
        return tl::nullopt;
    }

    renderer->add_shader_include_directories({ xr_app_config->shader_root() });

    namespace fs = std::filesystem;
    vector<xray::ui::font_info> font_list;
    for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(xr_app_config->font_root())) {
        if (!dir_entry.is_regular_file() || !dir_entry.path().has_extension())
            continue;

        const fs::path file_ext{ dir_entry.path().extension() };
        if (file_ext != ".ttf" && file_ext != ".otf")
            continue;

        font_list.emplace_back(dir_entry.path(), 18.0f);
    };

    xray::base::unique_pointer<user_interface> ui{ xray::base::make_unique<xray::ui::user_interface>(
        span{ font_list }) };

    tl::expected<UserInterfaceRenderBackend_Vulkan, VulkanError> vk_backend{ UserInterfaceRenderBackend_Vulkan::create(
        *renderer, ui->render_backend_create_info()) };

    if (!vk_backend) {
        XR_LOG_CRITICAL("Failed to create Vulkan render backed for UI!");
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
    //     gl::ClipControl(gl::LOWER_LEFT, gl::ZERO_TO_ONE);
    //     _ui->set_global_font("Roboto-Regular");
    //     _timer.start();

    const BindlessUniformBufferResourceHandleEntryPair g_ubo_handles =
        renderer->bindless_sys().add_chunked_uniform_buffer(std::move(*g_ubo), renderer->buffering_setup().buffers);

    return tl::make_optional<MainRunner>(
        PrivateConstructToken{},
        std::move(main_window),
        std::move(*renderer.take()),
        std::move(ui),
        xray::base::make_unique<UserInterfaceRenderBackend_Vulkan>(std::move(*vk_backend)),
        g_ubo_handles);
}

void
MainRunner::demo_quit()
{
    assert(demo_running());
    _vkrenderer.wait_device_idle();
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
    if (demo_running()) {
        _demo->event_handler(wnd_evt);
        return;
    }

    if (is_input_event(wnd_evt)) {

        if (wnd_evt.event.key.keycode == xray::ui::KeySymbol::escape &&
            wnd_evt.event.key.type == event_action_type::press && !_ui->wants_input()) {
            _window.quit();
            return;
        }

        _ui->input_event(wnd_evt);
    }
}

void
MainRunner::loop_event(const xray::ui::window_loop_event& loop_event)
{
    static bool doing_ur_mom{ false };
    if (!doing_ur_mom && !_demo) {
        _demo = dvk::TriangleDemo::create(app::init_context_t{
            .surface_width = _window.width(),
            .surface_height = _window.height(),
            .cfg = xr_app_config,
            .ui = raw_ptr(_ui),
            .renderer = &_vkrenderer,
            .quit_receiver = cpp::bind<&MainRunner::demo_quit>(this),
        });
        doing_ur_mom = true;
    }

    _timer.update_and_reset();
    _ui->tick(_timer.elapsed_millis());
    _ui->new_frame(loop_event.wnd_width, loop_event.wnd_height);

    const FrameRenderData frd{ _vkrenderer.begin_rendering() };

    //
    // flush and bind the global descriptor table
    _vkrenderer.bindless_sys().flush_descriptors(_vkrenderer);
    _vkrenderer.bindless_sys().bind_descriptors(_vkrenderer, frd.cmd_buf);

    auto g_ubo_mapping = UniqueMemoryMapping::map_memory(_vkrenderer.device(),
                                                         _global_ubo.second.memory,
                                                         frd.id * _global_ubo.second.aligned_chunk_size,
                                                         _global_ubo.second.aligned_chunk_size);

    if (_demo) {
        _demo->loop_event(RenderEvent{
            loop_event, &frd, &_vkrenderer, xray::base::raw_ptr(_ui), g_ubo_mapping->as<FrameGlobalData>() });
    } else {
        //
        // do main page UI
        ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Appearing);
        if (ImGui::Begin("Run a demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {

            //             int32_t selectedItem{};
            //             const bool wasClicked = ImGui::Combo("Available demos", &selectedItem, _combo_items.data());
            //
            //             if (wasClicked && selectedItem >= 1) {
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
            //             }
        }
        ImGui::End();

        const VkViewport viewport{
            .x = 0.0f,
            .y = static_cast<float>(loop_event.wnd_height),
            .width = static_cast<float>(loop_event.wnd_width),
            .height = -static_cast<float>(loop_event.wnd_height),
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
        _vkrenderer.clear_attachments(frd.cmd_buf, 1.0f, 0.0f, 1.0f);
    }

    //
    // move the UBO mapping into the lambda so that the data is flushed before the rendering starts
    _ui->draw().map([this, frd, g_ubo = std::move(*g_ubo_mapping)](UserInterfaceRenderContext ui_render_ctx) mutable {
        FrameGlobalData* fg = g_ubo.as<FrameGlobalData>();
        fg->frame = frd.id;
        fg->ui = UIData{
            .scale = vec2f{ ui_render_ctx.scale_x, ui_render_ctx.scale_y },
            .translate = vec2f{ ui_render_ctx.translate_x, ui_render_ctx.translate_y },
            .textureid = ui_render_ctx.textureid,
        };
        _ui_backend->render(ui_render_ctx, _vkrenderer, frd);
    });

    _vkrenderer.end_rendering();
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
