project(xray-physics)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/physics")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/physics")

set(project_sources
    ${proj_inc_dir}/constants.hpp
    ${proj_inc_dir}/particle.hpp
    ${proj_src_dir}/particle.cc
)

source_group(physics FILES ${project_sources})

add_library(xray-physics STATIC ${project_sources})
