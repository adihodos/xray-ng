project(xray-ui)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/ui")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
  find_package(X11 REQUIRED COMPONENTS xcb Xi Xinerama Xkb xkbcommon)
  set(platform_sources ${proj_inc_dir}/window_x11.hpp
                       ${proj_src_dir}/window_x11.cc)
endif()

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
  set(platform_sources ${proj_inc_dir}/window_win32.hpp
                       ${proj_src_dir}/window_win32.cc)
endif()

find_package(OpenGL REQUIRED)

set(PROJECT_SOURCES_MAIN
    ${proj_inc_dir}/events.hpp
    ${proj_inc_dir}/key_sym.hpp
    ${proj_inc_dir}/user_interface.hpp
    ${proj_src_dir}/user_interface.cc
    ${proj_inc_dir}/user_interface_backend_opengl.hpp
    ${proj_src_dir}/user_interface_backend_opengl.cc
    ${proj_inc_dir}/user_interface_render_context.hpp
    # ${proj_inc_dir}/ui.hpp ${proj_src_dir}/ui.cc
    ${platform_sources})

# source_group("xray_ui" FILES ${PROJECT_SOURCES_MAIN})

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES_MAIN})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PRIVATE)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_compile_definitions(
  ${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>
                          $<$<PLATFORM_ID:Linux>:XRAY_RENDERER_OPENGL>)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
    xray-base
    xray-rendering
    xray-imgui-docking
    $<$<PLATFORM_ID:Linux>:OpenGL::OpenGL;OpenGL::GLX;X11::Xi;X11::Xinerama;X11::xcb;X11::X11_xcb;X11::Xkb;X11::xkbcommon;X11::xkbcommon_X11>
)
