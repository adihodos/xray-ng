cmake_minimum_required(VERSION 3.20)
project(vulkan-demo-app)

set(proj_inc_dir "${PROJECT_SOURCE_DIR}/code")
set(proj_src_dir "${PROJECT_SOURCE_DIR}/code")

set(project_sources
    ${proj_src_dir}/vulkan-main.cc ${proj_inc_dir}/triangle/triangle.hpp
    ${proj_src_dir}/triangle/triangle.cc)

set(shader_src_dir "${PROJECT_SOURCE_DIR}/shaders")
set(shader_files ${shader_src_dir}/triangle/tri.vert
                 ${shader_src_dir}/triangle/tri.frag)

set(config_dir "${PROJECT_SOURCE_DIR}/config")
set(config_files ${config_dir}/app_config.conf
                 ${config_dir}/cam_controller_spherical.conf)

source_group(code FILES ${project_sources})
source_group(shaders FILES ${shader_files})
source_group(configuration FILES ${config_files})

add_executable(${PROJECT_NAME} ${project_sources} ${shader_files}
                               ${config_files})

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY
             "$<0:>${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin"
             LIBRARY_OUTPUT_DIRECTORY
             "$<0:>${CMAKE_BINARY_DIR}/${PROJECT_NAME}/lib"
             ARCHIVE_OUTPUT_DIRECTORY
             "$<0:>${CMAKE_BINARY_DIR}/${PROJECT_NAME}/lib")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_include_directories(${PROJECT_NAME} PRIVATE ${proj_inc_dir}
                                                   ${proj_src_dir})

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE xray-set-compile-options-lib
          xray-demo-common-core
          xray-base
          xray-rendering
          xray-physics
          xray-scene
          xray-ui
          xray-math
          TBB::tbb
          itlib::itlib
          cpp-lazy::cpp-lazy
          reflectcpp
          spdlog::spdlog)

cmake_language(
  EVAL
  CODE
  [=[
get_target_property(target_output_directory ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
file(GENERATE OUTPUT "${target_output_directory}/config/app_config.override.conf"
CONTENT "
directories : {
    root_win = \"c:/games/xray\";
    root = \"/home/adi/games/xray\";
    models = \"assets/models\";
    textures = \"assets/textures\";
    shaders = \"${CMAKE_CURRENT_SOURCE_DIR}/shaders\";
};"
CONDITION $<CONFIG:Debug>
NEWLINE_STYLE UNIX)
]=])

get_target_property(target_output_directory ${PROJECT_NAME}
                    RUNTIME_OUTPUT_DIRECTORY)
add_custom_target(
  COPY_SHADERS ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
          ${target_output_directory}/shaders
  COMMENT "::: Copying shader files :::")

add_custom_target(
  COPY_CONFIG_FILES ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config
          ${target_output_directory}/config
  COMMENT "::: Copying configuration files :::")

add_custom_target(
  OVERWRITE_APP_CONFIG ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy
    "${target_output_directory}/config/app_config.override.conf"
    "${target_output_directory}/config/app_config.conf"
  # DEPENDS COPY_CONFIG_FILES
  COMMENT
    "::: WARNING: overwriting main configuration file with a custom one ! :::")

add_dependencies(OVERWRITE_APP_CONFIG COPY_CONFIG_FILES)
