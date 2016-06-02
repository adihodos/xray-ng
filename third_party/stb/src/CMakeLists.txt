project(stb)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/third_party/stb/include")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/third_party/stb/src")

set(project_sources
    ${proj_src_dir}/stb_gen.cc)

add_library(stb STATIC ${project_sources})