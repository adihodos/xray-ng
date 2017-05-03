project(tinyobj)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/third_party/tinyobj/include")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/third_party/tinyobj/src")

include_directories(${proj_src_dir})

set(project_sources
    ${proj_inc_dir}/tinyobj/tinyobj_loader_opt.h
    ${proj_src_dir}/tinyobj_loader_opt.cc
    ${proj_src_dir}/ltalloc.h
    ${proj_src_dir}/ltalloc.hpp
    ${proj_src_dir}/ltalloc.cc
    )

add_library(tinyobj STATIC ${project_sources})
#target_link_libraries(xray-imgui xray-opengl-renderer ${platform_libs})
