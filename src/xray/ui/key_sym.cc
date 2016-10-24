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

//
//  All code in this file was automatically generated, DO NOT EDIT !!!

#include "xray/xray.hpp"
#include "xray/ui/key_sym.hpp"
#include <algorithm>

namespace xray {
namespace ui {

constexpr const key_sym::e key_sym::_member_entries[];

const char* key_sym::qualified_name(const key_sym::e member) noexcept {
  switch (member) {
  case key_sym::e::unknown:
    return "key_sym::e::unknown";
    break;
  case key_sym::e::cancel:
    return "key_sym::e::cancel";
    break;
  case key_sym::e::backspace:
    return "key_sym::e::backspace";
    break;
  case key_sym::e::tab:
    return "key_sym::e::tab";
    break;
  case key_sym::e::clear:
    return "key_sym::e::clear";
    break;
  case key_sym::e::enter:
    return "key_sym::e::enter";
    break;
  case key_sym::e::shift:
    return "key_sym::e::shift";
    break;
  case key_sym::e::control:
    return "key_sym::e::control";
    break;
  case key_sym::e::alt:
    return "key_sym::e::alt";
    break;
  case key_sym::e::pause:
    return "key_sym::e::pause";
    break;
  case key_sym::e::caps_lock:
    return "key_sym::e::caps_lock";
    break;
  case key_sym::e::escape:
    return "key_sym::e::escape";
    break;
  case key_sym::e::space:
    return "key_sym::e::space";
    break;
  case key_sym::e::page_up:
    return "key_sym::e::page_up";
    break;
  case key_sym::e::page_down:
    return "key_sym::e::page_down";
    break;
  case key_sym::e::end:
    return "key_sym::e::end";
    break;
  case key_sym::e::home:
    return "key_sym::e::home";
    break;
  case key_sym::e::left:
    return "key_sym::e::left";
    break;
  case key_sym::e::up:
    return "key_sym::e::up";
    break;
  case key_sym::e::right:
    return "key_sym::e::right";
    break;
  case key_sym::e::down:
    return "key_sym::e::down";
    break;
  case key_sym::e::select:
    return "key_sym::e::select";
    break;
  case key_sym::e::print_screen:
    return "key_sym::e::print_screen";
    break;
  case key_sym::e::insert:
    return "key_sym::e::insert";
    break;
  case key_sym::e::del:
    return "key_sym::e::del";
    break;
  case key_sym::e::separator:
    return "key_sym::e::separator";
    break;
  case key_sym::e::left_win:
    return "key_sym::e::left_win";
    break;
  case key_sym::e::right_win:
    return "key_sym::e::right_win";
    break;
  case key_sym::e::num_lock:
    return "key_sym::e::num_lock";
    break;
  case key_sym::e::scrol_lock:
    return "key_sym::e::scrol_lock";
    break;
  case key_sym::e::left_shift:
    return "key_sym::e::left_shift";
    break;
  case key_sym::e::right_shift:
    return "key_sym::e::right_shift";
    break;
  case key_sym::e::left_control:
    return "key_sym::e::left_control";
    break;
  case key_sym::e::right_control:
    return "key_sym::e::right_control";
    break;
  case key_sym::e::left_menu:
    return "key_sym::e::left_menu";
    break;
  case key_sym::e::right_menu:
    return "key_sym::e::right_menu";
    break;
  case key_sym::e::left_alt:
    return "key_sym::e::left_alt";
    break;
  case key_sym::e::right_alt:
    return "key_sym::e::right_alt";
    break;
  case key_sym::e::key_0:
    return "key_sym::e::key_0";
    break;
  case key_sym::e::key_1:
    return "key_sym::e::key_1";
    break;
  case key_sym::e::key_2:
    return "key_sym::e::key_2";
    break;
  case key_sym::e::key_3:
    return "key_sym::e::key_3";
    break;
  case key_sym::e::key_4:
    return "key_sym::e::key_4";
    break;
  case key_sym::e::key_5:
    return "key_sym::e::key_5";
    break;
  case key_sym::e::key_6:
    return "key_sym::e::key_6";
    break;
  case key_sym::e::key_7:
    return "key_sym::e::key_7";
    break;
  case key_sym::e::key_8:
    return "key_sym::e::key_8";
    break;
  case key_sym::e::key_9:
    return "key_sym::e::key_9";
    break;
  case key_sym::e::key_a:
    return "key_sym::e::key_a";
    break;
  case key_sym::e::key_b:
    return "key_sym::e::key_b";
    break;
  case key_sym::e::key_c:
    return "key_sym::e::key_c";
    break;
  case key_sym::e::key_d:
    return "key_sym::e::key_d";
    break;
  case key_sym::e::key_e:
    return "key_sym::e::key_e";
    break;
  case key_sym::e::key_f:
    return "key_sym::e::key_f";
    break;
  case key_sym::e::key_g:
    return "key_sym::e::key_g";
    break;
  case key_sym::e::key_h:
    return "key_sym::e::key_h";
    break;
  case key_sym::e::key_i:
    return "key_sym::e::key_i";
    break;
  case key_sym::e::key_j:
    return "key_sym::e::key_j";
    break;
  case key_sym::e::key_k:
    return "key_sym::e::key_k";
    break;
  case key_sym::e::key_l:
    return "key_sym::e::key_l";
    break;
  case key_sym::e::key_m:
    return "key_sym::e::key_m";
    break;
  case key_sym::e::key_n:
    return "key_sym::e::key_n";
    break;
  case key_sym::e::key_o:
    return "key_sym::e::key_o";
    break;
  case key_sym::e::key_p:
    return "key_sym::e::key_p";
    break;
  case key_sym::e::key_q:
    return "key_sym::e::key_q";
    break;
  case key_sym::e::key_r:
    return "key_sym::e::key_r";
    break;
  case key_sym::e::key_s:
    return "key_sym::e::key_s";
    break;
  case key_sym::e::key_t:
    return "key_sym::e::key_t";
    break;
  case key_sym::e::key_u:
    return "key_sym::e::key_u";
    break;
  case key_sym::e::key_v:
    return "key_sym::e::key_v";
    break;
  case key_sym::e::key_w:
    return "key_sym::e::key_w";
    break;
  case key_sym::e::key_x:
    return "key_sym::e::key_x";
    break;
  case key_sym::e::key_y:
    return "key_sym::e::key_y";
    break;
  case key_sym::e::key_z:
    return "key_sym::e::key_z";
    break;
  case key_sym::e::kp0:
    return "key_sym::e::kp0";
    break;
  case key_sym::e::kp1:
    return "key_sym::e::kp1";
    break;
  case key_sym::e::kp2:
    return "key_sym::e::kp2";
    break;
  case key_sym::e::kp3:
    return "key_sym::e::kp3";
    break;
  case key_sym::e::kp4:
    return "key_sym::e::kp4";
    break;
  case key_sym::e::kp5:
    return "key_sym::e::kp5";
    break;
  case key_sym::e::kp6:
    return "key_sym::e::kp6";
    break;
  case key_sym::e::kp7:
    return "key_sym::e::kp7";
    break;
  case key_sym::e::kp8:
    return "key_sym::e::kp8";
    break;
  case key_sym::e::kp9:
    return "key_sym::e::kp9";
    break;
  case key_sym::e::kp_multiply:
    return "key_sym::e::kp_multiply";
    break;
  case key_sym::e::kp_add:
    return "key_sym::e::kp_add";
    break;
  case key_sym::e::kp_minus:
    return "key_sym::e::kp_minus";
    break;
  case key_sym::e::kp_decimal:
    return "key_sym::e::kp_decimal";
    break;
  case key_sym::e::kp_divide:
    return "key_sym::e::kp_divide";
    break;
  case key_sym::e::f1:
    return "key_sym::e::f1";
    break;
  case key_sym::e::f2:
    return "key_sym::e::f2";
    break;
  case key_sym::e::f3:
    return "key_sym::e::f3";
    break;
  case key_sym::e::f4:
    return "key_sym::e::f4";
    break;
  case key_sym::e::f5:
    return "key_sym::e::f5";
    break;
  case key_sym::e::f6:
    return "key_sym::e::f6";
    break;
  case key_sym::e::f7:
    return "key_sym::e::f7";
    break;
  case key_sym::e::f8:
    return "key_sym::e::f8";
    break;
  case key_sym::e::f9:
    return "key_sym::e::f9";
    break;
  case key_sym::e::f10:
    return "key_sym::e::f10";
    break;
  case key_sym::e::f11:
    return "key_sym::e::f11";
    break;
  case key_sym::e::f12:
    return "key_sym::e::f12";
    break;
  case key_sym::e::f13:
    return "key_sym::e::f13";
    break;
  case key_sym::e::f14:
    return "key_sym::e::f14";
    break;
  case key_sym::e::f15:
    return "key_sym::e::f15";
    break;

  default:
    assert(false && "Unknown enum member!");
    break;
  }

  return "unknown enum member";
}

const char* key_sym::name(const key_sym::e member) noexcept {
  switch (member) {
  case key_sym::e::unknown:
    return "unknown";
    break;
  case key_sym::e::cancel:
    return "cancel";
    break;
  case key_sym::e::backspace:
    return "backspace";
    break;
  case key_sym::e::tab:
    return "tab";
    break;
  case key_sym::e::clear:
    return "clear";
    break;
  case key_sym::e::enter:
    return "enter";
    break;
  case key_sym::e::shift:
    return "shift";
    break;
  case key_sym::e::control:
    return "control";
    break;
  case key_sym::e::alt:
    return "alt";
    break;
  case key_sym::e::pause:
    return "pause";
    break;
  case key_sym::e::caps_lock:
    return "caps_lock";
    break;
  case key_sym::e::escape:
    return "escape";
    break;
  case key_sym::e::space:
    return "space";
    break;
  case key_sym::e::page_up:
    return "page_up";
    break;
  case key_sym::e::page_down:
    return "page_down";
    break;
  case key_sym::e::end:
    return "end";
    break;
  case key_sym::e::home:
    return "home";
    break;
  case key_sym::e::left:
    return "left";
    break;
  case key_sym::e::up:
    return "up";
    break;
  case key_sym::e::right:
    return "right";
    break;
  case key_sym::e::down:
    return "down";
    break;
  case key_sym::e::select:
    return "select";
    break;
  case key_sym::e::print_screen:
    return "print_screen";
    break;
  case key_sym::e::insert:
    return "insert";
    break;
  case key_sym::e::del:
    return "del";
    break;
  case key_sym::e::separator:
    return "separator";
    break;
  case key_sym::e::left_win:
    return "left_win";
    break;
  case key_sym::e::right_win:
    return "right_win";
    break;
  case key_sym::e::num_lock:
    return "num_lock";
    break;
  case key_sym::e::scrol_lock:
    return "scrol_lock";
    break;
  case key_sym::e::left_shift:
    return "left_shift";
    break;
  case key_sym::e::right_shift:
    return "right_shift";
    break;
  case key_sym::e::left_control:
    return "left_control";
    break;
  case key_sym::e::right_control:
    return "right_control";
    break;
  case key_sym::e::left_menu:
    return "left_menu";
    break;
  case key_sym::e::right_menu:
    return "right_menu";
    break;
  case key_sym::e::left_alt:
    return "left_alt";
    break;
  case key_sym::e::right_alt:
    return "right_alt";
    break;
  case key_sym::e::key_0:
    return "key_0";
    break;
  case key_sym::e::key_1:
    return "key_1";
    break;
  case key_sym::e::key_2:
    return "key_2";
    break;
  case key_sym::e::key_3:
    return "key_3";
    break;
  case key_sym::e::key_4:
    return "key_4";
    break;
  case key_sym::e::key_5:
    return "key_5";
    break;
  case key_sym::e::key_6:
    return "key_6";
    break;
  case key_sym::e::key_7:
    return "key_7";
    break;
  case key_sym::e::key_8:
    return "key_8";
    break;
  case key_sym::e::key_9:
    return "key_9";
    break;
  case key_sym::e::key_a:
    return "key_a";
    break;
  case key_sym::e::key_b:
    return "key_b";
    break;
  case key_sym::e::key_c:
    return "key_c";
    break;
  case key_sym::e::key_d:
    return "key_d";
    break;
  case key_sym::e::key_e:
    return "key_e";
    break;
  case key_sym::e::key_f:
    return "key_f";
    break;
  case key_sym::e::key_g:
    return "key_g";
    break;
  case key_sym::e::key_h:
    return "key_h";
    break;
  case key_sym::e::key_i:
    return "key_i";
    break;
  case key_sym::e::key_j:
    return "key_j";
    break;
  case key_sym::e::key_k:
    return "key_k";
    break;
  case key_sym::e::key_l:
    return "key_l";
    break;
  case key_sym::e::key_m:
    return "key_m";
    break;
  case key_sym::e::key_n:
    return "key_n";
    break;
  case key_sym::e::key_o:
    return "key_o";
    break;
  case key_sym::e::key_p:
    return "key_p";
    break;
  case key_sym::e::key_q:
    return "key_q";
    break;
  case key_sym::e::key_r:
    return "key_r";
    break;
  case key_sym::e::key_s:
    return "key_s";
    break;
  case key_sym::e::key_t:
    return "key_t";
    break;
  case key_sym::e::key_u:
    return "key_u";
    break;
  case key_sym::e::key_v:
    return "key_v";
    break;
  case key_sym::e::key_w:
    return "key_w";
    break;
  case key_sym::e::key_x:
    return "key_x";
    break;
  case key_sym::e::key_y:
    return "key_y";
    break;
  case key_sym::e::key_z:
    return "key_z";
    break;
  case key_sym::e::kp0:
    return "kp0";
    break;
  case key_sym::e::kp1:
    return "kp1";
    break;
  case key_sym::e::kp2:
    return "kp2";
    break;
  case key_sym::e::kp3:
    return "kp3";
    break;
  case key_sym::e::kp4:
    return "kp4";
    break;
  case key_sym::e::kp5:
    return "kp5";
    break;
  case key_sym::e::kp6:
    return "kp6";
    break;
  case key_sym::e::kp7:
    return "kp7";
    break;
  case key_sym::e::kp8:
    return "kp8";
    break;
  case key_sym::e::kp9:
    return "kp9";
    break;
  case key_sym::e::kp_multiply:
    return "kp_multiply";
    break;
  case key_sym::e::kp_add:
    return "kp_add";
    break;
  case key_sym::e::kp_minus:
    return "kp_minus";
    break;
  case key_sym::e::kp_decimal:
    return "kp_decimal";
    break;
  case key_sym::e::kp_divide:
    return "kp_divide";
    break;
  case key_sym::e::f1:
    return "f1";
    break;
  case key_sym::e::f2:
    return "f2";
    break;
  case key_sym::e::f3:
    return "f3";
    break;
  case key_sym::e::f4:
    return "f4";
    break;
  case key_sym::e::f5:
    return "f5";
    break;
  case key_sym::e::f6:
    return "f6";
    break;
  case key_sym::e::f7:
    return "f7";
    break;
  case key_sym::e::f8:
    return "f8";
    break;
  case key_sym::e::f9:
    return "f9";
    break;
  case key_sym::e::f10:
    return "f10";
    break;
  case key_sym::e::f11:
    return "f11";
    break;
  case key_sym::e::f12:
    return "f12";
    break;
  case key_sym::e::f13:
    return "f13";
    break;
  case key_sym::e::f14:
    return "f14";
    break;
  case key_sym::e::f15:
    return "f15";
    break;

  default:
    assert(false && "Unknown enum member");
    break;
  }

  return "unknown enum member";
}

bool key_sym::is_defined(const key_sym::e val) noexcept {
  using namespace std;
  return find(key_sym::cbegin(), key_sym::cend(), val) != key_sym::cend();
}

} // end ns
} // end ns