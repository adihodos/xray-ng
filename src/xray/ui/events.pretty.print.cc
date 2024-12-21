
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

#include "xray/ui/events.pretty.print.hpp"

auto
fmt::formatter<xray::ui::mouse_button>::format(const xray::ui::mouse_button value, format_context& ctx) const
    -> format_context::iterator
{
    fmt::string_view name{ "unknown" };

#define xr_mouse_button_enum_entry(x)                                                                                  \
    case x: {                                                                                                          \
        name = std::string_view{ #x };                                                                                 \
    } break

    switch (value) {
        xr_mouse_button_enum_entry(xray::ui::mouse_button::button1);
        xr_mouse_button_enum_entry(xray::ui::mouse_button::button2);
        xr_mouse_button_enum_entry(xray::ui::mouse_button::button3);
        xr_mouse_button_enum_entry(xray::ui::mouse_button::button4);
        xr_mouse_button_enum_entry(xray::ui::mouse_button::button5);
        default:
            break;
    }

    return formatter<string_view>::format(name, ctx);
}
