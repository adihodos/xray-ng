project(fontgen)

find_package(ZLIB REQUIRED)
find_package(PNG REQUIRED)
find_package(FREETYPE REQUIRED)

set(SOURCES main.cc)

# message("Freetype include dirs ${FREETYPE_INCLUDE_DIRS}")
message("Png library ${PNG_LIBRARIES}")
include_directories(${FREETYPE_INCLUDE_DIRS})
include_directories(${PNG_INCLUDE_DIRS})

add_executable(fontgen ${SOURCES})
target_link_libraries(fontgen ${FREETYPE_LIBRARIES} xray-base)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")

# add_custom_command(
#     TARGET fontgen PRE_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_CURRENT_SOURCE_DIR}/colordefs ${EXECUTABLE_OUTPUT_PATH}/colordefs)
