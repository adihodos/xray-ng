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

#include "xray/xray.hpp"
#include <initializer_list>
#include <type_traits>

namespace xray {
namespace math {

struct X
{};
struct Y
{};
struct Z
{};
struct W
{};

template<typename T>
constexpr size_t
type_to_component_index()
{
    if constexpr (std::is_same_v<T, X>) {
        return 0;
    } else if constexpr (std::is_same_v<T, Y>) {
        return 1;
    } else if constexpr (std::is_same_v<T, Z>) {
        return 2;
    } else if constexpr (std::is_same_v<T, W>) {
        return 3;
    }
}

template<typename T>
struct Swizzle2
{
    T x;
    T y;
};

template<typename T>
struct Swizzle3
{
    T x;
    T y;
    T z;
};

template<typename T>
struct Swizzle4
{
    T x;
    T y;
    T z;
    T w;
};

template<typename T, size_t Rank>
struct SwizzleBase
{
    static constexpr bool check_rank_restriction(std::initializer_list<size_t> indices) noexcept
    {
        for (auto i : indices) {
            if (i >= Rank)
                return false;
        }
        return true;
    }

    template<typename... Ts>
    static constexpr auto swizzle(const T (&components)[Rank], Ts...) noexcept
    {
        constexpr size_t rank = sizeof...(Ts);
        constexpr size_t indices[] = { type_to_component_index<Ts>()... };

        static_assert(SwizzleBase::check_rank_restriction({ type_to_component_index<Ts>()... }),
                      "Vector does not have component!");

        if constexpr (rank == 4) {
            return Swizzle4<T>{
                components[indices[0]], components[indices[1]], components[indices[2]], components[indices[3]]
            };
        }

        if constexpr (rank == 3) {
            return Swizzle3<T>{ components[indices[0]], components[indices[1]], components[indices[2]] };
        }

        if constexpr (rank == 2) {
            return Swizzle2<T>{ components[indices[0]], components[indices[1]] };
        }
    }
};

}
}
