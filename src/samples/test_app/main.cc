#include "xray/xray.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/delegate_list.hpp"
#include "xray/base/fast_delegate.hpp"
#include "xray/base/maybe.hpp"
#include "xray/base/nothing.hpp"
#include "xray/base/pod_zero.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar2x3.hpp"
#include "xray/math/scalar2x3_math.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r2.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/ui/basic_gl_window.hpp"
#include "xray/ui/window_context.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <opengl/opengl.hpp>

void basic_draw_func(const xray::ui::window_context&) {
  gl::Clear(gl::COLOR_BUFFER_BIT);
  gl::ClearColor(1.0f, 0.25f, 0.5f, 1.0f);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const xray::math::scalar3<T>& a) {
  os << "[" << a.x << ", " << a.y << ", " << a.z << "]" << std::endl;
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const xray::math::scalar2<T>& a) {
  os << "[" << a.x << ", " << a.y << "]" << std::endl;
  return os;
}

using namespace xray::math;

// template <typename T>
// bool lookat_rh(
//     const scalar3<T>& eye_pos, const scalar3<T>& target,
//     const scalar3<T>& world_up) noexcept {

//   const auto D = normalize(target - eye_pos);
//   const auto U = world_up - project_unit(world_up, D);
//   const auto R = cross(D, U);

//   // [R U -D E]

//   return true;
// }

int main(int, char**) {
  // xray::ui::basic_window wnd{xray::ui::render_params_t{
  //     xray::ui::api_debug_state::enabled, xray::ui::api_info::version}};

  // wnd.draw_event_delegate = xray::base::make_delegate(basic_draw_func);

  // wnd.pump_messages();

  using namespace std;
  using namespace xray::math;

  // {
  //   cout << "\n #### scalar2 #### " << endl;
  //   constexpr auto v0 = stdc<float2>::null;
  //   constexpr auto v1 = float2{1.0f, 1.0f};

  //   for (uint32_t i = 0; i <= 10; ++i) {
  //     const auto c      = (1.0f / 10.0f) * static_cast<float>(i);
  //     const auto interp = mix(v0, v1, c);
  //     cout << interp;
  //   }
  // }

  // {
  //   cout << "\n #### scalar3 #### " << endl;
  //   const auto v1 = stdc<float3>::unit_y;
  //   const auto v2 = float3{1.0f, 1.0f, 1.0f};

  //   for (uint32_t i = 0; i <= 10; ++i) {
  //     const auto tval = (1.0f / 10.0f) * static_cast<float>(i);
  //     const auto v    = mix(stdc<float3>::null, v2, tval);
  //     cout << v;
  //   }

  //   lookat_rh(
  //       stdc<float3>::null, float3{0.0f, 0.0f, 10.0f}, stdc<float3>::unit_y);

  //   const auto res = triple_scalar_product(
  //       stdc<float3>::unit_x, stdc<float3>::unit_y, stdc<float3>::unit_z);
  //   cout << (res < 0.0f ? "Left handed" : "Right handed") << endl;
  // }

  // {
  //   const auto m0 = R2::translate(10.0f, 1.0f);
  //   const auto v1 = mul_point(m0, stdc<float2>::unit_x);
  //   const auto v2 = mul_vec(m0, stdc<float2>::unit_x);

  //   std::cout << "V1 = " << v1 << "\n";
  //   std::cout << "V2 = " << v2 << "\n";
  // }

  // {
  //   const auto m =
  //       R3::rotate_axis_angle(normalize(float3{1.0f, 1.0f, 1.0f}), 0.1f);
  //   const auto v = mul_vec(m, {1.0f, 3.0f, -4.0f});
  //   cout << v << "\n";
  // }

  {
    const auto m = R4::translate(1.0f, 2.0f, 3.0f);
    const auto v = mul_point(m, float3{3.0f, 9.0f, 1.0f});
    cout << v << "\n";
  }

  return 0;
}
