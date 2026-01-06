#pragma once

#include <cstdint>
#include <cstddef>

#include <tl/optional.hpp>

#include "xray/base/memory.arena.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/ui/platform.window.hpp"

#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"

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

public:
	TestBDA(
		PrivateConstructionToken,
		xray::ui::PlatformWindow window,
		xray::rendering::VulkanRenderer vulkan_renderer,
		xray::rendering::GraphicsPipeline p_fsquad
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
	xray::rendering::GraphicsPipeline m_p_fsquad;
	//
	// because C++ sucks and the moves aren’t destructive, this garbage workaraound is needed.
	bool m_moved_from{false};
};

}  // namespace B5
