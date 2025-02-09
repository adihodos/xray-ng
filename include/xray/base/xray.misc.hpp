//
// Copyright (c) Adrian Hodos
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <span>

namespace xray::base {
template<typename C>
concept ContainerLikeObject = requires(C&& c) {
    {
        c.size()
    } -> std::convertible_to<size_t>;
    c[0];
    std::is_pointer_v<decltype(c.data())>;
};

template<ContainerLikeObject C>
[[nodiscard]] inline std::span<const uint8_t>
to_bytes_span(C&& c) noexcept
{
    return std::span{ reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(c[0]) };
}

template<typename T, size_t N>
[[nodiscard]] inline constexpr std::span<const uint8_t>
to_bytes_span(const T (&array_ref)[N]) noexcept
{
    return std::span{ reinterpret_cast<const uint8_t*>(&array_ref[0]), sizeof(array_ref) };
}

template<typename T>
    requires std::is_integral_v<T>
[[nodiscard]] constexpr inline T
align(const T value, const T alignment) noexcept
{
    assert(alignment != 0);
    return (alignment - 1 + value) / alignment * alignment;
}

template<typename T>
[[nodiscard]] constexpr inline std::span<const T>
to_cspan(const T& value) noexcept
{
    return std::span<const T>{ &value, 1 };
}

std::span<std::byte>
os_virtual_alloc(const size_t block_size) noexcept;

void
os_virtual_free(std::span<std::byte> block) noexcept;

}
