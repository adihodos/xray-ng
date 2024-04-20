project(mesh-tool)

set(SOURCES mesh_tool.cc)

add_executable(mesh-tool ${SOURCES})
target_link_libraries(mesh-tool mio::mio tl::optional xray-base ${SYS_DEP_LIBS} tinyobj)

# add_custom_target(
#     COPY_ALL ALL
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_CURRENT_SOURCE_DIR}/enumdefs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/enumdefs)

# add_custom_target(
#     COPY_TEMPLATE ALL
#     COMMAND ${CMAKE_COMMAND} -E copy
#     ${CMAKE_CURRENT_SOURCE_DIR}/enum.cc.template ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/enum.cc.template)

# add_custom_target(
#     COPY_TEMPLATE_HPP ALL
#     COMMAND ${CMAKE_COMMAND} -E copy
#     ${CMAKE_CURRENT_SOURCE_DIR}/enum.hpp.template ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/enum.hpp.template)
