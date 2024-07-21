cmake_minimum_required(VERSION 3.20)
project(xray-demo-common-core)

set(proj_inc_dir "${PROJECT_SOURCE_DIR}/include")
set(proj_src_dir "${PROJECT_SOURCE_DIR}/src")

set(project_sources
    ${proj_inc_dir}/demo_base.hpp ${proj_src_dir}/demo_base.cc
    ${proj_inc_dir}/fwd_app.hpp ${proj_inc_dir}/init_context.hpp)

source_group(code FILES ${project_sources})
add_library(${PROJECT_NAME} STATIC ${project_sources})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${proj_inc_dir}
  PRIVATE ${proj_src_dir})

target_link_libraries(
  ${PROJECT_NAME} PUBLIC xray-set-compile-options-lib xray-base spdlog::spdlog
                         xray-rendering)
