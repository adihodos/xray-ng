#include "xray/base/perf_stats_collector.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/debug_output.hpp"
#include <fmt/format.h>
#include <platformstl/filesystem/path.hpp>
#include <platformstl/filesystem/path_functions.hpp>
#include <platformstl/system/system_traits.hpp>
#include <stlsoft/memory/auto_buffer.hpp>
#include <type_traits>

using namespace xray::base;

xray::base::stats_thread::stats_thread() {
#if defined(XRAY_OS_IS_WINDOWS)
  _events[0] = CreateEvent(nullptr, false, false, "QueryCollectEvent");
  _events[1] = CreateEvent(nullptr, false, false, "ShutdownEvent");
#endif
  _collector_thread = std::thread{[this]() { this->run(); }};
}

xray::base::stats_thread::~stats_thread() noexcept { _collector_thread.join(); }

void xray::base::stats_thread::run() {

  _proc_stats.values.resize(counter_type::last);

#if defined(XRAY_OS_IS_WINDOWS)
  PdhOpenQuery(nullptr, 0, raw_handle_ptr(_proc_stats.query));
  if (!_proc_stats.query)
    return;

  _proc_stats.counters.resize(counter_type::last);

  static constexpr const char* const PERF_COUNTER_NAMES[] = {
    "% Processor Time",
    "Working Set",
    "Thread Count",
    "Virtual Bytes",
    "Virtual Bytes Peak",
    "Working Set Peak"};

  {
    stlsoft::auto_buffer<char, 512> mod_path{512};
    platformstl::system_traits<char>::get_module_filename(
      nullptr, mod_path.data(), mod_path.size());
    platformstl::path p{mod_path.data()};
    const auto        process_name = p.pop_ext().pop_sep().get_file();

    char counter_path_buff[1024];

    for (uint32_t i = 0; i < counter_type::last; ++i) {
      fmt::ArrayWriter awr{counter_path_buff};
      awr.write("\\Process({})\\{}", process_name, PERF_COUNTER_NAMES[i]);

      const auto result = PdhAddCounter(raw_handle(_proc_stats.query),
                                        awr.c_str(),
                                        0,
                                        &_proc_stats.counters[i]);

      if (result != ERROR_SUCCESS)
        XR_DBG_MSG("Failed to add counter {}", awr.c_str());
    }
  }

  PdhCollectQueryDataEx(raw_handle(_proc_stats.query),
                        1,
                        raw_handle(_events[thread_event::collect_query]));

  const HANDLE thr_events[] = {raw_handle(_events[thread_event::collect_query]),
                               raw_handle(_events[thread_event::shutdown])};

  _initialized = true;

  for (;;) {
    const auto signaled_event =
      WaitForMultipleObjectsEx(2, thr_events, false, INFINITE, false);

    if (signaled_event == WAIT_OBJECT_0) {
      //
      // collect query data
      for (uint32_t counter_idx = 0; counter_idx < counter_type::last;
           ++counter_idx) {
        DWORD                type{};
        PDH_FMT_COUNTERVALUE counter_val;
        const auto           result =
          PdhGetFormattedCounterValue(_proc_stats.counters[counter_idx],
                                      PDH_FMT_DOUBLE,
                                      &type,
                                      &counter_val);

        if (result != ERROR_SUCCESS) {
          continue;
        }

        _proc_stats.values[counter_idx] = counter_val.doubleValue;
      }

      continue;
    }

    break;
  }

  XR_DBG_MSG("Stats collect thread shutting down.");
#endif
}

xray::base::stats_thread::process_stats_info
xray::base::stats_thread::process_stats() const noexcept {
  process_stats_info psi;
  psi.cpu_usage = _proc_stats.values[counter_type::cpu_usage];
  psi.thread_count =
    static_cast<uint32_t>(_proc_stats.values[counter_type::thread_count]);
  psi.vbytes_peak =
    static_cast<uint32_t>(_proc_stats.values[counter_type::virtual_bytes_peak]);
  psi.virtual_bytes =
    static_cast<uint32_t>(_proc_stats.values[counter_type::virtual_bytes]);
  psi.working_set =
    static_cast<uint32_t>(_proc_stats.values[counter_type::working_set]);
  psi.work_set_peak =
    static_cast<uint32_t>(_proc_stats.values[counter_type::working_set_peak]);

  return psi;
}
