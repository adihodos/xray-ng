#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <bitset>

#include <tl/optional.hpp>

#include "xray/base/memory.arena.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/ui/platform.window.hpp"

#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"

namespace xray::ui {
class user_interface;
struct mouse_button_event;
struct mouse_motion_event;
struct window_event;
};	// namespace xray::ui

namespace B5 {
struct RenderEvent;
struct InitContext;

class TestBDA {
private:
	struct PrivateConstructionToken {
		explicit PrivateConstructionToken() = default;
	};

	//
	// images used in both compute and graphics queues and pipelines
	struct SharedImage {
		xray::rendering::VulkanImage image;
		xray::rendering::BindlessImageResourceHandleEntryPair bindless_graphics;
		xray::rendering::BindlessStorageImageResourceHandleEntryPair bindless_compute;
	};

public:
	TestBDA(
		PrivateConstructionToken,
		xray::ui::PlatformWindow window,
		xray::rendering::VulkanRenderer vulkan_renderer,
		xray::rendering::BindlessSystem compute_bindless,
		xray::rendering::VulkanPipeline p_fsquad,
		xray::rendering::VulkanPipeline p_compute,
		std::vector<SharedImage> images
	);
	TestBDA(TestBDA&&) noexcept;
	~TestBDA();

	static tl::optional<TestBDA> create();

	void run();
	void event_handler(const xray::ui::window_event& evt);
	void loop_event(const xray::ui::window_loop_event&);
	void poll_start(const xray::ui::poll_start_event&) {}
	void poll_end(const xray::ui::poll_end_event&) {}

private:
	xray::ui::PlatformWindow m_window;
	xray::rendering::VulkanRenderer m_renderer;
	xray::rendering::BindlessSystem m_compute_bindless;
	xray::rendering::VulkanPipeline m_p_fsquad;
	xray::rendering::VulkanPipeline m_p_compute;
	std::vector<SharedImage> m_textures;
	std::bitset<16> m_textures_layout_transitions{0};
	uint32_t m_face_index{};
	//
	// because C++ sucks and the moves aren’t destructive, this garbage workaround is needed.
	bool m_moved_from{false};
};

}  // namespace B5
