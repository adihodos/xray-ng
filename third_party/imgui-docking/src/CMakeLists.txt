project(xray-imgui-docking)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/third_party/imgui-docking/include")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

set(project_sources
    ${proj_inc_dir}/imgui/imconfig.h
    ${proj_inc_dir}/imgui/imgui.h
    ${proj_inc_dir}/imgui/imgui_internal.h
    ${proj_src_dir}/imgui.cpp
    ${proj_src_dir}/imgui_draw.cpp
    ${proj_src_dir}/imgui_demo.cpp
	${proj_src_dir}/imgui_tables.cpp
	${proj_src_dir}/imgui_widgets.cpp
    ${proj_inc_dir}/imgui/imstb_rectpack.h
    ${proj_inc_dir}/imgui/imstb_textedit.h
    ${proj_inc_dir}/imgui/imstb_truetype.h
    )

# if (WIN32 AND NOT MSVC)
#     set(platform_libs imm32)
# endif()    

add_library(${PROJECT_NAME} STATIC ${project_sources})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  $<INSTALL_INTERFACE:third_party/imgui-docking/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui-docking/include>
  PRIVATE
  # $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/third_party/imgui-docking/include/imgui>
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
