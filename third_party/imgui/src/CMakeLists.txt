project(xray-imgui)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/third_party/imgui/include")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

set(project_sources
    ${proj_inc_dir}/imgui/imconfig.h
    ${proj_inc_dir}/imgui/imgui.h
    ${proj_src_dir}/imgui_internal.h
    ${proj_src_dir}/imgui.cpp
    ${proj_src_dir}/imgui_draw.cpp
    ${proj_src_dir}/imgui_demo.cpp
    ${proj_src_dir}/stb_rect_pack.h
    ${proj_src_dir}/stb_textedit.h
    ${proj_src_dir}/stb_truetype.h
    )

# if (WIN32 AND NOT MSVC)
#     set(platform_libs imm32)
# endif()    

add_library(${PROJECT_NAME} STATIC ${project_sources})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:third_party/imgui/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui/include>
  PRIVATE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
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

target_link_libraries(${PROJECT_NAME} PUBLIC xray-math)
