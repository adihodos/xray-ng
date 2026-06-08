//
// Copyright (c) Adrian Hodos
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
#include "xray/base/xray.os.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/expected.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/serialization/serialization.hpp"
#include "xray/scene/scene.description.hpp"

#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/axis.aligned.bounding.box.hpp"

#include "xray/ui/platform.window.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"

#include "hud.config.hpp"
#include "hud.test.hpp"

#include "xray/ui/platform.window.hpp"

// https://brevzin.github.io/c++/2025/06/26/json-reflection/

#include <format>
#include <print>
#include <thread>

#include "system.memory.hpp"

struct MonkaError : xray::base::ErrorLocationInfo {};
struct GigaError : xray::base::ErrorLocationInfo {};

struct MonkaType {
	xray::I32 ival{1024};
	xray::F32 fval{3.3f};

	// MonkaType() noexcept = default;
	// MonkaType(xray::I32 i, xray::F32 f) noexcept : ival{i}, fval{f} {}

	void do_something() const { std::println("MonkaType - do something"); }
};

xray::base::Expected<MonkaType, MonkaError> create_monkas(xray::I32 val) noexcept {
	if (val > 1000) {
		return xray::base::Expected<MonkaType, MonkaError>{xray::base::ExpectedInPlaceTag{}, 0xaa, 9999.9f};
	} else {
		return xray::base::Expected<MonkaType, MonkaError>{MonkaError{XRAY_MARK_ERROR_LOCATION()}};
	}
}

int main(int argc, char** argv) {
	xray::base::xray_init();

	XR_LOG_INFO("Starting up ...");

	using namespace xray;
	using namespace xray::base;
	using namespace xray::math;
	using namespace xray::ui;
	using namespace xray::rendering;

	XR_LOG_INFO("Config root = %s", ConfigSystem::instance()->config_root().c_str());

	auto main_arena		   = B5::GlobalMemorySystem::instance()->grab_large_arena();
	PlatformWindow* window = PlatformWindow::create(
		*main_arena.arena,
		window_params_t{
			.title = "FootMadeHero",
		}
	);
	if (!window) {
		return EXIT_FAILURE;
	}

	auto vulkan_renderer = VulkanRenderer::create(*main_arena.arena, window->platform_data(), RendererConfig{});
	if (!vulkan_renderer) {
		return EXIT_FAILURE;
	}

	while (window->core.state.flags.quit == false) {
		window->tick();

		// XR_LOG_INFO("Frame %zu", window->core.frame);

		if (const ISIZE esc_idx = static_cast<ISIZE>(KeySymbol::escape); window->core.keys[esc_idx].was_pressed) {
			std::println("ESC pressed, closing");
			window->quit();
			break;
		}

		for (ISIZE keysym = static_cast<ISIZE>(KeySymbol::first); keysym < static_cast<ISIZE>(KeySymbol::count);
			 ++keysym) {
			if (window->core.keys[keysym].is_down) {
				if (window->core.key_names[keysym].len != 0) {
					std::println(
						"{}",
						std::string_view{
							window->core.key_names[keysym].name, static_cast<size_t>(window->core.key_names[keysym].len)
						}
					);
				}
			}
		}

		const FrameRenderData frame_render_data = vulkan_renderer->start_frame();
		vulkan_renderer->begin_rendering(frame_render_data, 0.0f, 0.0f, 1.0f);
		vulkan_renderer->end_rendering();
		std::this_thread::yield();
	}

	return 0;
}
