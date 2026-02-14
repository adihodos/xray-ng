project(xray-stargen)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/stargen")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

set(project_sources
  ${proj_inc_dir}/stargen.gas.hpp
  ${proj_src_dir}/stargen.gas.cc
  ${proj_inc_dir}/stargen.helpers.hpp
  ${proj_src_dir}/stargen.helpers.cc
  ${proj_inc_dir}/stargen.star.hpp
  ${proj_src_dir}/stargen.star.cc
  ${proj_inc_dir}/stargen.atmosphere.hpp
  ${proj_src_dir}/stargen.atmosphere.cc
  ${proj_inc_dir}/stargen.planet.hpp
  ${proj_src_dir}/stargen.planet.cc
  ${proj_inc_dir}/stargen.dust.hpp
  ${proj_src_dir}/stargen.dust.cc
  )

add_library(${PROJECT_NAME} STATIC ${project_sources})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PRIVATE)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_compile_definitions(${PROJECT_NAME}
  PRIVATE $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC
  xray-set-compile-options-lib
  xray-base
  cpp-lazy::cpp-lazy
)
