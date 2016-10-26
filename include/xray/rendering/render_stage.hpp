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
namespace rendering {

struct render_stage {
private:
  render_stage()  = delete;
  ~render_stage() = delete;

public:
  enum class e { vertex, tess_control, tess_eval, geometry, fragment, compute };

  using underlying_type = std::underlying_type<render_stage::e>::type;

  static const char* qualified_name(const render_stage::e member) noexcept;
  static const char* name(const render_stage::e member) noexcept;

  static const char* class_name() noexcept { return "render_stage::e"; }

  static underlying_type to_integer(const render_stage::e member) noexcept {
    return static_cast<underlying_type>(member);
  }

  static render_stage::e from_integer(const underlying_type ival) noexcept {
    return static_cast<render_stage::e>(ival);
  }

  using const_iterator         = const e*;
  using reverse_const_iterator = std::reverse_iterator<const_iterator>;

  static constexpr size_t size = 6u;
  static const_iterator   cbegin() noexcept { return &_member_entries[0]; }
  static const_iterator   cend() noexcept { return &_member_entries[6]; }
  static reverse_const_iterator crbegin() noexcept {
    return reverse_const_iterator(cend());
  }
  static reverse_const_iterator crend() noexcept {
    return reverse_const_iterator(cbegin());
  }
  static bool is_defined(const render_stage::e val) noexcept;

private:
  static constexpr const e _member_entries[] = {e::vertex,
                                                e::tess_control,
                                                e::tess_eval,
                                                e::geometry,
                                                e::fragment,
                                                e::compute};
};

} // namespace rendering
} // namespace xray
