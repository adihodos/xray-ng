cmake_minimum_required(VERSION 3.20)
project(
  xray-ng
  VERSION 0.5.0.0
  LANGUAGES C CXX)

# cmake_policy(SET CMP0091 NEW)
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(CTest)
enable_testing()

include(FetchContent)
include(cmake/CPM.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

cpmaddpackage("gh:adihodos/concurrencpp#fix-compile-error")
cpmaddpackage("gh:fmtlib/fmt#10.2.1")
cpmaddpackage("gh:TartanLlama/optional@1.1.0")
cpmaddpackage("gh:TartanLlama/expected@1.1.0")
cpmaddpackage("gh:vimpunk/mio#master")
cpmaddpackage("gh:iboB/itlib#master")
cpmaddpackage("gh:getml/reflect-cpp#main")
cpmaddpackage("gh:max0x7ba/atomic_queue#master")
cpmaddpackage("gh:serge-sans-paille/frozen#master")
cpmaddpackage("gh:bfgroup/Lyra#develop")

cpmaddpackage(
  NAME
  pipes
  GITHUB_REPOSITORY
  adihodos/pipes
  GIT_TAG
  master
  OPTIONS
  "BUILD_TESTING OFF")

cpmaddpackage(
  NAME
  JoltPhysics
  GITHUB_REPOSITORY
  jrouwe/JoltPhysics
  GIT_TAG
  # v5.2.0
  master
  DOWNLOAD_ONLY
  True
  OPTIONS
  "DOUBLE_PRECISION OFF"
  "GENERATE_DEBUG_SYMBOLS ON"
  "OVERRIDE_CXX_FLAGS ON"
  "CROSS_PLATFORM_DETERMINISTIC OFF"
  "INTERPROCEDURAL_OPTIMIZATION OFF"
  "FLOATING_POINT_EXCEPTIONS_ENABLED OFF"
  "CPP_EXCEPTIONS_ENABLED OFF"
  "CPP_RTTI_ENABLED OFF"
  "OBJECT_LAYER_BITS 16")

if(JoltPhysics_ADDED)
  set(DOUBLE_PRECISION "OFF")
  set(GENERATE_DEBUG_SYMBOLS "ON")
  set(OVERRIDE_CXX_FLAGS "OFF")
  set(CROSS_PLATFORM_DETERMINISTIC "OFF")
  set(INTERPROCEDURAL_OPTIMIZATION "OFF")
  set(FLOATING_POINT_EXCEPTIONS_ENABLED "OFF")
  set(CPP_EXCEPTIONS_ENABLED "OFF")
  set(CPP_RTTI_ENABLED "OFF")
  set(OBJECT_LAYER_BITS "16")
  set(CPP_RTTI_ENABLED "ON")
  set(USE_STATIC_MSVC_RUNTIME_LIBRARY "OFF")
  add_subdirectory(${JoltPhysics_SOURCE_DIR}/Build
                   ${JoltPhysics_SOURCE_DIR}/Build/out)
endif()

cpmaddpackage(
  NAME
  cpp-lazy
  GITHUB_REPOSITORY
  Kaaserne/cpp-lazy
  GIT_TAG
  master
  OPTIONS
  "CPP-LAZY_USE_STANDALONE ON")

cpmaddpackage(
  NAME
  spdlog
  GITHUB_REPOSITORY
  gabime/spdlog
  GIT_TAG
  v1.14.1
  OPTIONS
  "SPDLOG_FMT_EXTERNAL ON"
  OPTIONS
  "SPDLOG_NO_EXCEPTIONS ON")

cpmaddpackage("gh:ryanhaining/cppitertools#master")
cpmaddpackage("gh:rollbear/strong_type@15")
# cpmaddpackage("gh:wolfpld/tracy#master")

cpmaddpackage(
  NAME
  tracy-profiler
  GITHUB_REPOSITORY
  wolfpld/tracy
  GIT_TAG
  master
  OPTIONS
  "TRACY_ENABLE ON")

cpmaddpackage(
  NAME
  libconfig
  GITHUB_REPOSITORY
  # adihodos/libconfig
  hyperrealm/libconfig
  GIT_TAG
  master
  OPTIONS
  "BUILD_TESTING OFF"
  "BUILD_EXAMPLES OFF"
  "BUILD_SHARED_LIBS OFF"
  "BUILD_TESTS OFF")

cpmaddpackage("gh:adihodos/swl-variant#main")
cpmaddpackage(
  NAME
  spirv-reflect
  GITHUB_REPOSITORY
  KhronosGroup/SPIRV-Reflect
  GIT_TAG
  vulkan-sdk-1.3.280.0
  OPTIONS
  "SPIRV_REFLECT_STATIC_LIB ON"
  "SPIRV_REFLECT_EXECUTABLE OFF")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")

set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "Export compilation commands for code completion (clangd)")

set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/.git/index")

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# https://dhashe.com/how-to-build-highly-debuggable-c-binaries.html
add_library(xray-set-compile-options-lib INTERFACE)

set(cxx_is_gcc "$<COMPILE_LANG_AND_ID:CXX,GNU>")
set(cxx_is_clang "$<COMPILE_LANG_AND_ID:CXX,Clang>")
set(cxx_is_gcc_like "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU>")

if(TARGET Linux)
  target_compile_options(
    xray-set-compile-options-lib
    INTERFACE
      $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-fasynchronous-unwind-tables;-fsanitize=address>
      $<$<BOOL:${cxx_is_gcc_like}>:-ggdb3;-fno-omit-frame-pointer;-mno-omit-leaf-frame-pointer>
  )

  target_link_options(
    xray-set-compile-options-lib PUBLIC
    $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>-fsanitize=address>)

  target_compile_definitions(xray-set-compile-options-lib
                             INTERFACE "${cxx_stdlib_dbg_flags}")
endif()

target_compile_options(
  xray-set-compile-options-lib
  INTERFACE
  "$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8;/execution-charset:utf-8>")

target_compile_definitions(
  xray-set-compile-options-lib INTERFACE $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD> 
  $<$<PLATFORM_ID:Windows>:YY_NO_UNISTD_H>
  XRAY_MATH_ENABLE_FMT_SUPPORT
  "$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS;_CRT_NONSTDC_NO_WARNINGS;_CRT_SECURE_NO_DEPRECATE>")

add_subdirectory(third_party/stb)
add_subdirectory(third_party/opengl/src)
add_subdirectory(third_party/imgui-docking/src)
add_subdirectory(third_party/libnoise)
add_subdirectory(src)
