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
    ${proj_inc_dir}/camera.controller.arcball.hpp
    ${proj_src_dir}/camera.controller.arcball.cc
    ${proj_inc_dir}/point_light.hpp
    ${proj_inc_dir}/light.types.hpp
    ${proj_inc_dir}/scene.description.hpp
    ${proj_src_dir}/scene.description.cc
    ${proj_inc_dir}/scene.definition.hpp
    ${proj_src_dir}/scene.definition.cc)

add_library(${PROJECT_NAME} STATIC ${project_sources})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PRIVATE)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_compile_definitions(${PROJECT_NAME}
                           PRIVATE $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC xray-base xray-ui xray-rendering
         $<IF:$<PLATFORM_ID:Windows>,libconfig,config> tl::optional
  PRIVATE joboccara::pipes)
