project(xray-imgui)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/third_party/imgui/include")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/third_party/imgui/src")
include_directories(${proj_src_dir})

set(project_sources
    ${proj_inc_dir}/imgui/imconfig.h
    ${proj_inc_dir}/imgui/imgui_internal.h
    ${proj_src_dir}/imgui.cpp
    ${proj_src_dir}/imgui_draw.cpp
    ${proj_src_dir}/stb_rect_pack.h
    ${proj_src_dir}/stb_textedit.h
    ${proj_src_dir}/stb_truetype.h
    ${proj_inc_dir}/imgui/imgui.h
    )

if (WIN32 AND NOT MSVC)
    set(platform_libs imm32)
endif()    

add_library(xray-imgui STATIC ${project_sources})
target_link_libraries(xray-imgui xray-opengl-renderer ${platform_libs})
