project(xray-glloader)

set(proj_inc_dir ${CMAKE_SOURCE_DIR}/third_party/opengl/include)
set(proj_src_dir ${CMAKE_SOURCE_DIR}/third_party/opengl/src)

set(PROJECT_SOURCES_MAIN
    ${proj_inc_dir}/opengl/opengl.hpp
    ${proj_src_dir}/opengl.cpp)

if (WIN32)
    set(platform_libs opengl32)
elseif(UNIX)
    set(platform_libs GL)    
endif()

#source_group("xray_ui" FILES ${PROJECT_SOURCES_MAIN})

add_library(xray-glloader STATIC ${PROJECT_SOURCES_MAIN})
target_link_libraries(xray-glloader ${platform_libs})
