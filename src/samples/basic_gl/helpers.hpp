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
#include <cstdint>
#include <type_traits>

namespace app {

template <typename T>
inline const void* offset_cast(const uint32_t off) noexcept {
  static_assert(
      std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value ||
          std::is_same<T, uint32_t>::value,
      "Only uint8_t, uint16_t and uint32_t supported for index buffers!");

  return static_cast<const char*>(nullptr) + off * sizeof(T);
}

template <typename Container>
inline size_t bytes_size(const Container& c) noexcept {
  return c.size() * sizeof(c[0]);
}

template <typename T, size_t N>
inline constexpr size_t bytes_size(const T (&/*arr_ref*/)[N]) noexcept {
  return N * sizeof(T);
}

template <typename T>
inline constexpr size_t bytes_size(const T* data, const size_t count) noexcept {
  return count * sizeof(data[0]);
}
}
