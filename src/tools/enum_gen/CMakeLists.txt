project(enum_gen)

set(SOURCES main.cc)

add_executable(enum_gen ${SOURCES})
target_link_libraries(enum_gen fmt::fmt xray-base)


add_custom_target(
    COPY_ALL ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/enumdefs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/enumdefs)

add_custom_target(
    COPY_TEMPLATE ALL
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/enum.cc.template ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/enum.cc.template)

add_custom_target(
    COPY_TEMPLATE_HPP ALL
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/enum.hpp.template ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/enum.hpp.template)
