#pragma once

#include "xray/xray.hpp"
#include <chrono>
#include <cstdint>

namespace app {
namespace detail {

struct debug_record {
  uint32_t    hit_count;
  const char* file;
  const char* func;
  int32_t     line;
  float       millis;
};

void DEBUGRecord_enter_scope(const int32_t id, const char* file,
                             const char* func, const int32_t line,
                             uint32_t hitcnt);

void DEBUGRecord_exit_scope(const int32_t id, const float msecs);

const debug_record* DEBUGRecord_records_begin();
const debug_record* DEBUGRecord_records_end();

} // namespace detail

struct scoped_time_block {
private:
  int32_t                                                     _id;
  std::chrono::time_point<std::chrono::high_resolution_clock> _ts_entry;

public:
  scoped_time_block(const int32_t id, const char* file, const char* func,
                    const int32_t line, uint32_t hitcnt = 1)
      : _id{id} {
    detail::DEBUGRecord_enter_scope(id, file, func, line, hitcnt);
    _ts_entry = std::chrono::high_resolution_clock::now();
  }

  ~scoped_time_block() {
    auto ts_exit = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> interval = ts_exit - _ts_entry;
    detail::DEBUGRecord_exit_scope(_id, interval.count());
  }

private:
  XRAY_NO_COPY(scoped_time_block);
};

} // namespace app

#define SCOPED_TIME_BLOCK(...)                                                 \
  app::scoped_time_block XRAY_ANONYMOUS_VARIABLE(timedblockvar) {              \
    __COUNTER__, __FILE__, XRAY_QUALIFIED_FUNCTION_NAME, __LINE__              \
  }

#define SCOPED_NAMED_TIME_BLOCK(block_name)                                    \
  app::scoped_time_block XRAY_ANONYMOUS_VARIABLE(timedblockvar) {              \
    __COUNTER__, __FILE__, block_name, __LINE__                                \
  }
