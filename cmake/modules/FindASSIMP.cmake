# include(FindPkgMacros)

# findpkg_begin(Assimp)

# getenv_path(ASSIMP_ROOT)

# set(Assimp_PREFIX_PATH
#     "${ASSIMP_SDKDIR}" "${ENV_ASSIMP_SDKDIR}"
# )

# create_search_paths(Assimp)

# find_library(Assimp_LIBRARIES NAME assimp.lib HINTS ${Assimp_LIB_SEARCH_PATH})
# find_file(Assimp_DLL NAME assimp.dll HINTS ${Assimp_LIB_SEARCH_PATH})

# if (Assimp_LIBRARIES)
#     set(Assimp_FOUND true)
# endif()

# findpkg_finish(Assimp)

# This script locates the libconfig library
# ------------------------------------
#
# Usage
# -----
#
#
# This script defines the following variables:
# - LIBCONFIG_LIBRARY:   the list of all libraries corresponding to the required modules
# - LIBCONFIG_FOUND:     true if libconfig was found
# - LIBCONFIG_INCLUDE_DIR: the path where libconfig headers are located
#


# find the Assimp include directory
find_path(ASSIMP_INCLUDE_DIR assimp/version.h
          PATHS ${ASSIMP_ROOT} $ENV{ASSIMP_ROOT} /usr/include /usr/local/include
          PATH_SUFFIXES include)

message("ASSIMP include directory -> ${ASSIMP_INCLUDE_DIR}")

#if (CMAKE_BUILD_TYPE MATCHES Debug)
  #set(assimp_library_name "assimpd")
#else()
#endif()

find_library(
  ASSIMP_LIBRARY
  NAMES assimp assimp-vc130-mt assimp-vc140-mt
  HINTS ${ASSIMP_ROOT} $ENV{ASSIMP_ROOT} /usr/lib /usr/local/lib
  PATH_SUFFIXES lib)

find_file(
  ASSIMP_BIN_LIBRARY
  NAMES libassimp.dll libassimp.so assimp-vc130-mt.dll assimp-vc140-mt.dll
  HINTS ${ASSIMP_ROOT} $ENV{ASSIMP_ROOT} /usr/lib /usr/local/lib
  PATH_SUFFIXES bin lib)

message("ASSIMP library -> ${ASSIMP_LIBRARY}")
message("ASSIMP link library -> ${ASSIMP_LIBRARY}")
message("ASSIMP binary library -> ${ASSIMP_BIN_LIBRARY}")

if ((ASSIMP_LIBRARY STREQUAL "ASSIMP_LIBRARY-NOTFOUND") OR
    (ASSIMP_INCLUDE_DIR STREQUAL "ASSIMP_INCLUDE_DIR-NOTFOUND"))
    SET(ASSIMP_FOUND FALSE)
else()
    SET(ASSIMP_FOUND TRUE)
    message("ASSIMP library found!")
endif()

if (NOT ASSIMP_FOUND)
  if (ASSIMP_FIND_REQUIRED)
    message(FATAL_ERROR "Assimp library not found!")
  endif()
endif()
