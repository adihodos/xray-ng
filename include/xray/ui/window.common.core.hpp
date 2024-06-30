#pragma once

#include "xray/xray.hpp"
#include "xray/ui/events.hpp"

namespace xray::ui {

enum class InputKeySymHandling
{
    Utf8,
    Unicode
};

struct WindowCommonCore
{
    InputKeySymHandling key_sym_handling{ InputKeySymHandling::Unicode };
    struct
    {
        loop_event_delegate loop;
        window_event_delegate window;
        poll_start_event_delegate poll_start;
        poll_end_event_delegate poll_end;
    } events;
};

}
