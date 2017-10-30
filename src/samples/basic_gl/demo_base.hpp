//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
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

#include "xray/xray.hpp"
#include "fwd_app.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/ui/events.hpp"
#include "xray/ui/user_interface.hpp"
#include <cstdint>

namespace xray {
namespace scene {
class camera_controller;
} // namespace scene
} // namespace xray

namespace app {

class demo_base {
public:
  demo_base(const init_context_t& init_ctx);

  virtual ~demo_base();

  virtual void event_handler(const xray::ui::window_event& evt) = 0;
  virtual void poll_start(const xray::ui::poll_start_event&);
  virtual void poll_end(const xray::ui::poll_end_event&);
  virtual void loop_event(const xray::ui::window_loop_event&) = 0;

  bool     valid() const noexcept { return _valid; }
  explicit operator bool() const noexcept { return valid(); }

protected:
  bool                                                 _valid{false};
  xray::base::fast_delegate<void()>                    _quit_receiver;
  xray::base::unique_pointer<xray::ui::user_interface> _ui;
};

} // namespace app
