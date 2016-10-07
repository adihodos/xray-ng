//
// Copyright (c) 2011-2016 Adrian Hodos
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

/// \file   fixed_vector.hpp

#include "xray/xray.hpp"
#include <cassert>
#include <cstddef>
#include <initializer_list>
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
  using iterator               = pointer;
  using const_iterator         = const_pointer;
  using reverse_iterator       = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  /// @}

  /// \name Construction
  /// @{
public:
  fixed_vector() = default;

  fixed_vector(const size_type cnt, const T& value);

  template <typename InputIterator>
  fixed_vector(InputIterator first, InputIterator last);

  explicit fixed_vector(std::initializer_list<T> init_list);

  explicit fixed_vector(const size_type count);

  ~fixed_vector();

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
    return *mem_at(0);
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
  iterator begin() noexcept { return mem_at(0); }

  const_iterator begin() const noexcept { return mem_at(0); }

  const_iterator cbegin() const noexcept { return mem_at(0); }

  iterator end() noexcept { return mem_at(size()); }

  const_iterator end() const noexcept { return mem_at(size()); }

  const_iterator cend() const noexcept { return mem_at(size()); }

  reverse_iterator rbegin() noexcept { return reverse_iterator{end()}; }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator{end()};
  }
  const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator{cend()};
  }

  reverse_iterator rend() noexcept { return reverse_iterator{begin()}; }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator{begin()};
  }
  const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator{cbegin()};
  }

  /// @}

  /// \name Capacity
  /// @{
public:
  bool empty() const noexcept { return size() == 0; }

  size_type size() const noexcept { return _m_count; }

  constexpr size_type max_size() const noexcept { return MaxSize; }

  /// @}

  /// \name Modifiers
  /// @{
public:
  iterator insert(const_iterator pos, const T& value);

  iterator insert(const_iterator pos, const T& value, size_t cnt);

  iterator insert(const_iterator pos, T&& value);

  iterator insert(const_iterator pos, std::initializer_list<T> ilist);

  template <typename InputIterator>
  iterator insert(const_iterator pos, InputIterator first, InputIterator last);

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args);

  template <typename... Args>
  iterator emplace_back(Args&&... args);

  iterator erase(const_iterator pos);

  iterator erase(const_iterator first, const_iterator last);

  void push_back(const T& value);

  void push_back(T&& value);

  void pop_back();

  void clear();

  /// @}

private:
  T* mem_at(const size_type idx) noexcept {
    // assert(idx < MaxSize);
    return reinterpret_cast<T*>(&_m_storage[0] + idx * sizeof(T));
  }

  const T* mem_at(const size_type idx) const noexcept {
    // assert(idx < MaxSize);
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

  void shift_elements(iterator dst, iterator first, const_iterator last);

private:
  alignas(T) uint8_t _m_storage[sizeof(T) * MaxSize];
  size_type _m_count{0};

private:
  XRAY_NO_COPY(fixed_vector);
};

template <typename T, size_t MaxSize>
fixed_vector<T, MaxSize>::fixed_vector(const size_type cnt, const T& value) {
  insert(begin(), value, cnt);
}

template <typename T, size_t MaxSize>
fixed_vector<T, MaxSize>::fixed_vector(const size_type count) {
  insert(begin(), T{}, count);
}

template <typename T, size_t MaxSize>
template <typename InputIterator>
fixed_vector<T, MaxSize>::fixed_vector(InputIterator first,
                                       InputIterator last) {
  insert(begin(), first, last);
}

template <typename T, size_t MaxSize>
fixed_vector<T, MaxSize>::fixed_vector(std::initializer_list<T> init_list) {
  insert(begin(), std::move(init_list));
}

template <typename T, size_t MaxSize>
fixed_vector<T, MaxSize>::~fixed_vector() {
  clear();
}

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::insert(const_iterator pos, const T& value) {
  return insert(pos, value, 1);
}

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::insert(const_iterator pos, const T& value,
                                 size_t cnt) {
  assert(((cnt + size()) < max_size()) && "Capacity exceeded!");
  assert(cnt != 0);

  shift_elements(end() + cnt - 1, end() - 1, pos);
  // auto last    = end() + cnt - 1;
  // auto old_end = end();

  // while (old_end > pos) {
  //   copy_move_cons(last--, --old_end,
  //                  typename std::is_move_constructible<T>::type{});
  // }

  while (cnt) {
    new ((void*) pos) T{value};
    --cnt;
    ++pos;
    ++_m_count;
  }

  return begin() + (pos - begin());
}

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::insert(const_iterator pos, T&& value) {
  assert(((1 + size()) <= max_size()) && "Capacity exceeded!");

  shift_elements(end(), end() - 1, pos);
  // auto last    = end();
  // auto old_end = end();

  // while (old_end > pos) {
  //   copy_move_cons(last--, --old_end,
  //                  typename std::is_move_constructible<T>::type{});
  // }

  new ((void*) pos) T{std::move(value)};
  ++_m_count;

  return begin() + (pos - begin());
}

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::insert(const_iterator pos,
                                 std::initializer_list<T> ilist) {
  return insert(pos, std::begin(ilist), std::end(ilist));
}

template <typename T, size_t MaxSize>
template <typename InputIterator>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::insert(const_iterator pos, InputIterator first,
                                 InputIterator last) {
  assert(((size() + (last - first)) <= max_size()) && "Capacity exceeded");
  assert((first <= last) && "Invalid input range!");

  if (first == last)
    return begin() + (pos - begin());

  // auto new_end = end() + (last - first) - 1;
  // auto old_end = end();

  // while (old_end > pos) {
  //   copy_move_cons(new_end--, --old_end,
  //                  typename std::is_move_constructible<T>::type{});
  // }
  shift_elements(end() + (last - first) - 1, end() - 1, pos);

  auto cons_itr = begin() + (pos - begin());

  while (first != last) {
    new (cons_itr) T{*first};
    ++cons_itr;
    ++first;
    ++_m_count;
  }

  return begin() + (pos - begin());
}

template <typename T, size_t MaxSize>
void fixed_vector<T, MaxSize>::push_back(const T& value) {
  insert(end(), value);
}

template <typename T, size_t MaxSize>
void fixed_vector<T, MaxSize>::push_back(T&& value) {
  insert(end(), std::move(value));
}

template <typename T, size_t MaxSize>
template <typename... Args>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::emplace(const_iterator pos, Args&&... args) {
  assert(((1 + size()) <= max_size()) && "Capacity exceeded!");

  // auto last    = end();
  // auto old_end = end();

  // while (old_end > pos) {
  //   copy_move_cons(last--, --old_end,
  //                  typename std::is_move_constructible<T>::type{});
  // }

  shift_elements(end(), end() - 1, pos);
  new ((void*) pos) T{std::forward<Args>(args)...};
  ++_m_count;

  return begin() + (pos - begin());
}

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::erase(const_iterator pos) {
  assert(pos < end());
  return erase(pos, pos + 1);
}

template <typename T, size_t MaxSize>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::erase(const_iterator first, const_iterator last) {
  assert(first <= last);
  assert(last <= end());

  //
  //  destroy [first, last)
  {
    auto erase_first = first;
    auto erase_last  = last;

    while (erase_first < erase_last) {
      (*erase_first).~T();
      ++erase_first;
    }
  }

  //
  //  move [last, end) to [first, end)
  auto end_iter = end();
  auto dst_iter = begin() + (first - begin());
  auto src_iter = begin() + (last - begin());

  while (src_iter < end_iter) {
    copy_move_cons(dst_iter++, src_iter++,
                   typename std::is_move_constructible<T>::type{});
  }

  _m_count -= last - first;
  return (first == end_iter) ? end() : (begin() + (first - begin()));
}

template <typename T, size_t MaxSize>
void fixed_vector<T, MaxSize>::clear() {
  erase(begin(), end());
}

template <typename T, size_t MaxSize>
template <typename... Args>
typename fixed_vector<T, MaxSize>::iterator
fixed_vector<T, MaxSize>::emplace_back(Args&&... args) {
  return emplace(end(), std::forward<Args>(args)...);
}

template <typename T, size_t MaxSize>
void fixed_vector<T, MaxSize>::pop_back() {
  assert(!empty() && "Popback requires non empty container !!!");
  erase(begin() + size() - 1, end());
}

template <typename T, size_t MaxSize>
void fixed_vector<T, MaxSize>::shift_elements(iterator dst, iterator first,
                                              const_iterator last) {
  while (first >= last) {
    copy_move_cons(dst--, first--,
                   typename std::is_move_constructible<T>::type{});
  }
}

} // namespace base
} // namespace xray
