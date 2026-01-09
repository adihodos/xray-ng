#include "test.bda.hpp"

#include <mutex>

#include "xray/base/xray.types.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/xray.fmt.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.packed.pushconst.hpp"

using namespace xray;

B5::TestBDA::TestBDA(
	PrivateConstructionToken,
	xray::ui::PlatformWindow window,
	xray::rendering::VulkanRenderer vulkan_renderer,
	xray::rendering::BindlessSystem compute_bindless,
	xray::rendering::VulkanPipeline p_fsquad,
	xray::rendering::VulkanPipeline p_compute,
	std::vector<SharedImage> image
)
	: m_window{std::move(window)},
	  m_renderer{std::move(vulkan_renderer)},
	  m_compute_bindless{std::move(compute_bindless)},
	  m_p_fsquad{std::move(p_fsquad)},
	  m_p_compute{std::move(p_compute)},
	  m_textures{std::move(image)} {}

B5::TestBDA::TestBDA(TestBDA&& rhs) noexcept
	: m_window{std::move(rhs.m_window)},
	  m_renderer{std::move(rhs.m_renderer)},
	  m_compute_bindless{std::move(rhs.m_compute_bindless)},
	  m_p_fsquad{std::move(rhs.m_p_fsquad)},
	  m_p_compute{std::move(rhs.m_p_compute)},
	  m_textures{std::move(rhs.m_textures)},
	  m_moved_from{std::exchange(rhs.m_moved_from, true)},
	  m_textures_layout_transitions{std::move(rhs.m_textures_layout_transitions)} {}

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

	tl::expected<QueuedJob, VulkanError> img_layout_job = vulkan_renderer->create_job(QueueType::Graphics);

	const VkPhysicalDeviceImageFormatInfo2 img_fmt_info{
		.sType	= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
		.pNext	= nullptr,
		.format = VK_FORMAT_R8G8B8A8_UINT,
		.type	= VK_IMAGE_TYPE_2D,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage	= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.flags	= 0,
	};

	VkImageFormatProperties2 img_fmt_props{
		.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2,
		.pNext = nullptr,
	};

	const VkResult img_check_res =
		vkGetPhysicalDeviceImageFormatProperties2(vulkan_renderer->physical().device, &img_fmt_info, &img_fmt_props);

	XR_LOG_INFO("Image format support {}", img_check_res == VK_SUCCESS);

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

		{
			VkImageMemoryBarrier2 img_mem_barrier{
				.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.pNext				 = nullptr,
				.srcStageMask		 = VK_PIPELINE_STAGE_2_NONE,
				.srcAccessMask		 = VK_ACCESS_2_NONE,
				.dstStageMask		 = VK_PIPELINE_STAGE_2_NONE,
				.dstAccessMask		 = VK_ACCESS_2_NONE,
				.oldLayout			 = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout			 = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image				 = image->image(),
				.subresourceRange =
					VkImageSubresourceRange{
						.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel	= 0,
						.levelCount		= 1,
						.baseArrayLayer = 0,
						.layerCount		= 1,
					},
			};

			const VkDependencyInfo dep_info{
				.sType					  = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				.pNext					  = nullptr,
				.dependencyFlags		  = VK_DEPENDENCY_BY_REGION_BIT,
				.memoryBarrierCount		  = 0,
				.pMemoryBarriers		  = nullptr,
				.bufferMemoryBarrierCount = 0,
				.pBufferMemoryBarriers	  = nullptr,
				.imageMemoryBarrierCount  = 1,
				.pImageMemoryBarriers	  = &img_mem_barrier,
			};

			vkCmdPipelineBarrier2(img_layout_job->buffer, &dep_info);
		}

		textures.emplace_back(std::move(*image), img_bindless_graphics, img_bindless_compute);
	}

	auto submit_token = vulkan_renderer->submit_job(std::move(*img_layout_job));
	vulkan_renderer->consume_wait_token(std::move(*submit_token));

	return tl::optional<TestBDA>{
		tl::in_place,
		PrivateConstructionToken{},
		std::move(*main_window),
		std::move(*vulkan_renderer),
		std::move(*compute_bindless),
		std::move(*p_fsquad),
		std::move(*p_compute),
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
	const uint32_t frame_idx = frame_data.id;

	tl::expected<QueuedJob, VulkanError> compute_job = m_renderer.create_job(QueueType::Compute);
	const VulkanRenderer::QueueData compute_queue	 = m_renderer.queue_data(QueueType::Compute);

	m_compute_bindless.flush_descriptors(m_renderer);
	m_compute_bindless.bind_descriptors(m_renderer, compute_job->buffer);

	VkImage tex_image = m_textures[frame_idx].image.image();
	const VkImageSubresourceRange tex_subresource{
		.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel	= 0,
		.levelCount		= 1,
		.baseArrayLayer = 0,
		.layerCount		= 1,
	};

	static uint32_t coord_offset = 0;

	struct CSPushConstant {
		U32 packed0;
		U32 packed1;

		std::span<const U8> as_bytes() const noexcept {
			return std::span<const U8>{reinterpret_cast<const U8*>(this), sizeof(this)};
		}
	};

	const auto [cs_bindless_handle, elements_count] =
		destructure_bindless_resource_handle(m_textures[frame_idx].bindless_compute.first);
	const CSPushConstant cs_push_const = {
		.packed0 = cs_bindless_handle,
		.packed1 = coord_offset++,
	};

	vkCmdBindPipeline(compute_job->buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_p_compute.handle());
	vkCmdPushConstants(
		compute_job->buffer,
		m_p_compute.layout(),
		VK_SHADER_STAGE_COMPUTE_BIT,
		0,
		static_cast<U32>(cs_push_const.as_bytes().size()),
		cs_push_const.as_bytes().data()
	);

	vkCmdDispatch(compute_job->buffer, frame_data.fbsize.width / 8, frame_data.fbsize.height / 8, 1);
	tl::expected<QueueSubmitWaitToken, VulkanError> cs_submit_wait_token =
		m_renderer.submit_job(std::move(*compute_job));


	VulkanRenderer::QueueData graphics_queue = m_renderer.queue_data(QueueType::Graphics);
	VkCommandBuffer cmd_buf_layout_transition{};
	{
		const VkCommandBufferAllocateInfo cmd_buf_layout_transition_alloc{
			.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext				= nullptr,
			.commandPool		= graphics_queue.cmdpool,
			.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
	
		std::unique_lock<xray::base::concurrency::spin_mutex> queue_lock{graphics_queue.cmdpool_lock};
		const VkResult alloc_cmdbuffs_res = WRAP_VULKAN_FUNC(
			vkAllocateCommandBuffers, m_renderer.device(), &cmd_buf_layout_transition_alloc, &cmd_buf_layout_transition
		);
	}
	
	const VkCommandBufferBeginInfo cmd_buf_begin_info{
		.sType			  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext			  = nullptr,
		.flags			  = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr,
	};
	vkBeginCommandBuffer(cmd_buf_layout_transition, &cmd_buf_begin_info);
	{
		VkImageMemoryBarrier2 img_mem_barrier{
			.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext				 = nullptr,
			.srcStageMask		 = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
			.srcAccessMask		 = VK_ACCESS_2_SHADER_WRITE_BIT,
			.dstStageMask		 = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			.dstAccessMask		 = VK_ACCESS_2_SHADER_READ_BIT,
			.oldLayout			 = VK_IMAGE_LAYOUT_GENERAL,
			.newLayout			 = VK_IMAGE_LAYOUT_GENERAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image				 = tex_image,
			.subresourceRange	 = tex_subresource,
		};

		const VkDependencyInfo dep_info{
			.sType					  = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext					  = nullptr,
			.dependencyFlags		  = VK_DEPENDENCY_BY_REGION_BIT,
			.memoryBarrierCount		  = 0,
			.pMemoryBarriers		  = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers	  = nullptr,
			.imageMemoryBarrierCount  = 1,
			.pImageMemoryBarriers	  = &img_mem_barrier,
		};

		vkCmdPipelineBarrier2(cmd_buf_layout_transition, &dep_info);
	}
	
	vkEndCommandBuffer(cmd_buf_layout_transition);
	
	{
		const VkSubmitInfo submit_info{
			.sType				  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext				  = nullptr,
			.waitSemaphoreCount	  = 0,
			.pWaitSemaphores	  = nullptr,
			.pWaitDstStageMask	  = nullptr,
			.commandBufferCount	  = 1,
			.pCommandBuffers	  = &cmd_buf_layout_transition,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores	  = nullptr,
		};
		std::unique_lock<xray::base::concurrency::spin_mutex> submit_lock{graphics_queue.cmdpool_lock};
		const VkResult submit_result = WRAP_VULKAN_FUNC(vkQueueSubmit, graphics_queue.handle, 1, &submit_info, nullptr);
	}

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

	const PackedU32PushConstant graphics_push_const{m_textures[frame_idx].bindless_graphics.first, 0, frame_idx};

	vkCmdPushConstants(
		frame_data.cmd_buf,
		m_p_fsquad.layout(),
		VK_SHADER_STAGE_ALL,
		0,
		graphics_push_const.size(),
		graphics_push_const.as_bytes().data()
	);

	vkCmdDraw(frame_data.cmd_buf, 3, 1, 0, 0);

	m_renderer.end_rendering();
}
