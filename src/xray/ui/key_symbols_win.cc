#include "xray/ui/key_symbols.hpp"

namespace xray {
namespace ui {

static const int32_t os_scan_code_to_key_symbol[256] = {
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::cancel,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::backspace,    key_symbol::tab,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::clear,        key_symbol::enter,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::shift,        key_symbol::control,
    key_symbol::alt,          key_symbol::pause,
    key_symbol::caps_lock,    key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::escape,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::space,        key_symbol::page_up,
    key_symbol::page_down,    key_symbol::end,
    key_symbol::home,         key_symbol::left,
    key_symbol::up,           key_symbol::right,
    key_symbol::down,         key_symbol::select,
    key_symbol::print_screen, key_symbol::unknown,
    key_symbol::unknown,      key_symbol::insert,
    key_symbol::del,          key_symbol::unknown,
    key_symbol::key_0,        key_symbol::key_1,
    key_symbol::key_2,        key_symbol::key_3,
    key_symbol::key_4,        key_symbol::key_5,
    key_symbol::key_6,        key_symbol::key_7,
    key_symbol::key_8,        key_symbol::key_9,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::key_a,
    key_symbol::key_b,        key_symbol::key_c,
    key_symbol::key_d,        key_symbol::key_e,
    key_symbol::key_f,        key_symbol::key_g,
    key_symbol::key_h,        key_symbol::key_i,
    key_symbol::key_j,        key_symbol::key_k,
    key_symbol::key_l,        key_symbol::key_m,
    key_symbol::key_n,        key_symbol::key_o,
    key_symbol::key_p,        key_symbol::key_q,
    key_symbol::key_r,        key_symbol::key_s,
    key_symbol::key_t,        key_symbol::key_u,
    key_symbol::key_v,        key_symbol::key_w,
    key_symbol::key_x,        key_symbol::key_y,
    key_symbol::key_z,        key_symbol::left_win,
    key_symbol::right_win,    key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::kp0,          key_symbol::kp1,
    key_symbol::kp2,          key_symbol::kp3,
    key_symbol::kp4,          key_symbol::kp5,
    key_symbol::kp6,          key_symbol::kp7,
    key_symbol::kp8,          key_symbol::kp9,
    key_symbol::kp_multiply,  key_symbol::kp_add,
    key_symbol::separator,    key_symbol::kp_minus,
    key_symbol::kp_decimal,   key_symbol::kp_divide,
    key_symbol::f1,           key_symbol::f2,
    key_symbol::f3,           key_symbol::f4,
    key_symbol::f5,           key_symbol::f6,
    key_symbol::f7,           key_symbol::f8,
    key_symbol::f9,           key_symbol::f10,
    key_symbol::f11,          key_symbol::f12,
    key_symbol::f13,          key_symbol::f14,
    key_symbol::f15,          key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::num_lock,     key_symbol::scrol_lock,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::left_shift,   key_symbol::right_shift,
    key_symbol::unknown,      key_symbol::right_control,
    key_symbol::left_menu,    key_symbol::right_menu,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown,
    key_symbol::unknown,      key_symbol::unknown};

const int32_t* scan_code_mappings_table = os_scan_code_to_key_symbol;

} // namespace ui
} // namespace xray
