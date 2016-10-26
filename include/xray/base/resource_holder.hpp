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

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

/// \brief Helper class to allow usage of unique_ppointer objects with
/// non pointer resource types.
template <typename _ResType, _ResType _NullValue>
struct resource_holder {
public:
  resource_holder(_ResType handle) noexcept : _handle_to_resource{handle} {}

  resource_holder(std::nullptr_t = nullptr) noexcept
      : _handle_to_resource{_NullValue} {}

  operator _ResType() noexcept { return _handle_to_resource; }

  explicit operator bool() const noexcept {
    return _handle_to_resource != _NullValue;
  }

  bool operator==(const resource_holder<_ResType, _NullValue>& rhs) const
      noexcept {
    return _handle_to_resource == rhs._handle_to_resource;
  }

  bool operator!=(const resource_holder<_ResType, _NullValue>& rhs) const
      noexcept {
    return !(*this == rhs);
  }

private:
  _ResType _handle_to_resource;
};

/// @}

} // namespace base
} // namespace xray
