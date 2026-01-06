#include "test.bda.hpp"

#include "xray/base/app_config.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"

B5::TestBDA::TestBDA(
	PrivateConstructionToken, xray::ui::PlatformWindow window, xray::rendering::VulkanRenderer vulkan_renderer
)
	: m_window{std::move(window)}, m_renderer{std::move(vulkan_renderer)} {}

B5::TestBDA::TestBDA(TestBDA&& rhs) noexcept
	: m_window{std::move(rhs.m_window)},
	  m_renderer{std::move(rhs.m_renderer)},
	  m_moved_from{std::exchange(rhs.m_moved_from, true)} {}

B5::TestBDA::~TestBDA() {
	if (!m_moved_from) {
		m_renderer.wait_device_idle();
	}
}

tl::optional<B5::TestBDA> B5::TestBDA::create() {
	using namespace xray::base;
	using namespace xray::ui;
	using namespace xray::rendering;

	const window_params_t wnd_params{
		"Vulkan Demo",
		4,
		5,
		24,
		8,
		32,
		0,
		1,
		false,
	};

	tl::expected<PlatformWindow, PlatformWindowError> main_window{PlatformWindow::create(wnd_params)};
	if (!main_window) {
		return tl::nullopt;
	}

	const RendererConfig render_config{
		RendererConfig::from_file(ConfigSystem::instance()->config_root() / "renderer.conf")
	};

	ScratchPadArena scratch_pad = ThreadLocalContext::acquire_scratchpad({});
	tl::optional<VulkanRenderer> vulkan_renderer{
		VulkanRenderer::create(*scratch_pad.arena, WindowPlatformData{main_window->platform_data()}, render_config)
	};

	if (!vulkan_renderer) {
		return tl::nullopt;
	}

	vulkan_renderer->add_shader_include_directories({ConfigSystem::instance()->shader_root()});
	const auto slot_null_tex = vulkan_renderer->bindless_sys().reserve_image_slots(1);
	assert(slot_null_tex == 0);

	return tl::optional<TestBDA>{
		tl::in_place,
		PrivateConstructionToken{},
		std::move(*main_window),
		std::move(*vulkan_renderer),
	};
}

void B5::TestBDA::run() {
	m_window.core.events.loop		= cpp::bind<&TestBDA::loop_event>(this);
	m_window.core.events.poll_start = cpp::bind<&TestBDA::poll_start>(this);
	m_window.core.events.poll_end	= cpp::bind<&TestBDA::poll_end>(this);
	m_window.core.events.window		= cpp::bind<&TestBDA::event_handler>(this);

	m_window.message_loop();
}

void B5::TestBDA::event_handler(const xray::ui::window_event& wnd_evt) {
	using namespace xray::ui;

	if (is_input_event(wnd_evt)) {
		if (wnd_evt.event.key.keycode == xray::ui::KeySymbol::escape &&
			wnd_evt.event.key.type == event_action_type::press) {
			m_window.quit();
			return;
		}
	}
}

void B5::TestBDA::loop_event(const xray::ui::window_loop_event&) {
	using namespace xray::rendering;

	[[maybe_unused]] const FrameRenderData frame_data{m_renderer.begin_rendering(0.5f, 0.25f, 0.0f)};
	m_renderer.end_rendering();
}
