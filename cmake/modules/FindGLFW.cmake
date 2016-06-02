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

FIND_PATH (GLFW_ROOT_DIR
  NAMES include/GLFW/glfw3.h
  PATHS ENV GLFW_ROOT
  DOC "GLFW root directory")

message("GLFW_ROOT_DIR -> ${GLFW_ROOT_DIR}")  

FIND_PATH (GLFW_INCLUDE_DIR
  NAMES GLFW/glfw3.h
  HINTS ${GLFW_ROOT_DIR}
  PATH_SUFFIXES include
  DOC "GLFW include directory")

message("GLFW_INCLUDE_DIR -> ${GLFW_INCLUDE_DIR}")    

FIND_LIBRARY (GLFW_LIBRARY
  NAMES glfw3
  HINTS ${GLFW_ROOT_DIR}
  PATH_SUFFIXES lib
  DOC "GLFW library")

FIND_PACKAGE_HANDLE_STANDARD_ARGS (GLFW REQUIRED_VARS GLFW_ROOT_DIR
  GLFW_INCLUDE_DIR GLFW_LIBRARY)
