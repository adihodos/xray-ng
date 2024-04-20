project(xray-opengl-renderer)

message("xxx OpenGL xxx")

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/rendering/opengl")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/rendering/opengl")

set(project_sources
  ${proj_inc_dir}/gl_handles.hpp
  ${proj_src_dir}/gl_handles.cc
  ${proj_inc_dir}/gl_misc.hpp
  ${proj_inc_dir}/gpu_program.hpp
  ${proj_src_dir}/gpu_program.cc
  ${proj_inc_dir}/scoped_opengl_setting.hpp
  ${proj_inc_dir}/program_pipeline.hpp
  ${proj_src_dir}/program_pipeline.cc
  ${proj_inc_dir}/scoped_resource_mapping.hpp
  ${proj_inc_dir}/scoped_state.hpp
  ${proj_inc_dir}/shader_base.hpp
  ${proj_src_dir}/shader_base.cc)

add_library(${PROJECT_NAME} STATIC ${project_sources})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} xray-base xray-glloader tl::optional STLSoft mio::mio)
