//
// Copyright (c) 2014 Adrian Hodos
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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "xray/base/fast_delegate.hpp"
#include "xray/xray.hpp"
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

/// \brief  Stores a list of delegates. When operator() is called, the call
///         is actually forwarded to all delegates registered in the list.
template<typename return_type, typename... args_type>
class delegate_list
{
  public:
    using delegate_type = fast_delegate<return_type(args_type...)>;
    using class_type = delegate_list<return_type, args_type...>;

  public:
    /// \brief  Calls all the delegates with the supplied arguments.
    void operator()(args_type... args)
    {
        for (size_t del_idx = size_t{}, del_count = delegates_.size(); del_idx < del_count; ++del_idx) {

            auto& del_ref = delegates_[del_idx];
            del_ref(std::forward<args_type>(args)...);
        }
    }

    /// \brief  Adds a new delegate to the list.
    class_type& operator+=(const delegate_type& del)
    {
        delegates_.push_back(del);
        return *this;
    }

    /// \brief  Adds a new delegate to the list.
    class_type& operator+=(delegate_type&& del)
    {
        delegates_.emplace_back(std::move(del));
        return *this;
    }

    /// \brief  Removes a delegate from the list.
    class_type& operator-=(const delegate_type& del_to_remove)
    {
        using namespace std;
        auto itr_rem = remove(begin(delegates_), end(delegates_), del_to_remove);
        delegates_.erase(itr_rem, end(delegates_));

        return *this;
    }

  private:
    std::vector<delegate_type> delegates_;
};

/// @}

} // namespace base
} // namespace xray
