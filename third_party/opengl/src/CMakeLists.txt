project(xray-glloader)

set(proj_inc_dir ${CMAKE_SOURCE_DIR}/third_party/opengl/include)
set(proj_src_dir ${PROJECT_SOURCE_DIR})

set(PROJECT_SOURCES_MAIN
    ${proj_inc_dir}/opengl/opengl.hpp
    ${proj_src_dir}/opengl.cpp)

#source_group("xray_ui" FILES ${PROJECT_SOURCES_MAIN})

add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES_MAIN})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/opengl/include>
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

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  $<$<PLATFORM_ID:linux>:GL>
)
