project(colorgen)

set(SOURCES main.cc)

add_executable(colorgen ${SOURCES})
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")

# add_custom_target(
#     COPY_ALL ALL
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_CURRENT_SOURCE_DIR}/colordefs ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/colordefs)
