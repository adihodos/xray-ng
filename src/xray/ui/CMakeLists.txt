project(xray-ui)

set(proj_inc_dir ${CMAKE_SOURCE_DIR}/include/xray/ui)
set(proj_src_dir ${CMAKE_SOURCE_DIR}/src/xray/ui)

if (WIN32 AND XRAY_USE_DIRECTX11)
    set(platform_sources
        ${proj_inc_dir}/app_window.hpp
        ${proj_src_dir}/app_window.cc
        ${proj_src_dir}/key_symbols_win.cc)
else()
    set(platform_sources
        ${proj_inc_dir}/basic_gl_window.hpp
        ${proj_src_dir}/basic_gl_window.cc
        ${proj_inc_dir}/window_x11.hpp
        ${proj_src_dir}/window_x11.cc        
        ${proj_src_dir}/key_symbols_x11.cc
        ${proj_src_dir}/key_sym.cc
        )
endif()

set(PROJECT_SOURCES_MAIN
    ${proj_inc_dir}/user_interface.hpp
    ${proj_src_dir}/user_interface.cc
    ${proj_inc_dir}/input_event.hpp
    ${proj_inc_dir}/fwd_input_events.hpp
    ${proj_inc_dir}/key_symbols.hpp
    ${proj_inc_dir}/key_sym.hpp
    ${platform_sources})

#source_group("xray_ui" FILES ${PROJECT_SOURCES_MAIN})

if (WIN32 AND XRAY_USE_DIRECTX11)
    set(platform_libs dwmapi)
else()
    set(platform_libs ${GLFW_LIBRARY})
endif()

add_library(xray-ui STATIC ${PROJECT_SOURCES_MAIN})
target_link_libraries(xray-ui xray-imgui ${platform_libs})

# install(TARGETS XrayUI DESTINATION Lib)
