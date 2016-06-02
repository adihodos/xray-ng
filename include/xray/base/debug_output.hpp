#ifndef xray_base_debug_output_hpp__
#define xray_base_debug_output_hpp__

#include <cstdint>
#include "xray/xray.hpp"

namespace xray {
namespace base {    

void output_debug_string(const char* format, ...);

}    
}

#define OUTPUT_DBG_MSG_NO_FILE_LINE(fmt_string, ...)    \
    do {                                                \
        xray::base::output_debug_string(                  \
            fmt_string, ##__VA_ARGS__);                 \
    } while (0)

#define OUTPUT_DBG_MSG(fmt_string, ...)     \
    do {                                    \
        xray::base::output_debug_string(      \
            "\nFile [%s], line [%d] : \n",  \
            __FILE__, __LINE__);            \
        xray::base::output_debug_string(      \
            fmt_string, ##__VA_ARGS__); \
    } while (0)

#define OUTPUT_DBG_MSG_WINAPI_FAIL(function_name)   \
    do {                                            \
        xray::base::output_debug_string(              \
            "File [%s], line [%d] : \n",            \
            __FILE__, __LINE__);                    \
        xray::base::output_debug_string(              \
            "Function [%s] failed, error code %d",  \
            #function_name, GetLastError());        \
    } while (0)

#endif /* !defined(xray_base_debug_output_hpp__) */