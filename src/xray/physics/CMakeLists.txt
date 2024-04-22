project(xray-physics)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/physics")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

set(project_sources
    ${proj_inc_dir}/constants.hpp
    ${proj_inc_dir}/particle.hpp
    ${proj_src_dir}/particle.cc
)

source_group(physics FILES ${project_sources})

add_library(${PROJECT_NAME} STATIC ${project_sources})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PRIVATE
)

target_compile_features(
  ${PROJECT_NAME}
  PUBLIC cxx_std_20
)

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE
  $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>
)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC xray-math
)

