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

/// \file   fixed_pod_stack.hpp

#include "xray/xray.hpp"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace xray {
namespace base {

template <typename T, size_t MaxSize = 8>
class fixed_vector {
  /// \name Member types
  /// @{
public:
  using value_type             = T;
  using size_type              = std::size_t;
  using difference_type        = std::ptrdiff_t;
  using reference              = value_type&;
  using const_reference        = const value_type&;
  using pointer                = value_type*;
  using const_pointer          = const value_type*;
  const iterator               = pointer;
  using const_iterator         = const_pointer;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  /// @}

  /// \name Element access
  /// @{
public:
  reference operator[](const size_type idx) noexcept {
    assert(idx < size());
    return *mem_at(idx);
  }

  const_reference operator[](const size_type idx) const noexcept {
    assert(idx < size());
    return *mem_at(idx);
  }

  reference front() noexcept {
    assert(size() >= 1);
    return *mem_at(0);
  }

  const_reference front() const noexcept {
    assert(size() >= 1);
    return *mem_at(0)
  }

  reference back() noexcept {
    assert(size() >= 1);
    return *mem_at(size() - 1);
  }

  const_reference back() const noexcept {
    assert(size() >= 1);
    return *mem_at(size() - 1);
  }

  pointer       data() noexcept { return mem_at(0); }
  const_pointer data() const noexcept { return mem_at(0); }

  /// @}

  /// \name Iterators
  /// @{
public:
  iterator       begin() noexcept { return mem_at(0); }
  const_iterator begin() const noexcept { return mem_at(0); }
  const_iterator cbegin() const noexcept { return mem_at(0); }

  iterator       end() noexcept { return mem_at(size()); }
  const_iterator end() const noexcept { return mem_at(size()); }
  const_iterator cend() const noexcept { return mem_at(size()); }

  /// @}

  /// \name Capacity
  /// @{
public:
  bool                empty() const noexcept { return size() == 0; }
  size_type           size() const noexcept { return _m_count; }
  constexpr size_type max_size() const noexcept { return MaxSize; }

  /// @}

  /// \name Modifiers
  /// @{
public:
  iterator insert(const_iterator pos, const T& value);
  iterator insert(const_iterator pos, T&& value);
  /// @}

private:
  T* mem_at(const size_type idx) noexcept {
    assert(idx < MaxSize);
    return reinterpret_cast<T*>(&_m_storage[0] + idx * sizeof(T));
  }

  const T* mem_at(const size_type idx) const noexcept {
    assert(idx < MaxSize);
    return reinterpret_cast<const T*>(&_m_storage[0] + idx * sizeof(T));
  }

  void copy_move_cons(T* dst, T* src, std::true_type) {
    new (dst) T{std::move(*src)};
    src->~T();
  }

  void copy_move_cons(T* dst, T* src, std::false_type) {
    new (dst) T{*src};
    src->~T();
  }

  void shift_elements(iterator beg, const size_type offset) {
    assert(offset >= 1);

    auto dst = end() + offset - 1;
    auto cnt = end() - beg;

    while (cnt) {
      copy_move_cons(dst, beg, typename std::is_move_constructible<T>::type{});
      ++dst;
      ++beg;
      --cnt;
    }
  }

private:
  alignas(T) uint8_t _m_storage[sizeof(T) * MaxSize];
  size_type _m_count{0};
};

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::insert(const_iterator pos, const T& value) {
  assert(size() < MaxSize);

  auto itr = const_cast<iterator>(pos);
  shift_elements(itr, 1);
  new (itr) T{value};

  return itr;
}

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::insert(const_iterator pos, T&& value) {
  assert(size() < MaxSize);

  auto itr = const_cast<iterator>(pos);
  shift_elements(itr, 1);
  new (itr) T{std::move(value)};

  return itr;
}

} // namespace base
} // namespace xray
