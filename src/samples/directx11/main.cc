#include "cap1/test_object.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/nothing.hpp"
#include "xray/base/perf_stats_collector.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/math/projection.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/directx/dx11_renderer.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/scene/camera.hpp"
#include "xray/scene/camera_controller_spherical_coords.hpp"
#include "xray/ui/app_window.hpp"
#include "xray/ui/input_event.hpp"
#include "xray/ui/key_symbols.hpp"
#include "xray/ui/user_interface.hpp"
#include "xray/xray.hpp"
#include <imgui/imgui.h>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::ui;
using namespace xray::math;
using namespace xray::scene;
using namespace std;

class basic_scene
{
  public:
    explicit basic_scene(application_window* app_wnd, dx11_renderer* renderer) noexcept;

    ~basic_scene();

    void tick_event(const float delta_ms) noexcept;

    void draw_event(const window_draw_event& wde) noexcept;

    void resize_event(const uint32_t width, const uint32_t height) noexcept;

    void input_event(const input_event_t&) noexcept;

  private:
    static constexpr const char* controller_cfg_file_path = "config/cam_controller_spherical.conf";

    void ui_logic();

    void collect_stats(const float delta_ms);

    application_window* _appwnd{};
    dx11_renderer* _renderer{};
    camera _cam;
    draw_context_t _draw_context;
    app::simple_object _object;
    camera_controller_spherical_coords _cam_control;
    imgui_backend _ui;
    bool _ui_active{ false };
    rgb_color _clear_color{ color_palette::material::black };
    float _frame_time[60];
    uint32_t _frame_idx{};

    struct statistics
    {
        static constexpr uint32_t cpu_max_count = 4u;
        float s_frame_time{};
        uint32_t cpu_count{};
        float cpu_usage[cpu_max_count];
    } _stats;

    stats_thread _perf_stats_thread{};
    stats_thread::process_stats_info _proc_stats;

  private:
    XRAY_NO_COPY(basic_scene);
};

basic_scene::basic_scene(application_window* app_wnd, dx11_renderer* renderer) noexcept
    : _appwnd{ app_wnd }
    , _renderer{ renderer }
    , _object{ renderer->device() }
    , _cam_control{ &_cam, controller_cfg_file_path }
    , _ui{ renderer->device(), renderer->context() }
{

    memset(_frame_time, 0, sizeof(_frame_time));
    const auto r_win_height = static_cast<float>(_renderer->render_target_height());
    const auto r_win_width = static_cast<float>(_renderer->render_target_width());

    _cam.look_at({ 0.0f, 10.0f, -5.0f }, float3::stdc::zero, float3::stdc::unit_y);

    _cam.set_projection(projection::perspective_symmetric(r_win_width, r_win_height, radians(60.0f), 0.5f, 1000.0f));

    _draw_context.active_camera = &_cam;
    _draw_context.projection_matrix = _cam.projection();
    _draw_context.proj_view_matrix = _cam.projection_view();
    _draw_context.view_matrix = _cam.view();
    _draw_context.window_height = _renderer->render_target_width();
    _draw_context.window_height = _renderer->render_target_height();
    _draw_context.renderer = _renderer;

    _ui.events.setup_ui = make_delegate(*this, &basic_scene::ui_logic);
}

basic_scene::~basic_scene()
{
    _perf_stats_thread.signal_stop();
}

void
basic_scene::ui_logic()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Render target clear color");
    ImGui::SliderFloat("Red", &_clear_color.r, 0.0f, 1.0f);
    ImGui::SliderFloat("Green", &_clear_color.g, 0.0f, 1.0f);
    ImGui::SliderFloat("Blue", &_clear_color.b, 0.0f, 1.0f);
    ImGui::End();

    {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 120.0f));
        ImGui::SetNextWindowSize(ImVec2(400.0f, 200.0f), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Camera :");

        {
            ImGui::Text("View frame vectors :");
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
                               "R: [%3.3f %3.3f %3.3f]",
                               _cam.right().x,
                               _cam.right().y,
                               _cam.right().z);
            ImGui::TextColored(
                ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "U: [%3.3f %3.3f %3.3f]", _cam.up().x, _cam.up().y, _cam.up().z);
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
                               "D: [%3.3f %3.3f %3.3f]",
                               _cam.direction().x,
                               _cam.direction().y,
                               _cam.direction().z);
            ImGui::Text("O: [%3.3f %3.3f %3.3f]", _cam.origin().x, _cam.origin().y, _cam.origin().z);
        }

        ImGui::Separator();

        {
            const auto vm = _cam.view();
            ImGui::Text("View transform:\n"
                        "\t%.3f %.3f %.3f %.3f\n"
                        "\t%.3f %.3f %.3f %.3f\n"
                        "\t%.3f %.3f %.3f %.3f\n"
                        "\t%.3f %.3f %.3f %.3f ",
                        vm.a00,
                        vm.a01,
                        vm.a02,
                        vm.a03,
                        vm.a10,
                        vm.a11,
                        vm.a12,
                        vm.a13,
                        vm.a20,
                        vm.a21,
                        vm.a22,
                        vm.a23,
                        vm.a30,
                        vm.a31,
                        vm.a32,
                        vm.a33);
        }

        ImGui::Separator();

        {
            const auto vm = _cam.projection();
            ImGui::Text("Projection transform:\n"
                        "\t%.3f %.3f %.3f %.3f\n"
                        "\t%.3f %.3f %.3f %.3f\n"
                        "\t%.3f %.3f %.3f %.3f\n"
                        "\t%.3f %.3f %.3f %.3f ",
                        vm.a00,
                        vm.a01,
                        vm.a02,
                        vm.a03,
                        vm.a10,
                        vm.a11,
                        vm.a12,
                        vm.a13,
                        vm.a20,
                        vm.a21,
                        vm.a22,
                        vm.a23,
                        vm.a30,
                        vm.a31,
                        vm.a32,
                        vm.a33);
        }

        ImGui::End();
    }

    {
        ImGui::SetNextWindowPos(ImVec2(0.0f, 500.0f));
        ImGui::SetNextWindowSize(ImVec2(100.0f, 100.0f), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Performance stats :");
        ImGui::Text("CPU usage: %3.3f\n", _proc_stats.cpu_usage);
        ImGui::Text("Threads: %u", _proc_stats.thread_count);
        ImGui::Text("Peak virtual MBytes: %u", _proc_stats.vbytes_peak / 1024);
        ImGui::Text("Virtual MBytes: %u", _proc_stats.virtual_bytes / 1024);
        ImGui::Text("Working set MBytes: %u", _proc_stats.working_set / 1024);
        ImGui::Text("Working set peak MBytes: %u", _proc_stats.work_set_peak / 1024);
        ImGui::Separator();
        ImGui::PlotLines("CPU Usage", _frame_time, 60, 0, nullptr, 0.0f, 100.0f, ImVec2(0, 120));
        ImGui::End();
    }
}

void
basic_scene::collect_stats(const float delta_ms)
{
    _stats.s_frame_time = delta_ms;
}

void
basic_scene::tick_event(const float delta_ms) noexcept
{
    _cam_control.update();

    _draw_context.projection_matrix = _cam.projection();
    _draw_context.proj_view_matrix = _cam.projection_view();
    _draw_context.view_matrix = _cam.view();
    _draw_context.window_height = _renderer->render_target_width();
    _draw_context.window_height = _renderer->render_target_height();

    collect_stats(delta_ms);

    _ui.tick(delta_ms);

    _proc_stats = _perf_stats_thread.process_stats();

    if (_frame_idx == 60)
        _frame_idx = 0;
    _frame_time[_frame_idx++] = _proc_stats.cpu_usage;
}

void
basic_scene::draw_event(const window_draw_event& /*wde*/) noexcept
{
    // constexpr auto clear_color = color_palette::material::blue100;
    _renderer->clear_render_target_view(_clear_color.r, _clear_color.g, _clear_color.b);
    _renderer->clear_depth_stencil();

    if (!_object)
        return;

    _object.draw(_draw_context);

    if (_ui_active) {
        _ui.draw_event(_draw_context);
    }
}

void
basic_scene::resize_event(const uint32_t width, const uint32_t height) noexcept
{
    _renderer->handle_resize(width, height);

    const auto r_win_width = static_cast<float>(width);
    const auto r_win_height = static_cast<float>(height);

    _cam.set_projection(projection::perspective_symmetric(r_win_width, r_win_height, radians(60.0f), 0.5f, 1000.0f));

    _draw_context.window_height = height;
    _draw_context.window_width = width;
}

void
basic_scene::input_event(const input_event_t& in_event) noexcept
{
    if (in_event.type == input_event_type::key) {
        const auto key_evt = in_event.key_ev;

        if (key_evt.down) {
            const auto key_code = key_evt.key_code;

            switch (key_code) {
                case key_symbol::key_o: {
                    _ui_active = !_ui_active;
                    return;
                } break;

                case key_symbol::escape: {
                    _appwnd->quit();
                    return;
                } break;

                default:
                    break;
            }
        }
    }

    if (_ui_active) {
        _ui.input_event(in_event);
        if (_ui.wants_input())
            return;
    }
    _cam_control.input_event(in_event);
}

maybe<scalar2u32>
get_primary_monitor_resolution()
{
    auto primary_monitor = MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    if (!primary_monitor)
        return nothing{};

    MONITORINFO mon_info;
    mon_info.cbSize = sizeof(mon_info);

    if (!GetMonitorInfo(primary_monitor, &mon_info))
        return nothing{};

    const auto res_x = static_cast<uint32_t>(std::abs(mon_info.rcWork.right - mon_info.rcMonitor.left));
    const auto res_y = static_cast<uint32_t>(std::abs(mon_info.rcWork.bottom - mon_info.rcWork.top));

    XR_LOG_INFO("Primary monitor resolution [{0}x{1}]", res_x, res_y);
    return scalar2u32{ res_x, res_y };
}

class test_scene
{
  public:
    explicit test_scene(application_window* appwnd, dx11_renderer* renderer) noexcept
        : _appwnd{ appwnd }
        , _renderer{ renderer }
    {
    }

    void input_event(const xray::ui::input_event_t& input_evt);
    void draw_event(const xray::ui::window_draw_event& wde);
    void resize_event(const uint32_t w, const uint32_t h);
    void tick_event(const float);
    HRESULT swap_buffers_event(const bool);

    void toggle_fullscreen_event();

  private:
    HBRUSH _brush{};
    application_window* _appwnd{};
    dx11_renderer* _renderer;
};

void
test_scene::input_event(const xray::ui::input_event_t& input_evt)
{
    if (input_evt.type == input_event_type::key) {
        if (input_evt.key_ev.key_code == key_symbol::escape)
            _appwnd->quit();
    }
}

void
test_scene::draw_event(const xray::ui::window_draw_event& wde)
{
    // if (!_brush) {
    //  LOGBRUSH lb;
    //  lb.lbStyle = BS_SOLID;
    //  lb.lbColor = RGB(255, 64, 128);
    //  _brush     = CreateBrushIndirect(&lb);
    //}

    // RECT client_area;
    // GetClientRect(wde.window, &client_area);
    // auto hdc = GetDC(wde.window);
    // FillRect(hdc, &client_area, _brush);
    // ReleaseDC(wde.window, hdc);
    _renderer->clear_depth_stencil();
    _renderer->clear_render_target_view(1.0f, 0.25f, 0.5f, 1.0f);
}

void
test_scene::resize_event(const uint32_t w, const uint32_t h)
{
    _renderer->handle_resize(w, h);
}

void
test_scene::tick_event(const float)
{
}

HRESULT
test_scene::swap_buffers_event(const bool)
{
    return S_OK;
}

void
test_scene::toggle_fullscreen_event()
{
}

int WINAPI
WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    XR_LOGGER_INIT();
    XR_LOG_INFO("Starting up ...");

    xray::base::app_config configurator_instance{ nullptr };

    const auto primary_mon_res = get_primary_monitor_resolution();

    const window_params wnd_setup{ primary_mon_res ? primary_mon_res.value().x : 1024,
                                   primary_mon_res ? primary_mon_res.value().y : 768,
                                   false,
                                   "Direct3D App" };

    application_window app_wnd{ wnd_setup };
    if (!app_wnd) {
        XR_LOG_CRITICAL("Failed to create output window !");
        return EXIT_FAILURE;
    }

    const renderer_init_params renderer_setup{ DXGI_FORMAT_R8G8B8A8_UNORM,
                                               DXGI_FORMAT_D24_UNORM_S8_UINT,
                                               D3D_DRIVER_TYPE_HARDWARE,
                                               D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_SINGLETHREADED,
                                               app_wnd.handle() };

    dx11_renderer renderer{ renderer_setup };
    if (!renderer) {
        XR_LOG_CRITICAL("Failed to create renderer, exiting...");
        return EXIT_FAILURE;
    }

    basic_scene s{ &app_wnd, &renderer };

    {
        app_wnd.events.input = make_delegate(s, &basic_scene::input_event);
        app_wnd.events.draw = make_delegate(s, &basic_scene::draw_event);
        app_wnd.events.tick = make_delegate(s, &basic_scene::tick_event);
        app_wnd.events.resize = make_delegate(s, &basic_scene::resize_event);

        app_wnd.events.swap_buffers = make_delegate(renderer, &dx11_renderer::swap_buffers);
        app_wnd.events.toggle_fullscreen = make_delegate(renderer, &dx11_renderer::toggle_fullscreen);
    }

    // test_scene ts{&app_wnd, &renderer};
    // app_wnd.events.input  = make_delegate(ts, &test_scene::input_event);
    // app_wnd.events.draw   = make_delegate(ts, &test_scene::draw_event);
    // app_wnd.events.tick   = make_delegate(ts, &test_scene::tick_event);
    // app_wnd.events.resize = make_delegate(ts, &test_scene::resize_event);
    // app_wnd.events.swap_buffers =
    //    make_delegate(renderer, &dx11_renderer::swap_buffers);
    // app_wnd.events.toggle_fullscreen =
    //    make_delegate(renderer, &dx11_renderer::toggle_fullscreen);

    app_wnd.run();

    XR_LOG_INFO("Shutting down ...");

    return 0;
}
