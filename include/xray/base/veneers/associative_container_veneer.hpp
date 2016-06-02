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

/// \file associative_container_veneer.hpp

#include <cstddef>
#include <initializer_list>
#include <utility>

#include "xray/xray.hpp"

namespace xray {
namespace base {

/// \addtogroup __GroupXray_Base
/// @{

/// When going out of scope, applies a user defined functor object to 
/// each element in an associative container.
/// \code 
///  //
///  // Suppose we have a pool of resources (file descriptors, sockets, etc)
///  class resource_manager {
///  private :    
///  typedef std::map<some_key_t, int> sock_pool_t;
///
///  struct sock_dtor {
///      void operator()(int sock_fd) const {
///          if (sock_fd != INVALID_SOCKET)
///              close(sock_fd);   
///      } 
///  }; 
///
///  sequence_container_veneer<sock_pool_t, sock_dtor> sock_pool_;
///  ...
///  };
///  //
///  // When the 'resource_manager' object goes out of scope, all sockets in the
///  // pool will automatically be closed.
/// \endcode
/// \note The "veneer" concept is described in detail in this paper :
///  http://synesis.com.au/resources/articles/cpp/veneers.pdf
template<typename container_type, typename element_destructor_fn>
class associative_container_veneer : public container_type {
public :
    typedef container_type                                  base_class;

    typedef typename base_class::allocator_type             allocator_type;

    typedef typename base_class::size_type                  size_type;

    typedef typename base_class::key_compare                key_compare;

    typedef typename base_class::value_type                 value_type;

    typedef associative_container_veneer
    <
        container_type, element_destructor_fn
    >                                                       class_type;

public :
    /// \name Constructors
    /// @{

    /// Default constructor.
    associative_container_veneer() 
        :       base_class{} 
    {}

    /// Construct using a specified comparison function.
    explicit associative_container_veneer(
        const key_compare& comp)
        :       base_class{comp} 
    {}

    /// Construct with user specified comparator and allocator.
    associative_container_veneer(
        const key_compare&              comp,
        const allocator_type&           alloc = allocator_type())
        :       base_class{comp, alloc} 
    {}

    /// Copy constructor.
    associative_container_veneer(
        const class_type& rhs)
        :       base_class{rhs} 
    {}

    /// Construct from rvalue (C++11 only).
    associative_container_veneer(
        class_type&& rhs)
        :       base_class{std::forward<base_class>(rhs)} 
    {}

    /// Construct from existing range of elements.
    template<typename input_iterator>
    associative_container_veneer(
        input_iterator              first,
        input_iterator              last)
        :       base_class{first, last} 
    {}

    /// Construct from existing range, with specified comparator and allocator.
    template<typename input_iterator>
    associative_container_veneer(
        input_iterator              first,
        input_iterator              last,
        const key_compare&          comp,
        const allocator_type&       alloc = allocator_type())
        :       base_class{first, last, comp, alloc} 
    {}

    /// Construct using initializer list syntax (C++11 only).
    associative_container_veneer(
        std::initializer_list<value_type>       init_list,
        const key_compare&                      comp = key_compare(),
        const allocator_type&                   alloc = allocator_type())
        :       base_class{init_list, comp, alloc} 
    {}

    /// @}

    /// Destructs the elements, calling the element destruction functor for
    /// each element.
    ~associative_container_veneer() {
        base_class* this_ptr = static_cast<base_class*>(this);
        typename base_class::iterator itr_first = this_ptr->begin();
        typename base_class::iterator itr_last = this_ptr->end();

        for (; itr_first != itr_last; ++itr_first) {
            element_destructor_fn()(itr_first->second);
        }
    }

    /// \name Assignment operators
    /// @{

    /// Self assign operator.
    class_type& operator=(const class_type& rhs) {
        base_class::operator=(rhs);
        return *this;
    }

    /// Assign from rvalue (C++11) only.
    class_type& operator=(class_type&& rhs) {
        base_class::operator=(std::forward<base_class>(rhs));
        return *this;
    }

    /// Assign from an initializer list (C++11 only).
    class_type& operator=(std::initializer_list<value_type> init_lst) {
        base_class::operator=(init_lst);
        return *this;
    }

    /// @}

private :

    void*   operator new(size_t);
    void    operator delete(void*, size_t);
};

} // namespace base
} // namespace xray
