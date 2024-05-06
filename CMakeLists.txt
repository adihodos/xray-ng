cmake_minimum_required(VERSION 3.20)
project(xray-ng VERSION 0.5.0.0 LANGUAGES C CXX)

# download CPM.cmake
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.38.3/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:oneapi-src/oneTBB@2021.12.0")
CPMAddPackage("gh:fmtlib/fmt#10.2.1")
CPMAddPackage("gh:TartanLlama/optional@1.1.0")
# CPMAddPackage("gh:tinyobjloader/tinyobjloader#release")
CPMAddPackage("gh:vimpunk/mio#master")
CPMAddPackage("gh:iboB/itlib#master")
# CPMAddPackage("gh:odygrd/quill@3.8.0")
CPMAddPackage("gh:getml/reflect-cpp#main")

CPMAddPackage(
  NAME pipes
  GITHUB_REPOSITORY adihodos/pipes
  GIT_TAG master
  OPTIONS "BUILD_TESTING OFF")

CPMAddPackage(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  GIT_TAG v1.14.1
  OPTIONS "SPDLOG_FMT_EXTERNAL ON"
  OPTIONS "SPDLOG_NO_EXCEPTIONS ON")

CPMAddPackage("gh:ryanhaining/cppitertools#master")

CPMAddPackage(
  NAME libconfig
  GITHUB_REPOSITORY adihodos/libconfig
  GIT_TAG master
  OPTIONS "BUILD_TESTING OFF"
  "BUILD_EXAMPLES OFF"
  "BUILD_SHARED_LIBS OFF"
  "BUILD_TESTS OFF")

# set(CMAKE_MODULE_PATH
#   "${CMAKE_PROJECT_DIR}/cmake"
#   "${CMAKE_PROJECT__DIR}/cmake/modules")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "Export compilation commands for code completion (clangd)")

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# if (MSVC)
#   option(XRAY_USE_DIRECTX11 "DirectX 11" OFF)
# endif()

# function(set_target_options target compiler_defs compiler_opts)
#   message("Setting compile options and definitions for target ${target}")
#
#   foreach (c IN LISTS compiler_defs)
# 	message("Compiler definition = ${c}")
# 	target_compile_features("${target}" PUBLIC ${compiler_dendforeach()
#
#   foreach (o IN LISTS compiler_opts)
# 	message("Compiler option = ${o}")
#   endforeach()
#
# endfunction()
#
# list(APPEND compiler_defs "XRAY_RENDERER_OPENGL" "XRAY_DOING_UR_MOM")
# list(APPEND compiler_opts "cxx_std_23" "c_std_11")
#
# set_target_options(${PROJECT_NAME} "${compiler_defs}" "${compiler_opts}")

# list(APPEND compiler_definitions )
# list(APPEND xr_compiler_features "cxx_std_20=PUBLIC" "c_std_11=PUBLIC")
#
# if (NOT XRAY_USE_DIRECTX11)
#   list(APPEND compiler_definitions "XRAY_RENDERER_OPENGL")
# else()
#   message("Using DirectX11")
#   list(APPEND compiler_definitions "XRAY_RENDERER_DIRECTX")
#   list(APPEND compiler_definitions "XRAY_USE_LEFT_HANDED")
# endif()
#
#
# if (WIN32)
#   # define NOMINMAX so that code that uses STL and windows.h compiles
#   list(APPEND compiler_definitions "NOMINMAX")
#
#   # Disabled retarded warnings about some functions being deprecated
#   list(APPEND compiler_definitions "_CRT_SECURE_NO_WARNINGS")
#   list(APPEND compiler_definitions "_CRT_NONSTDC_NO_WARNINGS")
#   list(APPEND compiler_definitions "_CRT_SECURE_NO_DEPRECATE")
#
#   # Compile for Windows 7 and later
#   list(APPEND compiler_definitions "_WIN32_WINNT=0x0601")
#   list(APPEND compiler_definitions "WINVER=0x0601")
#   list(APPEND compiler_definitions "DXGI_1_2_FORMATS")
#   list(APPEND compiler_definitions "_WIN7_PLATFORM_UPDATE")
#
# endif()
#
# if (UNIX)
#   list(APPEND SYSTEM_DEPS rt m dl pthread X11 Xrandr Xi Xcursor Xxf86vm GL Xinerama)
# endif()
#
# if (WIN32 AND NOT MSVC)
#   if (NOT XRAY_USE_DIRECTX11)
#     list(APPEND SYSTEM_DEPS opengl32)
#   endif()
#
#   if (NOT MSVC)
#     list(APPEND compiler_flags "mwindows")
#   endif()
# endif()
#
# if (NOT XRAY_USE_DIRECTX11 AND NOT XRAY_USE_DIRECTX10)
#   include_directories(third_party/opengl/include)
# endif()
#
add_subdirectory(third_party/opengl/src)
add_subdirectory(third_party/imgui/src)

# if (CMAKE_BUILD_TYPE MATCHES Debug)
#   message(" ---> Debug build <---")
#   list(APPEND compiler_definitions "XRAY_IS_DEBUG_BUILD")
# endif()
#
# #
# # common macros for all builds
# list(APPEND compiler_definitions "LIBCONFIG_STATIC")
# list(APPEND compiler_definitions "KTX_OPENGL=1")
#
# include_directories(include)
# include_directories(third_party/libconfig/include)
# include_directories(third_party/stb/include)
# include_directories(third_party/imgui/include)
# include_directories(third_party/easyloggingpp/include)
# include_directories(third_party/catch/include)
# include_directories(third_party/nuklear/include)
# include_directories(third_party/ktx/include)
#
# # include_directories(third_party/tinyobj/include)
# # include_directories(third_party/tinyobj/src)
# # add_subdirectory(third_party/tinyobj)
#
# add_subdirectory(third_party/libconfig/src)
# add_subdirectory(third_party/stb/src)
#
# include_directories(third_party/ktx/include)
# include_directories(third_party/ktx/other_include)
# add_subdirectory(third_party/ktx/lib)
#
# include_directories(third_party/libnoise/include)
# add_subdirectory(third_party/libnoise)
#
add_subdirectory(src)
