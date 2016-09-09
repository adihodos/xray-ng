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

#include "xray/xray.hpp"
#include <type_traits>

namespace xray {
namespace base {

template <typename IntA, typename IntB>
struct is_same_signed {
  static_assert(std::is_arithmetic<IntA>::value &&
                    std::is_arithmetic<IntB>::value,
                "Both types must be integer/floating point types!");

  enum {
    value = (std::is_signed<IntA>::value && std::is_signed<IntB>::value) ||
            (std::is_unsigned<IntA>::value && std::is_unsigned<IntB>::value)
  };
};

struct enum_helper {

  template <typename enum_class>
  static constexpr auto
  to_underlying_type(const enum_class enum_member) noexcept {
    return static_cast<typename std::underlying_type<enum_class>::type>(
        enum_member);
  }

  template <typename EnumType, typename IntType>
  static constexpr typename std::enable_if<
      std::is_integral<IntType>::value &&
          (sizeof(typename std::underlying_type<EnumType>::type) >=
           sizeof(IntType)) &&
          is_same_signed<typename std::underlying_type<EnumType>::type,
                         IntType>::value,
      EnumType>::type
  from_underlying_type(IntType int_val) noexcept {
    return static_cast<EnumType>(int_val);
  }
};

} // namespace base
} // namespace xray