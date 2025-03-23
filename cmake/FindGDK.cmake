include(FindPackageHandleStandardArgs)

find_path(
  GDK_INCLUDE_DIR 
  NAMES grdk.h 
  PATHS 
    ENV GRDKLatest 
  PATH_SUFFIXES GameKit/Include
  REQUIRED)

find_library(
  GDK_GAMEINPUT_LIBRARY 
  NAMES GameInput
  PATHS
    ENV GRDKLatest
  PATH_SUFFIXES
    GameKit/Lib/amd64
  REQUIRED)

find_library(
  GDK_XGAMERUNTIME_LIBRARY 
  NAMES xgameruntime
  PATHS
    ENV GRDKLatest
  PATH_SUFFIXES
    GameKit/Lib/amd64
  REQUIRED)

find_package_handle_standard_args(GDK REQUIRED_VARS GDK_INCLUDE_DIR GDK_GAMEINPUT_LIBRARY GDK_XGAMERUNTIME_LIBRARY)

if (GDK_INCLUDE_DIR)
  mark_as_advanced(GDK_INCLUDE_DIR)
  mark_as_advanced(GDK_GAMEINPUT_LIBRARY)
  mark_as_advanced(GDK_XGAMERUNTIME_LIBRARY)
endif()

if (GDK_GAMEINPUT_LIBRARY AND NOT TARGET GDK::GameInput)
  add_library(GDK::GameInput SHARED IMPORTED)
  set_property(TARGET GDK::GameInput PROPERTY IMPORTED_LOCATION ${GDK_GAMEINPUT_LIBRARY})
endif()

if (GDK_XGAMERUNTIME_LIBRARY AND NOT TARGET GDK::GameRuntime)
  add_library(GDK::GameRuntime SHARED IMPORTED)
  set_property(TARGET GDK::GameRuntime PROPERTY IMPORTED_LOCATION ${GDK_XGAMERUNTIME_LIBRARY})
endif()

if (GDK_INCLUDE_DIR AND GDK_GAMEINPUT_LIBRARY AND GDK_XGAMERUNTIME_LIBRARY)
  add_library(GDKLibrary INTERFACE)
  target_include_directories(GDKLibrary INTERFACE ${GDK_INCLUDE_DIR})
  target_link_libraries(GDKLibrary INTERFACE ${GDK_GAMEINPUT_LIBRARY} ${GDK_XGAMERUNTIME_LIBRARY})
  add_library(GDK::GDK ALIAS GDKLibrary)
endif()
