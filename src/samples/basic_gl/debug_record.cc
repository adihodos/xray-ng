#include "debug_record.hpp"
#include <cassert>

static constexpr uint32_t MAX_DEBUG_COUNTERS = 256;
app::detail::debug_record DEBUG_RECORDS_STORAGE[MAX_DEBUG_COUNTERS];

void
app::detail::DEBUGRecord_enter_scope(const int32_t id,
                                     const char* file,
                                     const char* func,
                                     const int32_t line,
                                     uint32_t hitcnt)
{
    assert(id < static_cast<int32_t>(MAX_DEBUG_COUNTERS));
    DEBUG_RECORDS_STORAGE[id].hit_count += hitcnt;
    DEBUG_RECORDS_STORAGE[id].file = file;
    DEBUG_RECORDS_STORAGE[id].func = func;
    DEBUG_RECORDS_STORAGE[id].line = line;
    DEBUG_RECORDS_STORAGE[id].millis = 0.0f;
}

void
app::detail::DEBUGRecord_exit_scope(const int32_t id, const float msecs)
{
    assert(id < static_cast<int32_t>(MAX_DEBUG_COUNTERS));
    DEBUG_RECORDS_STORAGE[id].millis = msecs;
}

const app::detail::debug_record*
app::detail::DEBUGRecord_records_begin()
{
    return DEBUG_RECORDS_STORAGE;
}

const app::detail::debug_record*
app::detail::DEBUGRecord_records_end()
{
    return &DEBUG_RECORDS_STORAGE[MAX_DEBUG_COUNTERS];
}