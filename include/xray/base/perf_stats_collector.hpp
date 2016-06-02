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

/// \file com_ptr.hpp

#include "xray/xray.hpp"
#include "xray/base/unique_handle.hpp"
#include <tbb/atomic.h>
#include <tbb/tbb_thread.h>
#include <vector>
#include <platformstl/synch/util/features.h>

#if defined(XRAY_OS_IS_WINDOWS)
#include "xray/base/windows/handle_traits.hpp"
#include <pdh.h>
#include <windows.h>
#endif

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

class stats_thread {
public:
  stats_thread();
  ~stats_thread() noexcept;

  struct process_stats_info {
    double   cpu_usage;
    uint32_t working_set;
    uint32_t thread_count;
    uint32_t virtual_bytes;
    uint32_t vbytes_peak;
    uint32_t work_set_peak;
  };

  void signal_stop() {
#if defined(XRAY_OS_IS_WINDOWS)
      SetEvent(xray::base::raw_handle(_events[1]));
#endif
  }
  process_stats_info process_stats() const noexcept;
  bool initialized() const noexcept { return _initialized.load(); }

private:
  struct thread_event {
    enum value { collect_query, shutdown };
  };

  struct counter_type {
    enum {
      cpu_usage,
      working_set,
      thread_count,
      virtual_bytes,
      virtual_bytes_peak,
      working_set_peak,
      last
    };
  };

  struct process_statistics {
#if defined(XRAY_OS_IS_WINDOWS)
    std::vector<HCOUNTER> counters;
#endif
    std::vector<double>                                            values;
#if defined(XRAY_OS_IS_WINDOWS)
    xray::base::unique_handle<xray::base::win32::pdh_query_handle> query;
#endif

    process_statistics() {
#if defined(XRAY_OS_IS_WINDOWS)
      counters.resize(counter_type::last);
#endif
      values.resize(counter_type::last);
    }
  };

  void run();

#if defined(XRAY_OS_IS_WINDOWS)
  xray::base::unique_handle<xray::base::win32::event_handle> _events[2];
#endif
  process_statistics                                         _proc_stats;
  tbb::atomic<bool> _initialized{false};
  tbb::tbb_thread   _collector_thread;

private:
  XRAY_NO_COPY(stats_thread);
};

/// @}

} // namespace base
} // namespace xray
