project(xray-scene)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/scene")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

set(project_sources
  ${proj_inc_dir}/camera.hpp
  ${proj_src_dir}/camera.cc
  ${proj_inc_dir}/camera_controller_spherical_coords.hpp
  ${proj_src_dir}/camera_controller_spherical_coords.cc
  ${proj_inc_dir}/fps_camera_controller.hpp
  ${proj_src_dir}/fps_camera_controller.cc
  ${proj_inc_dir}/point_light.hpp
)

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

target_link_libraries(${PROJECT_NAME} PUBLIC xray-base config tl::optional PRIVATE joboccara::pipes)
