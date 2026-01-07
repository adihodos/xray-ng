#include "test.bda.hpp"

#include "xray/base/app_config.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"

B5::TestBDA::TestBDA(
	PrivateConstructionToken,
	xray::ui::PlatformWindow window,
	xray::rendering::VulkanRenderer vulkan_renderer,
	xray::rendering::GraphicsPipeline p_fsquad
)
	: m_window{std::move(window)}, m_renderer{std::move(vulkan_renderer)}, m_p_fsquad{std::move(p_fsquad)} {}

B5::TestBDA::TestBDA(TestBDA&& rhs) noexcept
	: m_window{std::move(rhs.m_window)},
	  m_renderer{std::move(rhs.m_renderer)},
	  m_p_fsquad{std::move(rhs.m_p_fsquad)},
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

	tl::expected<GraphicsPipeline, VulkanError> p_fsquad{
		GraphicsPipelineBuilder{scratch_pad.arena}
			.add_shader(
				ShaderStage::Vertex,
				ShaderBuildOptions{
					.code_or_file_path = ConfigSystem::instance()->shader_path("core/fullscreen.quad.glsl")
				}
			)
			.add_shader(
				ShaderStage::Fragment,
				ShaderBuildOptions{.code_or_file_path = ConfigSystem::instance()->shader_path("bda.test.fs.glsl")}
			)
			.rasterization_state({
				.poly_mode	= VK_POLYGON_MODE_FILL,
				.cull_mode	= VK_CULL_MODE_NONE,
				.front_face = VK_FRONT_FACE_CLOCKWISE,
				.line_width = 1.0f,
			})
			.depth_stencil_state(DepthStencilState{.depth_test_enable = false, .depth_write_enable = false})
			.dynamic_state({
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR,
			})
			.create_bindless(*vulkan_renderer)
	};

	if (!p_fsquad) {
		return tl::nullopt;
	}

	return tl::optional<TestBDA>{
		tl::in_place,
		PrivateConstructionToken{},
		std::move(*main_window),
		std::move(*vulkan_renderer),
		std::move(*p_fsquad),
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

	[[maybe_unused]] const FrameRenderData frame_data{m_renderer.begin_rendering(0.0f, 0.0f, 0.0f)};

	//
	// flush and bind the global descriptor table
	m_renderer.bindless_sys().flush_descriptors(m_renderer);
	m_renderer.bindless_sys().bind_descriptors(m_renderer, frame_data.cmd_buf);

	const VkViewport viewport{
		.x		  = 0.0f,
		.y		  = 0.0f,
		.width	  = frame_data.fb_f32.width,
		.height	  = frame_data.fb_f32.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(frame_data.cmd_buf, 0, 1, &viewport);

	const VkRect2D scissor{
		.offset = VkOffset2D{0, 0},
		.extent = frame_data.fbsize,
	};
	vkCmdSetScissor(frame_data.cmd_buf, 0, 1, &scissor);
	
	vkCmdBindPipeline(frame_data.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_p_fsquad.handle());
	vkCmdDraw(frame_data.cmd_buf, 3, 1, 0, 0);

	m_renderer.end_rendering();
}
