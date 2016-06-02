project(libconfig)

add_definitions(-DLIBCONFIG_STATIC)

set(SOURCES
    grammar.c
    libconfig.c
    scanctx.c
    scanner.c
    strbuf.c
    )

add_library(libconfig STATIC ${SOURCES})