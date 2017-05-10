project(xray-scene)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/scene")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/scene")

set(project_sources
    ${proj_inc_dir}/camera.hpp
    ${proj_src_dir}/camera.cc
    ${proj_inc_dir}/camera_controller.hpp
    ${proj_src_dir}/camera_controller.cc
    ${proj_inc_dir}/camera_controller_spherical_coords.hpp
    ${proj_src_dir}/camera_controller_spherical_coords.cc
    ${proj_inc_dir}/point_light.hpp
    )

add_library(xray-scene STATIC ${project_sources})
target_link_libraries(xray-scene libconfig xray-ui)
#target_link_libraries(xray ${GLFW_LIBRARY} xray-base xray-opengl-renderer)