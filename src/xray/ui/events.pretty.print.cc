
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
#include "xray/ui/events.gamepad.hpp"

// auto
// fmt::formatter<xray::ui::mouse_button>::format(const xray::ui::mouse_button value, format_context& ctx) const
//     -> format_context::iterator
// {
//     fmt::string_view name{ "unknown" };
//     using namespace xray::ui;
// 
// #define MOUSE_BUTTON_ENUM_ENTRY(x)                                                                                     \
//     case x: {                                                                                                          \
//         name = std::string_view{ #x };                                                                                 \
//     } break
// 
//     switch (value) {
//         MOUSE_BUTTON_ENUM_ENTRY(mouse_button::button1);
//         MOUSE_BUTTON_ENUM_ENTRY(mouse_button::button2);
//         MOUSE_BUTTON_ENUM_ENTRY(mouse_button::button3);
//         MOUSE_BUTTON_ENUM_ENTRY(mouse_button::button4);
//         MOUSE_BUTTON_ENUM_ENTRY(mouse_button::button5);
//         default:
//             break;
//     }
// #undef MOUSE_BUTTON_ENUM_ENTRY
// 
//     return formatter<string_view>::format(name, ctx);
// }
// 
// auto
// fmt::formatter<xray::ui::GamepadAxis>::format(const xray::ui::GamepadAxis value, format_context& ctx) const
//     -> format_context::iterator
// {
//     fmt::string_view name{ "unknown" };
//     using namespace xray::ui;
// 
// #define GAMEPAD_AXIS_ENUM_ENTRY(x)                                                                                     \
//     case x: {                                                                                                          \
//         name = std::string_view{ #x };                                                                                 \
//     } break
// 
//     switch (value) {
//         GAMEPAD_AXIS_ENUM_ENTRY(GamepadAxis::LeftX);
//         GAMEPAD_AXIS_ENUM_ENTRY(GamepadAxis::LeftY);
//         GAMEPAD_AXIS_ENUM_ENTRY(GamepadAxis::RightX);
//         GAMEPAD_AXIS_ENUM_ENTRY(GamepadAxis::RightY);
//         GAMEPAD_AXIS_ENUM_ENTRY(GamepadAxis::LeftZ);
//         GAMEPAD_AXIS_ENUM_ENTRY(GamepadAxis::RightZ);
//         default:
//             break;
//     }
// #undef GAMEPAD_AXIS_ENUM_ENTRY
//     return formatter<string_view>::format(name, ctx);
// }
// 
// auto
// fmt::formatter<xray::ui::GamepadButton>::format(const xray::ui::GamepadButton value, format_context& ctx) const
//     -> format_context::iterator
// {
//     fmt::string_view name{ "unknown" };
//     using namespace xray::ui;
// 
// #define GAMEPAD_BUTTON_ENUM_ENTRY(x)                                                                                   \
//     case x: {                                                                                                          \
//         name = std::string_view{ #x };                                                                                 \
//     } break
// 
//     switch (value) {
//         GAMEPAD_BUTTON_ENUM_ENTRY(GamepadButton::Left);
//         GAMEPAD_BUTTON_ENUM_ENTRY(GamepadButton::Left2);
//         GAMEPAD_BUTTON_ENUM_ENTRY(GamepadButton::Right);
//         GAMEPAD_BUTTON_ENUM_ENTRY(GamepadButton::Right2);
//         default:
//             break;
//     }
// #undef GAMEPAD_BUTTON_ENUM_ENTRY
//     return formatter<string_view>::format(name, ctx);
// }
// 
// auto
// fmt::formatter<xray::ui::event_action_type>::format(const xray::ui::event_action_type value, format_context& ctx) const
//     -> format_context::iterator
// {
//     fmt::string_view name{ "unknown" };
//     using namespace xray::ui;
// 
//     switch (value) {
//         default:
//         case event_action_type::press:
//             name = "event_action_type::press";
//             break;
// 
//         case event_action_type::release:
//             name = "event_action_type::release";
//             break;
//     }
//     return formatter<string_view>::format(name, ctx);
// }
