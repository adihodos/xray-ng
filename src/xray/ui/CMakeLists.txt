project(xray-ui)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/ui")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

# if (WIN32)
#     set(platform_sources
#       ${proj_inc_dir}/window_win32.hpp
#       ${proj_src_dir}/window_win32.cc)
# else()
#     set(platform_sources
#         ${proj_inc_dir}/window_x11.hpp
#         ${proj_src_dir}/window_x11.cc)
# endif()

set(PROJECT_SOURCES_MAIN
    ${proj_inc_dir}/events.hpp
    ${proj_inc_dir}/key_sym.hpp
    ${proj_src_dir}/key_sym.cc
    ${proj_inc_dir}/user_interface.hpp
    ${proj_src_dir}/user_interface.cc
#    ${proj_inc_dir}/ui.hpp
#    ${proj_src_dir}/ui.cc
    ${platform_sources})

#source_group("xray_ui" FILES ${PROJECT_SOURCES_MAIN})

# if (WIN32) 
#     if (XRAY_USE_DIRECTX11)
#         set(platform_libs dwmapi)
#     else()
#         set(platform_libs gdi32 user32 kernel32)
#     endif()
# else()
#     set(platform_libs X11 GL)
# endif()

add_library(
  ${PROJECT_NAME} STATIC
  ${PROJECT_SOURCES_MAIN}
  $<$<PLATFORM_ID:Linux>:${proj_inc_dir}/window_x11.hpp>
  $<$<PLATFORM_ID:Linux>:${proj_src_dir}/window_x11.cc>
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PRIVATE
)

target_compile_features(
  ${PROJECT_NAME}
  PUBLIC cxx_std_20
)

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE
  $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>
  $<$<PLATFORM_ID:Linux>:XRAY_RENDERER_OPENGL>
)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
  xray-base
  xray-rendering
  xray-imgui
  $<$<PLATFORM_ID:Linux>:X11 GL>
)

