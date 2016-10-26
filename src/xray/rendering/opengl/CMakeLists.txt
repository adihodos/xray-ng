project(xray-opengl-renderer)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/rendering/opengl")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/rendering/opengl")

set(project_sources
    ${proj_inc_dir}/gl_handles.hpp
    ${proj_src_dir}/gl_handles.cc
    ${proj_inc_dir}/gpu_program.hpp
    ${proj_src_dir}/gpu_program.cc
    ${proj_inc_dir}/program_pipeline.hpp
    ${proj_src_dir}/program_pipeline.cc
    ${proj_inc_dir}/scoped_resource_mapping.hpp
    ${proj_inc_dir}/scoped_state.hpp
    ${proj_inc_dir}/shader_base.hpp
    ${proj_src_dir}/shader_base.cc)

add_library(xray-opengl-renderer STATIC ${project_sources})
target_link_libraries(xray-opengl-renderer xray-glloader)
