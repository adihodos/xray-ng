//
// Copyright (c) 2011 - 2016 Adrian Hodos
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

/// \file   fixed_stack.hpp

#include "xray/xray.hpp"
#include "xray/base/containers/fixed_vector.hpp"

namespace xray {
namespace base {

template <typename T, size_t MaxSize = 8>
class fixed_stack {
  /// \name Member types
  /// @{
public:
  using container_type  = fixed_vector<T, MaxSize>;
  using value_type      = typename container_type::value_type;
  using size_type       = typename container_type::size_type;
  using reference       = typename container_type::reference;
  using const_reference = typename container_type::const_reference;
  /// @}

  /// \name Construction
  /// @{
public:
  fixed_stack()  = default;
  ~fixed_stack() = default;

  /// @}

  /// \name Element access
  /// @{
public:
  reference top() noexcept {
    assert(!empty());
    return _m_container.back();
  }

  const_reference top() const noexcept {
    assert(!empty());
    return _m_container.back();
  }
  /// @}

  /// \name Capacity
  /// @{
public:
  bool      empty() const noexcept { return _m_container.empty(); }
  size_type size() const noexcept { return _m_container.size(); }

  /// @}

  /// \name Modifiers
  /// @{
public:
  template <typename... Args>
  void emplace(Args&&... args) {
    _m_container.emplace_back(std::forward<Args>(args)...);
  }

  void push(const T& value) { _m_container.push_back(value); }
  void push(T&& value) { _m_container.push_back(std::forward<Args>(args)...); }

  void pop() {
    assert(!empty());
    _m_container.pop_back();
  }
  /// @}

private:
  container_type _m_container;

private:
  XRAY_NO_COPY(fixed_stack);
};

} // namespace base
} // namespace xray
