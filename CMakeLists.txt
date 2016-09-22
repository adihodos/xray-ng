cmake_minimum_required(VERSION 3.6)
project(xray-ng VERSION 0.5.0.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )
# Second, for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

if (MSVC)
  option(XRAY_USE_DIRECTX11 "DirectX 11" ON)
endif()

include(CheckCXXSymbolExists)
if (WIN32 AND NOT MSVC)
  CHECK_CXX_SYMBOL_EXISTS(_GLIBCXX_HAS_GTHREADS thread HAVE_CPP_THREADING_SUPPORT)
  if (NOT HAVE_CPP_THREADING_SUPPORT)
    message("MinGW std::thread workaround enabled!")
    add_definitions(-DXRAY_MINGW_STD_THREAD_WORKAROUND)
    include_directories(third_party/mingw-threading)
  endif()
endif()

if (WIN32)

  if (XRAY_USE_DIRECTX11 OR XRAY_USE_DIRECTX10)
    message("DirectX 10/11 -:)")
    add_definitions(-DXRAY_RENDERER_DIRECTX)
    add_definitions(-DXRAY_USE_LEFT_HANDED)
  endif()

  # define NOMINMAX so that code that uses STL and windows.h compiles
  add_definitions(-DNOMINMAX)

  # Disabled retarded warnings about some functions being deprecated
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)

  # Compile for Windows 7 and later
  add_definitions(-D_WIN32_WINNT=0x0601)
  add_definitions(-DWINVER=0x0601)
  add_definitions(-DDXGI_1_2_FORMATS)
  add_definitions(-D_WIN7_PLATFORM_UPDATE)

endif()

if (NOT MSVC)
    message("Clang/MinGW/GCC")
    set(cxx_disabled_warnings "-Wno-unused-local-typedefs")
    set(cxx_warn_level "-Wall -Wextra")
    set(cxx_standard "-std=c++14")
    set(cxx_debug_flags "-O0 -ggdb3")
    add_definitions(-DXRAY_RENDERER_OPENGL)

else()
    message("MSVC")
    set(cxx_warn_level "/W4")
    set(cxx_disabled_warnings "/wd4201 /wd4127 /wd4005 /wd4996 /wd4268")
    add_definitions(/D__TBB_NO_IMPLICIT_LINKAGE)

    if (CMAKE_BUILD_TYPE MATCHES Debug)
        add_definitions(/D_ITERATOR_DEBUG_LEVEL=0)
        # set(cxx_aditional_flags "/MT /Od /Zi")
        set(cxx_aditional_flags "/Od /Zi")
    else()
        #add_definitions(/DXRAY_IS_DEBUG_BUILD)
        add_definitions(/D_ITERATOR_DEBUG_LEVEL=0)
        # set(cxx_aditional_flags "/MT /O2 /Zi /Zc:inline")
        set(cxx_aditional_flags "/O2 /Zi /Zc:inline")
    endif()

    foreach(flags_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINF)
        # string(REPLACE "/MD" "/MT" "${flags_var}" "${${flags_var}}")
        # string(REPLACE "/MDd" "/MTd" "${flags_var}" "${${flags_var}}")
        string(REPLACE "/W3" "/W4" "${flags_var}" "${${flags_var}}")
    endforeach()

endif()

if (UNIX)
  #set(SYS_DEP_LIBS rt m dl pthread X11 Xi Xrandr Xcursor Xinerama Xxf86vm GL)
  set(SYS_DEP_LIBS rt m dl pthread X11 Xrandr Xi Xcursor Xxf86vm GL Xinerama)
endif()

if (WIN32 AND NOT XRAY_USE_DIRECTX11 AND NOT XRAY_USE_DIRECTX10)
  set(SYS_DEP_LIBS opengl32)
  add_definitions(-mwindows)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cxx_standard} ${cxx_warn_level} ${cxx_disabled_warnings} ${cxx_debug_flags} ${cxx_aditional_flags}")
message("Compiler flags = ${CMAKE_CXX_FLAGS}")

#
# required libraries
find_package(TBB REQUIRED)
include_directories(${TBB_INCLUDE_DIR})

if (NOT XRAY_USE_DIRECTX11 AND NOT XRAY_USE_DIRECTX10)
  find_package(GLFW REQUIRED)
  include_directories(${GLFW_INCLUDE_DIR})
  include_directories(third_party/opengl/include)
  add_subdirectory(third_party/opengl/src)
endif()

find_package(ASSIMP REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIR})

find_package(FMT REQUIRED)
include_directories(${FMT_INCLUDE_DIR})

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(" ---> Debug build <---")
    add_definitions(-DXRAY_IS_DEBUG_BUILD)
endif()

#
# common macros for all builds
add_definitions(-DLIBCONFIG_STATIC)

include_directories(include)
include_directories(third_party/gsl/include)
include_directories(third_party/libconfig/include)
include_directories(third_party/stlsoft-1.9.124/include)
include_directories(third_party/stb/include)
include_directories(third_party/imgui/include)
include_directories(third_party/easyloggingpp/include)

add_subdirectory(third_party/libconfig/src)
add_subdirectory(third_party/stb/src)
add_subdirectory(third_party/imgui/src)
add_subdirectory(src)
