project(opengl-lib)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/third_party/opengl/include")
set(proj_src_dir "${CMAKE_SOURCE_DIR}")

set(project_sources
  ${proj_inc_dir}/opengl.hpp
  ${proj_src_dir}/opengl.cc
)

add_library(${PROJECT_NAME} STATIC ${project_sources})

target_compile_features(
  ${PROJECT_NAME}
  PUBLIC cxx_std_20)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/opengl/include>
  PRIVATE
)


target_link_libraries(${PROJECT_NAME}  xray-glloader tl::optional STLSoft mio::mio)
