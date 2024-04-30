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

/// \file   fixed_pod_stack.hpp

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

#include "xray/base/containers/fixed_pod_vector.hpp"
#include "xray/xray.hpp"

namespace xray {
namespace base {

///
/// \brief Adaptor class, similar to std::stack, for fixed size POD containers.
/// The underlying container must support the back(), push_back() and pop_back()
/// operations.
template<typename T, size_t k_max_size = 64U, typename container = fixed_pod_vector<T, k_max_size>>
class fixed_pod_stack
{

    /// \name Defined types.
    /// @{

  public:
    typedef container container_type;
    typedef typename container::value_type value_type;
    typedef typename container::size_type size_type;
    typedef typename container::reference reference;
    typedef typename container::const_reference const_reference;
    typedef fixed_pod_stack<T, k_max_size, container> class_type;

    /// @}

    /// \name Constructos.
    /// @{

  public:
    static_assert(std::is_pod<T>::value, "POD-only container!");

    fixed_pod_stack() noexcept
        : backend_{}
    {
    }

    explicit fixed_pod_stack(const container_type& cont) noexcept
        : backend_{ cont }
    {
    }

    /// @}

    /// \name Attributes.
    /// @{

  public:
    bool empty() const noexcept { return backend_.empty(); }

    size_t size() const noexcept { return backend_.size(); }

    reference top() noexcept
    {
        assert(!empty());
        return backend_.back();
    }

    const_reference top() const noexcept
    {
        assert(!empty());
        return backend_.back();
    }

    /// @}

    /// \name Operations.
    /// @{

  public:
    void pop() noexcept
    {
        assert(!empty());
        backend_.pop_back();
    }

    void push(const T& val) noexcept
    {
        assert(size() < k_max_size);
        backend_.push_back(val);
    }

    /// @}

    /// \name Data members.
    /// @{

  private:
    container backend_;

    /// @}
};

} // namespace base
} // namespace xray
