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

#pragma once

/// \file window.hpp

#pragma once

#include <cstdint>
#include <span>

#include <tl/expected.hpp>

#include "xray/xray.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/window_params.hpp"
#include "xray/ui/window.common.core.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"

namespace xray {
namespace ui {

struct PlatformWindowError {};

class PlatformWindow {
public:
	struct PlatformImpl;
	WindowCommonCore core{};
	/// \name Construction and destruction.
	/// @{
public:
	explicit PlatformWindow(xray::base::unique_pointer<PlatformImpl> impl);
	PlatformWindow(PlatformWindow&&) noexcept;
	~PlatformWindow();

	/// @}

	static tl::expected<PlatformWindow, PlatformWindowError> create(const window_params_t& win_params);

	xray::rendering::WindowPlatformData platform_data() const noexcept;

	void disable_cursor() noexcept;
	void enable_cursor() noexcept;

	int32_t width() const noexcept;
	int32_t height() const noexcept;

	std::span<const GamepadAxisInfo> gamepad_axis_info() const noexcept;

	void message_loop();
	void quit() noexcept;

private:
	xray::base::unique_pointer<PlatformImpl> _platform;

	XRAY_NO_COPY(PlatformWindow);
};

}  // namespace ui
}  // namespace xray
