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
#include "xray/xray_types.hpp"
#include <chrono>

namespace xray {
namespace base {

/**
 * @brief Basic timer class.
 */
template<typename precise_type>
class basic_timer
{
  public:
    basic_timer()
        : start_{ std::chrono::high_resolution_clock::now() }
        , end_{ std::chrono::high_resolution_clock::now() }
    {
    }

    void start() { start_ = std::chrono::high_resolution_clock::now(); }

    void end()
    {
        end_ = std::chrono::high_resolution_clock::now();
        interval_ = end_ - start_;
    }

    void update_and_reset()
    {
        end();
        start();
    }

    precise_type elapsed_millis() const { return interval_.count(); }

  private:
    using timepoint_type = std::chrono::time_point<std::chrono::high_resolution_clock>;

    timepoint_type start_;
    timepoint_type end_;
    std::chrono::duration<precise_type, std::milli> interval_;
};

using timer_stdp = basic_timer<scalar_lowp>;
using timer_highp = basic_timer<scalar_mediump>;

template<typename timer_type>
struct scoped_timing_object
{
  public:
    explicit scoped_timing_object(timer_type* timer) noexcept
        : timer_{ timer }
    {
        timer_->start();
    }

    ~scoped_timing_object() { timer_->end(); }

  private:
    timer_type* timer_;

  private:
    XRAY_NO_COPY(scoped_timing_object);
};

} // namespace base
} // namespace xray
