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
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <type_traits>
#include <cstdint>
#include <random>

namespace xray {
namespace base {

///
/// \brief Simple wrapper around stdlib random number generator.
class random_number_generator
{
  public:
    random_number_generator() = default;

    float next_float(const float rmin, const float rmax) noexcept { return next(rmin, rmax); }
    uint32_t next_uint(const uint32_t rmin, const uint32_t rmax) noexcept { return next(rmin, rmax); }
    int32_t next_int(const int32_t rmin, const int32_t rmax) noexcept { return next(rmin, rmax); }

    template<typename T>
    T next(const T rmin, const T rmax) noexcept
        requires std::is_arithmetic_v<T>
    {
        return static_cast<T>(rmin + (rmax - rmin) * _distribution(_engine));
    }

  private:
    std::random_device _rdevice{};
    std::mt19937 _engine{ _rdevice() };
    std::uniform_real_distribution<double> _distribution{ 0.0f, 1.0f };
};

} // namespace base
} // namespace xray
