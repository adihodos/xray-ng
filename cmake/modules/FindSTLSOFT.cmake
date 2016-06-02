# Module for locating Intel's Threading Building Blocks (TBB).
#
# Customizable variables:
#   TBB_ROOT_DIR
#     Specifies TBB's root directory.
#
# Read-only variables:
#   TBB_FOUND
#     Indicates whether the library has been found.
#
#   TBB_INCLUDE_DIRS
#      Specifies TBB's include directory.
#
#   TBB_LIBRARIES
#     Specifies TBB libraries that should be passed to target_link_libararies.
#

INCLUDE (FindPackageHandleStandardArgs)

FIND_PATH (STLSOFT_ROOT_DIR
  NAMES include/stlsoft/stlsoft.h
  PATHS ENV STLSOFT_ROOT
  DOC "STLSOFT root directory")

message("STLSOFT_ROOT_DIR -> ${STLSOFT_ROOT_DIR}")  

FIND_PATH (STLSOFT_INCLUDE_DIR
  NAMES stlsoft/stlsoft.h
  HINTS ${STLSOFT_ROOT_DIR}
  PATH_SUFFIXES include
  DOC "STLSOFT include directory")

message("STLSOFT_INCLUDE_DIR -> ${STLSOFT_INCLUDE_DIR}")    

FIND_PACKAGE_HANDLE_STANDARD_ARGS (STLSOFT REQUIRED_VARS STLSOFT_ROOT_DIR
  STLSOFT_INCLUDE_DIR)
