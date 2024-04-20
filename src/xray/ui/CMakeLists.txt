project(xray-ui)

set(proj_inc_dir ${CMAKE_SOURCE_DIR}/include/xray/ui)
set(proj_src_dir ${CMAKE_SOURCE_DIR}/src/xray/ui)

if (WIN32)
    if (XRAY_USE_DIRECTX11)
        set(platform_sources
            ${proj_inc_dir}/app_window.hpp
            ${proj_src_dir}/app_window.cc
            ${proj_src_dir}/key_symbols_win.cc)
    else()
        set(platform_sources
            ${proj_inc_dir}/window_win32.hpp
            ${proj_src_dir}/window_win32.cc)
    endif()
else()
    set(platform_sources
        ${proj_inc_dir}/window_x11.hpp
        ${proj_src_dir}/window_x11.cc)
endif()

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

if (WIN32) 
    if (XRAY_USE_DIRECTX11)
        set(platform_libs dwmapi)
    else()
        set(platform_libs gdi32 user32 kernel32)
    endif()
else()
    set(platform_libs X11 GL)
endif()

add_library(xray-ui STATIC ${PROJECT_SOURCES_MAIN})
target_link_libraries(xray-ui xray-imgui STLSoft ${platform_libs})

