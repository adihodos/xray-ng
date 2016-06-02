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

/// \file   key_symbols.hpp

#include "xray/xray.hpp"
#include <cstdint>

namespace xray {
namespace ui {

/// \addtogroup __GroupXrayUI
/// @{

///
/// \brief System independent key constants.
struct key_symbol {
  enum {
    unknown,

    /// \name Special keys
    /// @{

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
    right_control,
    left_menu,
    right_menu,

    /// @}

    /// \name Aplha numeric keys
    /// @{

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

    /// @}

    /// \name Numpad keys
    /// @{

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

    /// @}

    /// \name Function keys
    /// @{

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
    f15,

    /// @}

    last
  };
};

extern const int32_t* scan_code_mappings_table;

inline int32_t os_key_scan_to_app_key(int32_t os_key) {
  return scan_code_mappings_table[os_key];
}

/// @}

} // namespace ui
} // namespace xray
