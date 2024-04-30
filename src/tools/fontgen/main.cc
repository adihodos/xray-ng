#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/xray.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ft2build.h>
#include FT_FREETYPE_H

int
main(int, char**)
{
    FT_Library library;
    const auto err = FT_Init_FreeType(&library);

    if (err) {
        OUTPUT_DBG_MSG("Failed to init freetype !");
        return EXIT_FAILURE;
    }

    return 0;
}