#include "xray/xray.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/containers/fixed_vector.hpp"
#include "xray/base/dbg/debug_ext.hpp"
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
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/ui/basic_gl_window.hpp"
#include "xray/ui/window_context.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <opengl/opengl.hpp>

#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>

using namespace xray::math;

TEST_CASE("fixed vector is correctly initialized", "[fixed_vector]") {
  using namespace xray::base;
  using namespace xray::math;

  SECTION("default constructor") {
    fixed_vector<scalar2i32, 8> vec{};

    REQUIRE(vec.size() == 0);
    REQUIRE(vec.max_size() == 8);
    REQUIRE(vec.empty() == true);
    REQUIRE(vec.begin() == vec.end());
  }

  SECTION("construct with default value") {
    fixed_vector<float2, 8> vec{5u};
    REQUIRE(vec.size() == 5u);
    REQUIRE(vec.empty() == false);
    REQUIRE(vec.begin() != vec.end());
  }

  SECTION("construct from single value") {
    fixed_vector<float2, 8> vec{4, float2::stdc::unit_x};
    REQUIRE(vec.size() == 4);
    REQUIRE(vec[0] == float2::stdc::unit_x);
    REQUIRE(vec[3] == float2::stdc::unit_x);
  }
}

TEST_CASE("fixed vector insertion", "[fixed_vector]") {
  using namespace xray::base;
  using namespace xray::math;
  using namespace std;

  fixed_vector<float2, 16> v{};

  SECTION("push-back") {
    v.push_back(float2::stdc::unit_x);
    v.push_back(float2::stdc::unit_y);

    REQUIRE(v.size() == 2);
    REQUIRE(v.front() == float2::stdc::unit_x);
    REQUIRE(v.back() == float2::stdc::unit_y);
    REQUIRE(v.front() == v[0]);
    REQUIRE(v.back() == v[1]);
  }

  SECTION("insert at front") {
    v.insert(v.begin(), float2::stdc::unit_x);

    REQUIRE(v.size() == 1);
    REQUIRE(v.front() == float2::stdc::unit_x);
    REQUIRE(v.front() == v[0]);
    REQUIRE(v.front() == v.back());
  }

  SECTION("insert at end") {
    v.insert(v.end(), float2::stdc::unit_y);
    REQUIRE(v.size() == 1);
    REQUIRE(v.back() == float2::stdc::unit_y);
    REQUIRE(v.back() == v[0]);
    REQUIRE(v.back() == v.front());
  }

  fixed_vector<char, 16u> v0;
  v0.push_back('a');
  v0.push_back('b');
  v0.push_back('c');
  v0.push_back('d');
  v0.push_back('e');
  v0.push_back('f');

  SECTION("insert random pos") {
    auto itr_pos = v0.insert(find(begin(v0), end(v0), 'd'), 'x');
    REQUIRE(v0.size() == 7);
    REQUIRE(*itr_pos == 'x');
    REQUIRE(v0[3] == 'x');
    REQUIRE(v0[4] == 'd');
    REQUIRE(v0[5] == 'e');
    REQUIRE(v0[6] == 'f');
  }

  SECTION("insert random pos, inserted = existing") {
    v0.insert(find(begin(v0), end(v0), 'd'), 'x', 3);
    REQUIRE(v0.size() == 9);
    REQUIRE(v0[3] == 'x');
    REQUIRE(v0[4] == 'x');
    REQUIRE(v0[5] == 'x');
    REQUIRE(v0[6] == 'd');
    REQUIRE(v0[7] == 'e');
    REQUIRE(v0[8] == 'f');
  }

  SECTION("insert random pos, inserted > existing") {
    v0.insert(find(begin(v0), end(v0), 'e'), 'x', 4);
    REQUIRE(v0.size() == 10);
    REQUIRE(v0[4] == 'x');
    REQUIRE(v0[5] == 'x');
    REQUIRE(v0[6] == 'x');
    REQUIRE(v0[7] == 'x');
    REQUIRE(v0[8] == 'e');
    REQUIRE(v0[9] == 'f');
  }

  SECTION("emplace") {
    v0.emplace(find(begin(v0), end(v0), 'd'), 'x');
    REQUIRE(v0.size() == 7);
    REQUIRE(v0[3] == 'x');
    REQUIRE(v0[4] == 'd');
    REQUIRE(v0[5] == 'e');
    REQUIRE(v0[6] == 'f');
  }
}

TEST_CASE("fixed vector removal", "[fixed_vector]") {
  using namespace xray::base;
  using namespace xray::math;
  using namespace std;

  fixed_vector<char> v{};

  SECTION("erase, empty vector is no-op") {
    auto itr = v.erase(begin(v), end(v));
    REQUIRE(itr == end(v));
  }

  v.push_back('a');
  v.push_back('b');
  v.push_back('c');
  v.push_back('d');
  v.push_back('e');
  v.push_back('f');

  SECTION("erase, @ begin") {
    auto itr = v.erase(begin(v));
    REQUIRE(*itr == 'b');
    REQUIRE(v.size() == 5);
    REQUIRE(v[0] == 'b');
  }

  SECTION("erase everything") {
    auto itr = v.erase(begin(v), end(v));
    REQUIRE(v.size() == 0);
    REQUIRE(v.empty());
    REQUIRE((void*) itr == (void*) end(v));
  }
}