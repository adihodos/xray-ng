#include "test.bda.hpp"

#include "xray/base/app_config.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/xray.fmt.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"

B5::TestBDA::TestBDA(
	PrivateConstructionToken,
	xray::ui::PlatformWindow window,
	xray::rendering::VulkanRenderer vulkan_renderer,
	xray::rendering::BindlessSystem compute_bindless,
	xray::rendering::VulkanPipeline p_fsquad,
	std::vector<SharedImage> image
)
	: m_window{std::move(window)},
	  m_renderer{std::move(vulkan_renderer)},
	  m_compute_bindless{std::move(compute_bindless)},
	  m_p_fsquad{std::move(p_fsquad)},
	  m_textures{std::move(image)} {}

B5::TestBDA::TestBDA(TestBDA&& rhs) noexcept
	: m_window{std::move(rhs.m_window)},
	  m_renderer{std::move(rhs.m_renderer)},
	  m_compute_bindless{std::move(rhs.m_compute_bindless)},
	  m_p_fsquad{std::move(rhs.m_p_fsquad)},
	  m_textures{std::move(rhs.m_textures)},
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
	// const auto slot_null_tex = vulkan_renderer->bindless_sys().reserve_image_slots(1);
	// assert(slot_null_tex == 0);

	const VkPushConstantRange compute_bindless_push_consts[] = {
		VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
			.offset		= 0,
			.size		= static_cast<uint32_t>(2 * sizeof(uint32_t)),
		},
	};

	const LayoutBindingsByResourceType compute_bindless_set_layouts[] = {
		LayoutBindingsByResourceType{
			.res_type		  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptor_count = 16,
			.stage_flags	  = VK_SHADER_STAGE_ALL,
			.tag			  = "Compute_DS_storage_image",
		},
	};

	tl::expected<BindlessSystem, VulkanError> compute_bindless = BindlessSystem::create(
		*scratch_pad.arena,
		BindlessSystem::Kind::Compute,
		vulkan_renderer->device(),
		vulkan_renderer->physical().properties.descriptor_indexing,
		compute_bindless_set_layouts,
		compute_bindless_push_consts
	);

	if (!compute_bindless) {
		return tl::nullopt;
	}

	tl::expected<VulkanPipeline, VulkanError> p_compute =
		VulkanPipelineBuilder{scratch_pad.arena}
			.add_shader(
				ShaderStage::Compute,
				ShaderBuildOptions{.code_or_file_path = ConfigSystem::instance()->shader_path("bda.compute.glsl")}
			)
			.create(
				*vulkan_renderer,
				VulkanPipelineKind::Compute,
				VulkanPipelineTemplate{
					.layout					= compute_bindless->pipeline_layout(),
					.descriptor_set_layouts = compute_bindless->descriptor_set_layouts(),
				}
			);

	if (!p_compute) {
		return tl::nullopt;
	}

	tl::expected<VulkanPipeline, VulkanError> p_fsquad{
		VulkanPipelineBuilder{scratch_pad.arena}
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
			.create(
				*vulkan_renderer,
				VulkanPipelineKind::Graphics,
				VulkanPipelineTemplate{
					.layout					= vulkan_renderer->bindless_sys().pipeline_layout(),
					.descriptor_set_layouts = vulkan_renderer->bindless_sys().descriptor_set_layouts(),
				}
			)
	};

	if (!p_fsquad) {
		return tl::nullopt;
	}

	const VkExtent2D fb_size  = vulkan_renderer->surface_state().caps.currentExtent;
	const uint32_t max_frames = vulkan_renderer->max_inflight_frames();
	std::vector<SharedImage> textures;
	textures.reserve(max_frames);

	char scratch_buffer[1024];

	for (uint32_t idx = 0; idx < max_frames; ++idx) {
		format_to_n(scratch_buffer, "tex_{}", idx);
		tl::expected<VulkanImage, VulkanError> image = VulkanImage::from_memory(
			*vulkan_renderer,
			xray::rendering::VulkanImageCreateInfo{
				.tag_name	 = scratch_buffer,
				.wpkg		 = tl::nullopt,
				.type		 = VK_IMAGE_TYPE_2D,
				.usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
				.format		 = VK_FORMAT_R8G8B8A8_UNORM,
				.width		 = fb_size.width,
				.height		 = fb_size.height,
			}
		);

		if (!image) {
			return tl::nullopt;
		}

		auto img_bindless_graphics = vulkan_renderer->bindless_sys().add_image(*image, nullptr, tl::nullopt);
		auto img_bindless_compute  = compute_bindless->add_storage_image(*image, tl::nullopt);

		textures.emplace_back(std::move(*image), img_bindless_graphics, img_bindless_compute);
	}

	return tl::optional<TestBDA>{
		tl::in_place,
		PrivateConstructionToken{},
		std::move(*main_window),
		std::move(*vulkan_renderer),
		std::move(*compute_bindless),
		std::move(*p_fsquad),
		std::move(textures),
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

	// tl::expected<QueuedJob, VulkanError> compute_job = m_renderer.create_job(QueueType::Compute);
	// const QueueData compute_queue					 = m_renderer.queue_data(QueueType::Compute);

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
