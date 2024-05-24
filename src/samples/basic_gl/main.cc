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

#include "xray/xray.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <numeric>
#include <thread>
#include <tuple>
#include <vector>

#include <xcb/xcb.h>

#include <oneapi/tbb/info.h>
#include <oneapi/tbb/task_arena.h>

// #include <opengl/opengl.hpp>

#include <itertools.hpp>
#include <pipes/pipes.hpp>
#include <swl/variant.hpp>

#include "xray/base/app_config.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/debug_output.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/random.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/renderer.vulkan/renderer.vulkan.hpp"
#include "xray/rendering/renderer.vulkan/vulkan.window.platform.data.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/key_sym.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/ui/window.hpp"

#include "demo_base.hpp"
#include "init_context.hpp"
#include "lighting/directional_light_demo.hpp"
#include "misc/fractals/fractal_demo.hpp"
#include "misc/instanced_drawing/instanced_drawing_demo.hpp"
#include "misc/mesh/mesh_demo.hpp"
#include "misc/procedural_city/procedural_city_demo.hpp"
#include "misc/texture_array/texture_array_demo.hpp"

// #include "misc/geometric_shapes/geometric_shapes_demo.hpp"

using namespace xray;
using namespace xray::base;
using namespace xray::math;
using namespace xray::ui;
using namespace xray::rendering;
using namespace std;

xray::base::ConfigSystem* xr_app_config{ nullptr };
//
// namespace app {
//
// struct DemoInfo
// {
//     std::string_view shortDesc;
//     std::string_view detailedDesc;
//     fast_delegate<tl::optional<demo_bundle_t>(const init_context_t&)> createFn;
// };
//
// template<typename... Ts>
// struct RegisteredDemosList;
//
// template<typename T, typename... Rest>
// struct RegisteredDemosList<T, Rest...>
// {
//     static void registerDemo(vector<DemoInfo>& demoList)
//     {
//         demoList.emplace_back(T::short_desc(), T::detailed_desc(), make_delegate(T::create));
//         RegisteredDemosList<Rest...>::registerDemo(demoList);
//     }
// };
//
// template<>
// struct RegisteredDemosList<>
// {
//     static void registerDemo(vector<DemoInfo>&) {}
// };
//
// enum class demo_type : int32_t
// {
//     none,
//     colored_circle,
//     fractal,
//     texture_array,
//     mesh,
//     bufferless_draw,
//     lighting_directional,
//     lighting_point,
//     procedural_city,
//     instanced_drawing,
//     geometric_shapes,
//     // terrain_basic
// };
//
// class main_app
// {
//   public:
//     explicit main_app(xray::ui::window* wnd);
//     ~main_app() {}
//
//     bool valid() const noexcept { return _initialized; }
//
//     explicit operator bool() const noexcept { return valid(); }
//
//   private:
//     bool demo_running() const noexcept { return _demo != nullptr; }
//     void run_demo(const demo_type type);
//     void hookup_event_delegates();
//     void demo_quit();
//
//     const DemoInfo& get_demo_info(const size_t idx) const
//     {
//         assert(idx < _registeredDemos.size());
//         return _registeredDemos[idx];
//     }
//
//     /// \group Event handlers
//     /// @{
//
//     void loop_event(const xray::ui::window_loop_event& loop_evt);
//
//     void event_handler(const xray::ui::window_event& wnd_evt);
//
//     void poll_start(const xray::ui::poll_start_event&);
//
//     void poll_end(const xray::ui::poll_end_event&);
//
//     /// @}
//
//     void draw(const xray::ui::window_loop_event& loop_evt);
//
//   private:
//     xray::ui::window* _window;
//     xray::base::unique_pointer<xray::ui::user_interface> _ui{};
//     xray::base::unique_pointer<demo_base> _demo;
//     xray::rendering::rgb_color _clear_color{ xray::rendering::color_palette::material::bluegrey800 };
//     xray::base::timer_highp _timer;
//     vector<DemoInfo> _registeredDemos;
//     bool _initialized{ false };
//     vector<char> _combo_items{};
//
//     XRAY_NO_COPY(main_app);
// };
//
// main_app::main_app(xray::ui::window* wnd)
//     : _window{ wnd }
// {
//     namespace fs = std::filesystem;
//     const vector<xray::ui::font_info> font_list = fs::recursive_directory_iterator(xr_app_config->font_root()) >>=
//         pipes::filter([](const fs::directory_entry& dir_entry) {
//             if (!dir_entry.is_regular_file())
//                 return false;
//
//             const std::string_view file_ext{ dir_entry.path().extension().c_str() };
//             return file_ext == ".ttf" || file_ext == ".otf";
//         }) >>= pipes::transform([](const fs::directory_entry& dir_entry) {
//             return xray::ui::font_info{ .path = dir_entry.path(), .pixel_size = 18.0f };
//         }) >>= pipes::to_<vector<xray::ui::font_info>>();
//
//     _ui = xray::base::make_unique<xray::ui::user_interface>(font_list.data(), font_list.size());
//
//     hookup_event_delegates();
//
//     RegisteredDemosList<FractalDemo, DirectionalLightDemo, procedural_city_demo, InstancedDrawingDemo>::registerDemo(
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
// }
//
// void
// main_app::poll_start(const xray::ui::poll_start_event&)
// {
// }
//
// void
// main_app::poll_end(const xray::ui::poll_end_event&)
// {
// }
//
// void
// main_app::demo_quit()
// {
//     assert(demo_running());
//     _demo = nullptr;
//     hookup_event_delegates();
// }
//
// void
// main_app::hookup_event_delegates()
// {
//     _window->events.loop = make_delegate(*this, &main_app::loop_event);
//     _window->events.poll_start = make_delegate(*this, &main_app::poll_start);
//     _window->events.poll_end = make_delegate(*this, &main_app::poll_end);
//     _window->events.window = make_delegate(*this, &main_app::event_handler);
// }
//
// void
// main_app::event_handler(const xray::ui::window_event& wnd_evt)
// {
//     assert(!demo_running());
//
//     if (is_input_event(wnd_evt)) {
//
//         if (wnd_evt.event.key.keycode == xray::ui::KeySymbol::escape &&
//             wnd_evt.event.key.type == event_action_type::press && !_ui->wants_input()) {
//             _window->quit();
//             return;
//         }
//
//         _ui->input_event(wnd_evt);
//     }
// }
//
// void
// main_app::loop_event(const xray::ui::window_loop_event& levt)
// {
//     _timer.update_and_reset();
//     _ui->tick(_timer.elapsed_millis());
//     _ui->new_frame(levt.wnd_width, levt.wnd_height);
//
//     {
//         ImGui::SetNextWindowPos({ 0.0f, 0.0f }, ImGuiCond_Appearing);
//
//         if (ImGui::Begin("Run a demo", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
//
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
//         }
//
//         ImGui::End();
//     }
//
//     const xray::math::vec4f viewport{
//         0.0f, 0.0f, static_cast<float>(levt.wnd_width), static_cast<float>(levt.wnd_height)
//     };
//
//     gl::ViewportIndexedfv(0, viewport.components);
//     gl::ClearNamedFramebufferfv(0, gl::COLOR, 0, _clear_color.components);
//     gl::ClearNamedFramebufferfi(0, gl::DEPTH_STENCIL, 0, 1.0f, 0xffffffff);
//     _ui->draw();
// }
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
//
int
main(int argc, char** argv)
{
    using namespace xray::ui;
    using namespace xray::base;

    xray::base::setup_logging();

    XR_LOG_INFO("Starting up ...");

    const int num_threads = oneapi::tbb::info::default_concurrency();
    XR_LOG_INFO("Default concurency {}", num_threads);

    ConfigSystem app_cfg{ "config/app_config.conf" };
    xr_app_config = &app_cfg;

    XR_LOG_INFO("Configured paths");
    XR_LOG_INFO("Root {}", xr_app_config->root_directory().c_str());
    XR_LOG_INFO("Shaders {}", xr_app_config->shader_config_path("").c_str());
    XR_LOG_INFO("Models {}", xr_app_config->model_path("").c_str());
    XR_LOG_INFO("Textures {}", xr_app_config->texture_path("").c_str());
    XR_LOG_INFO("Fonts {}", xr_app_config->font_path("").c_str());

    const window_params_t wnd_params{ "OpenGL Demo", 4, 5, 24, 8, 32, 0, 1, false };

    window main_window{ wnd_params };
    if (!main_window) {
        XR_LOG_ERR("Failed to initialize application window!");
        return EXIT_FAILURE;
    }

    VulkanRenderer::create(WindowPlatformDataXlib{
                               .display = main_window.native_display(),
                               .window = main_window.native_window(),
                               .visual = main_window.native_visual(),
                           })
        .map_or_else([](VulkanRenderer vkr) {}, []() { XR_LOG_CRITICAL("Failed to create Vulkan renderer!"); });

    // gl::DebugMessageCallback(app::debug_proc, nullptr);
    // //
    // // turn off these so we don't get spammed
    // gl::DebugMessageControl(gl::DONT_CARE, gl::DONT_CARE, gl::DEBUG_SEVERITY_NOTIFICATION, 0, nullptr,
    // gl::FALSE_);
    //
    // app::main_app app{ &main_window };
    // main_window.message_loop();
    //
    // XR_LOG_INFO("Shutting down ...");
    return EXIT_SUCCESS;
}

// static void
// cursor_set(xcb_connection_t* c, xcb_screen_t* screen, xcb_window_t window, int cursor_id)
// {
//     uint32_t values_list[3];
//     xcb_void_cookie_t cookie_font;
//     xcb_void_cookie_t cookie_gc;
//     xcb_generic_error_t* error;
//     xcb_font_t font;
//     xcb_cursor_t cursor;
//     xcb_gcontext_t gc;
//     uint32_t mask;
//     uint32_t value_list;
//
//     font = xcb_generate_id(c);
//     cookie_font = xcb_open_font_checked(c, font, strlen("cursor"), "cursor");
//     error = xcb_request_check(c, cookie_font);
//     if (error) {
//         fprintf(stderr, "ERROR: can't open font : %d\n", error->error_code);
//         xcb_disconnect(c);
//         exit(-1);
//     }
//
//     cursor = xcb_generate_id(c);
//     xcb_create_glyph_cursor(c, cursor, font, font, cursor_id, cursor_id + 1, 0, 0, 0, 0, 0, 0);
//
//     gc = xcb_generate_id(c);
//     mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;
//     values_list[0] = screen->black_pixel;
//     values_list[1] = screen->white_pixel;
//     values_list[2] = font;
//     cookie_gc = xcb_create_gc_checked(c, gc, window, mask, values_list);
//     error = xcb_request_check(c, cookie_gc);
//     if (error) {
//         fprintf(stderr, "ERROR: can't create gc : %d\n", error->error_code);
//         xcb_disconnect(c);
//         exit(-1);
//     }
//
//     mask = XCB_CW_CURSOR;
//     value_list = cursor;
//     xcb_change_window_attributes(c, window, mask, &value_list);
//
//     xcb_free_cursor(c, cursor);
//
//     cookie_font = xcb_close_font_checked(c, font);
//     error = xcb_request_check(c, cookie_font);
//     if (error) {
//         fprintf(stderr, "ERROR: can't close font : %d\n", error->error_code);
//         xcb_disconnect(c);
//         exit(-1);
//     }
// }
//
// int
// main()
// {
//
//     using namespace xray::ui;
//     using namespace xray::base;
//
//     xray::base::setup_logging();
//
//     XR_LOG_INFO("Starting up ...");
//
//     const int num_threads = oneapi::tbb::info::default_concurrency();
//     XR_LOG_INFO("Default concurency {}", num_threads);
//
//     ConfigSystem app_cfg{ "config/app_config.conf" };
//     xr_app_config = &app_cfg;
//
//     XR_LOG_INFO("Configured paths");
//     XR_LOG_INFO("Root {}", xr_app_config->root_directory().c_str());
//     XR_LOG_INFO("Shaders {}", xr_app_config->shader_config_path("").c_str());
//     XR_LOG_INFO("Models {}", xr_app_config->model_path("").c_str());
//     XR_LOG_INFO("Textures {}", xr_app_config->texture_path("").c_str());
//     XR_LOG_INFO("Fonts {}", xr_app_config->font_path("").c_str());
//
//     const window_params_t wnd_params{ "OpenGL Demo", 4, 5, 24, 8, 32, 0, 1, false };
//
//     xcb_screen_iterator_t screen_iter;
//     xcb_connection_t* c;
//     const xcb_setup_t* setup;
//     xcb_screen_t* screen;
//     xcb_generic_event_t* e;
//     xcb_generic_error_t* error;
//     xcb_void_cookie_t cookie_window;
//     xcb_void_cookie_t cookie_map;
//     xcb_window_t window;
//     uint32_t mask;
//     uint32_t values[2];
//     int screen_number;
//     uint8_t is_hand = 0;
//     constexpr const int WIDTH = 1024;
//     constexpr const int HEIGHT = 1024;
//
//     /* getting the connection */
//     c = xcb_connect(NULL, &screen_number);
//     if (!c) {
//         fprintf(stderr, "ERROR: can't connect to an X server\n");
//         return -1;
//     }
//
//     /* getting the current screen */
//     setup = xcb_get_setup(c);
//
//     screen = NULL;
//     screen_iter = xcb_setup_roots_iterator(setup);
//     for (; screen_iter.rem != 0; --screen_number, xcb_screen_next(&screen_iter))
//         if (screen_number == 0) {
//             screen = screen_iter.data;
//             break;
//         }
//     if (!screen) {
//         fprintf(stderr, "ERROR: can't get the current screen\n");
//         xcb_disconnect(c);
//         return -1;
//     }
//
//     /* creating the window */
//     window = xcb_generate_id(c);
//     mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
//     values[0] = screen->white_pixel;
//     values[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_EXPOSURE |
//                 XCB_EVENT_MASK_POINTER_MOTION;
//     cookie_window = xcb_create_window_checked(c,
//                                               screen->root_depth,
//                                               window,
//                                               screen->root,
//                                               20,
//                                               200,
//                                               WIDTH,
//                                               HEIGHT,
//                                               0,
//                                               XCB_WINDOW_CLASS_INPUT_OUTPUT,
//                                               screen->root_visual,
//                                               mask,
//                                               values);
//     cookie_map = xcb_map_window_checked(c, window);
//
//     /* error managing */
//     error = xcb_request_check(c, cookie_window);
//     if (error) {
//         fprintf(stderr, "ERROR: can't create window : %d\n", error->error_code);
//         xcb_disconnect(c);
//         return -1;
//     }
//     error = xcb_request_check(c, cookie_map);
//     if (error) {
//         fprintf(stderr, "ERROR: can't map window : %d\n", error->error_code);
//         xcb_disconnect(c);
//         return -1;
//     }
//
//     cursor_set(c, screen, window, 68);
//     xcb_flush(c);
//
//     const xray::rendering::WindowPlatformData window_platform_data =
//         WindowPlatformDataXcb{ .connection = c, .window = window, .visual = screen->root_visual };
//
//     VulkanRenderer::create(window_platform_data)
//         .map_or_else([](VulkanRenderer vkr) { XR_LOG_INFO("Ebaaaaaaaat ! VulkanRenderer created successfully!"); },
//                      []() { XR_LOG_CRITICAL("Failed to create Vulkan renderer!"); });
//
//     return 0;
// }
