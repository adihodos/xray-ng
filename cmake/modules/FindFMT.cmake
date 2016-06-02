
find_path(FMT_INCLUDE_DIR fmt/format.h
          PATHS ${FMT_ROOT} $ENV{FMT_ROOT} /usr/include /usr/local/include
          PATH_SUFFIXES include)

message("FMT include directory -> ${FMT_INCLUDE_DIR}")

find_library(
  FMT_LIBRARY
  NAMES fmt
  HINTS ${FMT_ROOT} $ENV{FMT_ROOT} /usr/lib /usr/local/lib
  PATH_SUFFIXES lib)

message("FMT library -> ${FMT_LIBRARY}")

if ((FMT_LIBRARY STREQUAL "FMT_LIBRARY-NOTFOUND") OR
    (FMT_INCLUDE_DIR STREQUAL "FMT_INCLUDE_DIR-NOTFOUND"))
    SET(FMT_FOUND FALSE)
else()
    SET(FMT_FOUND TRUE)
    message("FMT library found!")
endif()

if (NOT FMT_FOUND)
  if (FMT_FIND_REQUIRED)
    message(FATAL_ERROR "FMT library not found!")
  endif()
endif()
