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

/// \file   scoped_guard.hpp

#include <utility>
#include "xray/xray.hpp"

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

///
/// Allow conditional execution of code when going out of a scope.
/// Largely based on the implementation presented here :
/// http://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Andrei-Alexandrescu-Systematic-Error-Handling-in-C
template <typename fun_type>
class scoped_guard {
public:
  typedef scoped_guard<fun_type> class_type;

public:
  explicit scoped_guard(fun_type action) noexcept : action_{std::move(action)} {
  }

  scoped_guard(class_type&& rhs) noexcept : action_{std::move(rhs.action_)},
                                            active_{rhs.active_} {
    rhs.dismiss();
  }

  ~scoped_guard() noexcept {
    if (active_)
      action_();
  }

  void dismiss() noexcept { active_ = false; }

private:
  fun_type action_;
  bool active_{true};

private:
  scoped_guard() = delete;
  scoped_guard(const class_type&) = delete;
  class_type& operator=(const class_type&) = delete;
  class_type& operator=(class_type&&) = delete;
};

template <typename fun_type>
scoped_guard<fun_type> make_scope_guard(fun_type action) noexcept {
  return scoped_guard<fun_type>(std::move(action));
}

namespace detail {
enum class scoped_guard_helper {};

template <typename fun_type>
scoped_guard<fun_type> operator+(scoped_guard_helper, fun_type&& fn) noexcept {
  return make_scope_guard<fun_type>(std::forward<fun_type>(fn));
}
}

/// @}

}  // namespace base
}  // namespace xray

#define XRAY_SCOPE_EXIT                                                        \
  auto XRAY_ANONYMOUS_VARIABLE(XRAY_SCOPE_EXIT_STATE) =                        \
      ::xray::base::detail::scoped_guard_helper{} + [&]()
