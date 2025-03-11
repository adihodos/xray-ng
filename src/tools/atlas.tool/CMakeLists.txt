cmake_minimum_required(VERSION 3.20)
project(
  atlas-tool
  VERSION 0.1.0.0
  LANGUAGES C CXX)

# add_library(xray-set-compile-options-lib INTERFACE)

set(proj_inc_dir "${PROJECT_SOURCE_DIR}/src")
set(proj_src_dir "${PROJECT_SOURCE_DIR}/src")

set(project_sources ${proj_src_dir}/atlass.tool.main.cc)
add_executable(${PROJECT_NAME} WIN32 ${project_sources})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(
  ${PROJECT_NAME}
  xray-set-compile-options-lib
  cpp-lazy::cpp-lazy
  reflectcpp
  fmt::fmt
  ktx
  mio::mio
  stb::stb
  xray-base
  xray-rendering
  bfg::lyra)

# if (MSVC) option(XRAY_USE_DIRECTX11 "DirectX 11" OFF) endif()

# function(set_target_options target compiler_defs compiler_opts)
# message("Setting compile options and definitions for target ${target}")
#
# foreach (c IN LISTS compiler_defs) message("Compiler definition = ${c}")
# target_compile_features("${target}" PUBLIC ${compiler_dendforeach()
#
# foreach (o IN LISTS compiler_opts) message("Compiler option = ${o}")
# endforeach()
#
# endfunction()
#
# list(APPEND compiler_defs "XRAY_RENDERER_OPENGL" "XRAY_DOING_UR_MOM")
# list(APPEND compiler_opts "cxx_std_23" "c_std_11")
#
# set_target_options(${PROJECT_NAME} "${compiler_defs}" "${compiler_opts}")

# list(APPEND compiler_definitions ) list(APPEND xr_compiler_features
# "cxx_std_20=PUBLIC" "c_std_11=PUBLIC")
#
# if (NOT XRAY_USE_DIRECTX11) list(APPEND compiler_definitions
# "XRAY_RENDERER_OPENGL") else() message("Using DirectX11") list(APPEND
# compiler_definitions "XRAY_RENDERER_DIRECTX") list(APPEND compiler_definitions
# "XRAY_USE_LEFT_HANDED") endif()
#
# if (WIN32) # define NOMINMAX so that code that uses STL and windows.h compiles
# list(APPEND compiler_definitions "NOMINMAX")
#
# # Disabled retarded warnings about some functions being deprecated list(APPEND
# compiler_definitions "_CRT_SECURE_NO_WARNINGS") list(APPEND
# compiler_definitions "_CRT_NONSTDC_NO_WARNINGS") list(APPEND
# compiler_definitions "_CRT_SECURE_NO_DEPRECATE")
#
# # Compile for Windows 7 and later list(APPEND compiler_definitions
# "_WIN32_WINNT=0x0601") list(APPEND compiler_definitions "WINVER=0x0601")
# list(APPEND compiler_definitions "DXGI_1_2_FORMATS") list(APPEND
# compiler_definitions "_WIN7_PLATFORM_UPDATE")
#
# endif()
#
# if (NOT XRAY_USE_DIRECTX11 AND NOT XRAY_USE_DIRECTX10)
# include_directories(third_party/opengl/include) endif()
#

# if (CMAKE_BUILD_TYPE MATCHES Debug) message(" ---> Debug build <---")
# list(APPEND compiler_definitions "XRAY_IS_DEBUG_BUILD") endif()
#
# # # common macros for all builds list(APPEND compiler_definitions
# "LIBCONFIG_STATIC") list(APPEND compiler_definitions "KTX_OPENGL=1")
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
# # include_directories(third_party/tinyobj/include) #
# include_directories(third_party/tinyobj/src) #
# add_subdirectory(third_party/tinyobj)
#
# add_subdirectory(third_party/libconfig/src)
# add_subdirectory(third_party/stb/src)
#
# include_directories(third_party/ktx/include)
# include_directories(third_party/ktx/other_include)
# add_subdirectory(third_party/ktx/lib)
#
# include_directories(third_party/libnoise/include)
#
