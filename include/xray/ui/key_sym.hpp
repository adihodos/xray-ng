//
// Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Adrian Hodos
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

//
//  All code in this file was automatically generated, DO NOT EDIT !!!

#include <cassert>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace xray {
namespace ui {

struct key_sym
{
  private:
    key_sym() = delete;
    ~key_sym() = delete;

  public:
    enum class e
    {
        unknown,
        cancel,
        backspace,
        tab,
        clear,
        enter,
        shift,
        control,
        alt,
        pause,
        caps_lock,
        escape,
        space,
        page_up,
        page_down,
        end,
        home,
        left,
        up,
        right,
        down,
        select,
        print_screen,
        insert,
        del,
        separator,
        left_win,
        right_win,
        num_lock,
        scrol_lock,
        left_shift,
        right_shift,
        left_control,
        right_control,
        left_menu,
        right_menu,
        left_alt,
        right_alt,
        key_0,
        key_1,
        key_2,
        key_3,
        key_4,
        key_5,
        key_6,
        key_7,
        key_8,
        key_9,
        key_a,
        key_b,
        key_c,
        key_d,
        key_e,
        key_f,
        key_g,
        key_h,
        key_i,
        key_j,
        key_k,
        key_l,
        key_m,
        key_n,
        key_o,
        key_p,
        key_q,
        key_r,
        key_s,
        key_t,
        key_u,
        key_v,
        key_w,
        key_x,
        key_y,
        key_z,
        kp0,
        kp1,
        kp2,
        kp3,
        kp4,
        kp5,
        kp6,
        kp7,
        kp8,
        kp9,
        kp_multiply,
        kp_add,
        kp_minus,
        kp_decimal,
        kp_divide,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7,
        f8,
        f9,
        f10,
        f11,
        f12,
        f13,
        f14,
        f15
    };

    using underlying_type = std::underlying_type<key_sym::e>::type;

    static const char* qualified_name(const key_sym::e member) noexcept;
    static const char* name(const key_sym::e member) noexcept;

    static const char* class_name() noexcept { return "key_sym::e"; }

    static underlying_type to_integer(const key_sym::e member) noexcept { return static_cast<underlying_type>(member); }

    static key_sym::e from_integer(const underlying_type ival) noexcept { return static_cast<key_sym::e>(ival); }

    using const_iterator = const e*;
    using reverse_const_iterator = std::reverse_iterator<const_iterator>;

    static constexpr size_t size = 104u;
    static const_iterator cbegin() noexcept { return &_member_entries[0]; }
    static const_iterator cend() noexcept { return &_member_entries[104]; }
    static reverse_const_iterator crbegin() noexcept { return reverse_const_iterator(cend()); }
    static reverse_const_iterator crend() noexcept { return reverse_const_iterator(cbegin()); }
    static bool is_defined(const key_sym::e val) noexcept;

  private:
    static constexpr const e _member_entries[] = { e::unknown,      e::cancel,
                                                   e::backspace,    e::tab,
                                                   e::clear,        e::enter,
                                                   e::shift,        e::control,
                                                   e::alt,          e::pause,
                                                   e::caps_lock,    e::escape,
                                                   e::space,        e::page_up,
                                                   e::page_down,    e::end,
                                                   e::home,         e::left,
                                                   e::up,           e::right,
                                                   e::down,         e::select,
                                                   e::print_screen, e::insert,
                                                   e::del,          e::separator,
                                                   e::left_win,     e::right_win,
                                                   e::num_lock,     e::scrol_lock,
                                                   e::left_shift,   e::right_shift,
                                                   e::left_control, e::right_control,
                                                   e::left_menu,    e::right_menu,
                                                   e::left_alt,     e::right_alt,
                                                   e::key_0,        e::key_1,
                                                   e::key_2,        e::key_3,
                                                   e::key_4,        e::key_5,
                                                   e::key_6,        e::key_7,
                                                   e::key_8,        e::key_9,
                                                   e::key_a,        e::key_b,
                                                   e::key_c,        e::key_d,
                                                   e::key_e,        e::key_f,
                                                   e::key_g,        e::key_h,
                                                   e::key_i,        e::key_j,
                                                   e::key_k,        e::key_l,
                                                   e::key_m,        e::key_n,
                                                   e::key_o,        e::key_p,
                                                   e::key_q,        e::key_r,
                                                   e::key_s,        e::key_t,
                                                   e::key_u,        e::key_v,
                                                   e::key_w,        e::key_x,
                                                   e::key_y,        e::key_z,
                                                   e::kp0,          e::kp1,
                                                   e::kp2,          e::kp3,
                                                   e::kp4,          e::kp5,
                                                   e::kp6,          e::kp7,
                                                   e::kp8,          e::kp9,
                                                   e::kp_multiply,  e::kp_add,
                                                   e::kp_minus,     e::kp_decimal,
                                                   e::kp_divide,    e::f1,
                                                   e::f2,           e::f3,
                                                   e::f4,           e::f5,
                                                   e::f6,           e::f7,
                                                   e::f8,           e::f9,
                                                   e::f10,          e::f11,
                                                   e::f12,          e::f13,
                                                   e::f14,          e::f15 };
};

} // namespace ui
} // namespace xray