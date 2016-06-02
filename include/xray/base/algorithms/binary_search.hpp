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

/// \file binary_search.hpp

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

#include "xray/xray.hpp"
#include "xray/base/maybe.hpp"

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

/// Binary search an ordered range of elements.
/// \param  first Iterator to the beginning of the range.
/// \param  last Iterator to the end of the range (one past the final element).
/// \param  Unary predicate that must return an integer :
///             - negative integer if the element passed in is greater then the searched element
///             - positive integer if the element passed in is smaller then the searched element
///             - 0 for equality    
template<typename random_access_iterator, typename predicate, typename value_type>
random_access_iterator 
binary_search_range(
    random_access_iterator          first,
    random_access_iterator          last,
    predicate                       query_predicate,
    const value_type&               key) NOEXCEPT
{
    random_access_iterator initial_end = last;

    while (first < last) {
        random_access_iterator middle = first + (last - first - 1) / 2;

        const xr_int32_t cmp_res = query_predicate(key, *middle);
            
        if (cmp_res < 0)
            last = middle;
        else if (cmp_res > 0)
            first = middle + 1;
        else
            return middle;
    }

    return initial_end;
}

template<typename random_access_iterator, typename predicate, typename value_type>
maybe<size_t>
binary_search_range_index(
    random_access_iterator          first,
    random_access_iterator          last,
    predicate                       query_predicate,
    const value_type&               key) NOEXCEPT
{
    random_access_iterator initial_first = first;


    while (first < last) {
        random_access_iterator middle = first + (last - first - 1) / 2;

        const xr_int32_t cmp_res = query_predicate(key, *middle);

        if (cmp_res < 0)
            last = middle;
        else if (cmp_res > 0)
            first = middle + 1;
        else
            return static_cast<size_t>(std::distance(initial_first, middle));
    }

    return nothing{};
}

template<typename random_access_iterator, typename value_type>
random_access_iterator
binary_search_range(
    random_access_iterator          first,
    random_access_iterator          last,
    const value_type&               key) NOEXCEPT
{
    random_access_iterator initial_end = last;

    while (first < last) {
        random_access_iterator middle = first + (last - first - 1) / 2;

        if (key < *middle)
            end = middle;
        else if (*middle < key)
            first = middle + 1;
        else
            return middle;
    }

    return initial_end;
}

template<typename container_type>
struct select_iterator {
    typedef typename container_type::iterator       iterator_type;
};

template<typename container_type>
struct select_iterator<const container_type> {
    typedef typename container_type::const_iterator iterator_type;
};

/// Binary search an ordered sequential container.
template<typename container_type, typename predicate, typename key_type>
typename select_iterator<container_type>::iterator_type
binary_search_container(
    container_type&             cont, 
    predicate                   query_predicate,
    const key_type&             key) NOEXCEPT
{
    static_assert(std::is_same<typename std::iterator_traits<typename container_type::iterator>::iterator_category,
                               std::random_access_iterator_tag>::value,
                 "Container argument must be a sequential container!");

    return binary_search_range(std::begin(cont), std::end(cont), query_predicate, key);
}

/// Binary search for an ordered array.
template<typename T, size_t count, typename predicate, typename key_type>
inline T* 
binary_search_container(
    T                               (&arr_ref)[count], 
    predicate                       pr__,
    const key_type&                 key) NOEXCEPT 
{
    return binary_search_range(std::begin(arr_ref), std::end(arr_ref), pr__, key);
}

/// @}

} // namespace base
} // namespace xray
