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

/// \file maybe.hpp

#include <algorithm>
#include <cassert>

#include "xray/xray.hpp"
#include "xray/base/nothing.hpp"

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

/// A class that may hold a valid value of some type. It is similar to
/// Haskell's maybe or Nullable<T> from C#.
template <typename value_type>
class maybe {

  /// \name Defined types.
  /// @{

public:
  using class_type = maybe<value_type>;

  /// @}

  /// \name Constructors
  /// @{

public:
  maybe(nothing) noexcept : has_value_{false} {}

  /// Construct from existing value.
  maybe(const value_type& existing_value) : has_value_{true} {
    new (&stored_value_) value_type(existing_value);
  }

  /// Construct from a temporary value.
  maybe(value_type&& other_val) : has_value_{true} {
    new (&stored_value_) value_type(std::move(other_val));
  }

  /// Construct from a temporary object of this class.
  maybe(maybe<value_type>&& other_obj) : has_value_{other_obj.has_value()} {
    if (has_value()) {
      new (&stored_value_) value_type(std::move(other_obj.stored_value_));
      other_obj.has_value_ = false;
    }
  }

  /// \brief  Assign from temporary object.
  class_type& operator=(class_type&& other) {
    if (other.has_value()) {

      if (has_value())
        stored_value_.~value_type();

      new (&stored_value_) value_type(std::move(other.stored_value_));
      other.has_value_ = false;
      has_value_       = true;

    } else {
      if (has_value()) {
        stored_value_.~value_type();
      }

      has_value_ = false;
    }

    return *this;
  }

  /// \brief  Assign from existing value.
  class_type& operator=(value_type& other) {
    if (has_value())
      stored_value_.~value_type();

    new (&stored_value_) value_type(other);
    has_value_ = true;

    return *this;
  }

  /// \brief  Assign from a temporary value.
  class_type& operator=(value_type&& other) {
    if (has_value())
      stored_value_.~value_type();

    new (&stored_value_) value_type(std::move(other));
    has_value_ = true;

    return *this;
  }

  ~maybe() {
    if (has_value())
      stored_value_.~value_type();
  }

  /// @}

  /// \name State/sanity.
  /// @{

public:
  /// Test if object holds a valid value.
  explicit operator bool() const noexcept { return has_value(); }

  /// Test if object holds a valid value.
  bool has_value() const noexcept { return has_value_; }

  /// @}

  /// \name Stored value access.
  /// @{

public:
  /// Access the stored value. Check if the object stored a valid value first.
  value_type& value() noexcept {
    assert(has_value());
    return stored_value_;
  }

  /// Access the stored value. Check if the object stored a valid value first.
  const value_type& value() const noexcept {
    assert(has_value());
    return stored_value_;
  }

  /// @}

  /// \name Data members.
  /// @{

private:
  bool has_value_;

  union {
    value_type stored_value_;
    nothing    null_value_;
  };

  /// @}

  /// \name Deleted member functions.
  /// @{

private:
  XRAY_NO_COPY(maybe<value_type>);

  /// @}
};

template <typename value_type>
const value_type& value_of(const maybe<value_type>& opt) noexcept {
  return opt.value();
}

template <typename value_type>
value_type& value_of(maybe<value_type>& opt) noexcept {
  return opt.value();
}

/// @}

} // namespace base
} // namespace xray
