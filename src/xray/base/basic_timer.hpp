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
    using underlying_clock_type = std::chrono::steady_clock;
    static_assert(underlying_clock_type::is_steady == true, "Underlying clock type must be a steady clock type!");
    using timepoint_type = typename underlying_clock_type::time_point;

    basic_timer() noexcept = default;

    void tick() noexcept
    {
        end_ = current_;
        current_ = underlying_clock_type::now();
        interval_ = current_ - end_;
        delta_time_ = interval_.count();
        elapsed_since_start_ += time_scale_ * delta_time_;
    }

    precise_type delta_time() const noexcept { return delta_time_ * time_scale_; }
    precise_type delta_time_unscaled() const noexcept { return delta_time_; }
    precise_type time_since_start() const noexcept { return elapsed_since_start_; }
    void set_timescale(const precise_type ts) noexcept { time_scale_ = ts; }
    void scale(const float s) noexcept { time_scale_ *= s; }

    timepoint_type timepoint_start() const noexcept { return start_; }
    timepoint_type timepoint_end() const noexcept { return end_; }

  private:
    timepoint_type start_{ underlying_clock_type::now() };
    timepoint_type end_{ start_ };
    timepoint_type current_{ start_ };
    std::chrono::duration<precise_type, std::milli> interval_{ 0 };
    precise_type time_scale_{ 1.0 };
    precise_type delta_time_{ 0.0 };
    precise_type elapsed_since_start_{ 0.0 };
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
    }

    ~scoped_timing_object() { timer_->tick(); }

  private:
    timer_type* timer_;

  private:
    XRAY_NO_COPY(scoped_timing_object);
};

} // namespace base
} // namespace xray
