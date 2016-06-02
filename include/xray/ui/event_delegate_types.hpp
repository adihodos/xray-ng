#pragma once

#include <cstddef>
#include <cstdint>

#include "xray/xray.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/ui/fwd_input_events.hpp"

namespace xray {
namespace ui {

struct window_context;

using draw_delegate_type = base::fast_delegate<void(const window_context&)>;

//using keydown_delegate_type = base::fast_delegate<void(const int32_t)>;

using tick_delegate_type = base::fast_delegate<void(const float)>;

using debug_delegate_type =
    base::fast_delegate<void(const int32_t, const int32_t, const uint32_t,
                             const int32_t, const size_t, const char*)>;

using window_size_delegate =
    base::fast_delegate<void(const int32_t, const int32_t)>;

//using key_delegate =
//    base::fast_delegate<void(const int32_t, const int32_t, const int32_t)>;

//using mouse_button_delegate =
//    base::fast_delegate<void(const int32_t, const int32_t, const int32_t)>;

//using mouse_scroll_delegate =
//    base::fast_delegate<void(const float, const float)>;

//using mouse_enter_exit_delegate = base::fast_delegate<void(const bool)>;

//using mouse_move_delegate = base::fast_delegate<void(const float, const float)>;

using input_event_delegate = base::fast_delegate<void(const input_event_t&)>;

} // namespace ui
} // namespace xray
