cmake_minimum_required(VERSION 3.20)
project(vulkan-demo-app)

set(proj_inc_dir "${PROJECT_SOURCE_DIR}/code")
set(proj_src_dir "${PROJECT_SOURCE_DIR}/code")

set(project_sources
    ${proj_src_dir}/vulkan-main.cc ${proj_inc_dir}/triangle/triangle.hpp
    ${proj_src_dir}/bindless.pipeline.config.hpp
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

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)
endif()

add_executable(build-info-tool ${proj_src_dir}/build-info-tool.cc)
target_compile_features(build-info-tool PRIVATE cxx_std_20)
target_compile_definitions(
  build-info-tool PRIVATE $<$<PLATFORM_ID:Windows>:PLATFORM_WINDOWS>
                          $<$<PLATFORM_ID:Linux>:PLATFORM_LINUX>)

# set(windows_libs User32 Secur32 Ws2_32)

target_link_libraries(
  build-info-tool
  PRIVATE # "$<$<PLATFORM_ID:Windows>:${windows_libs}>"
          "$<$<PLATFORM_ID:Windows>:User32;Secur32;Ws2_32>"
          $<$<PLATFORM_ID:Linux>:PkgConfig::LIBGIT2> fmt::fmt)

set(OVERWRITE_APP_CONFIG
    TRUE
    CACHE BOOL "Redirect shader directory to the source directory" FORCE)

add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin/build.config.hpp"
  COMMAND build-info-tool "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin"
          --write-build-info
  VERBATIM)

# add_custom_target( WriteBuildInfo COMMAND build-info-tool
# "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin" --write-build-info BYPRODUCTS
# "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin/build.config.hpp" COMMENT ":::
# Running build-info-tool (custom target) :::" VERBATIM)

add_executable(
  ${PROJECT_NAME}
  ${project_sources} "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin/build.config.hpp"
  ${shader_files} ${config_files})

add_dependencies(${PROJECT_NAME} build-info-tool)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY
             "$<0:>${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin"
             LIBRARY_OUTPUT_DIRECTORY
             "$<0:>${CMAKE_BINARY_DIR}/${PROJECT_NAME}/lib"
             ARCHIVE_OUTPUT_DIRECTORY
             "$<0:>${CMAKE_BINARY_DIR}/${PROJECT_NAME}/lib"
             VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(
  ${PROJECT_NAME} PRIVATE ${proj_inc_dir} ${proj_src_dir}
                          "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin")

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
          itlib::itlib
          cpp-lazy::cpp-lazy
          reflectcpp
          spdlog::spdlog
          concurrencpp::concurrencpp
          $<IF:$<PLATFORM_ID:Windows>,libconfig,config>
          noise
          atomic_queue
          fastgltf::fastgltf
        )

get_target_property(target_output_directory ${PROJECT_NAME}
                    RUNTIME_OUTPUT_DIRECTORY)

add_custom_target(
  COPY_CONFIGS ALL
  COMMAND
    ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/config" ${target_output_directory}/config
  COMMAND
    ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/config/vk_layer_settings.txt" "${target_output_directory}/vk_layer_settings.txt"
  COMMENT "::: Copy scene configuration step :::"
  VERBATIM
  DEPENDS 
  "${PROJECT_SOURCE_DIR}/config/scenes/simple.scene.conf"
  "${PROJECT_SOURCE_DIR}/config/renderer.conf"
  "${PROJECT_SOURCE_DIR}/config/vk_layer_settings.txt" 
  SOURCES 
  "${PROJECT_SOURCE_DIR}/config/scenes/simple.scene.conf"
  "${PROJECT_SOURCE_DIR}/config/renderer.conf"
  "${PROJECT_SOURCE_DIR}/config/vk_layer_settings.txt" 
)

add_dependencies(${PROJECT_NAME} COPY_CONFIGS)

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  # COPY_SHADERS ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
          ${target_output_directory}/shaders
  COMMENT "::: Copying shader files :::")

set(build_tool_cmd_args "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin")
set(build_tool_cmd_args1 "--write-app-config=${PROJECT_SOURCE_DIR}/shaders")
set(build_tool_byproducts
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/bin/config/app_config.conf")

message("OVERWRITE ${OVERWRITE_APP_CONFIG}")

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config
          ${target_output_directory}/config
  COMMAND
    build-info-tool $<$<BOOL:${OVERWRITE_APP_CONFIG}>:${build_tool_cmd_args}>
    # $<$<BOOL:${OVERWRITE_APP_CONFIG}>:${build_tool_cmd_args1}
  BYPRODUCTS $<$<BOOL:${OVERWRITE_APP_CONFIG}>:${build_tool_byproducts}>
  COMMENT "::: Overwrite config step :::")

add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:${PROJECT_NAME}>
          $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
  COMMAND_EXPAND_LISTS)
