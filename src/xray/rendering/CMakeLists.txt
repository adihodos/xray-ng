project(xray-rendering)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/rendering")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

find_package(Vulkan REQUIRED COMPONENTS glslc shaderc_combined)

set(project_sources
    ${proj_inc_dir}/colors/color_cast_rgb_hsv.hpp
    ${proj_src_dir}/colors/color_cast_rgb_hsv.cc
    ${proj_inc_dir}/colors/color_cast_rgb_hsl.hpp
    ${proj_src_dir}/colors/color_cast_rgb_hsl.cc
    ${proj_inc_dir}/colors/color_cast_rgb_xyz.hpp
    ${proj_src_dir}/colors/color_cast_rgb_xyz.cc
    ${proj_inc_dir}/colors/color_palettes.hpp
    ${proj_src_dir}/colors/color_palettes.cc
    ${proj_inc_dir}/geometry/aabb_visualizer.hpp
    ${proj_src_dir}/geometry/aabb_visualizer.cc
    ${proj_inc_dir}/geometry/surface_normal_visualizer.hpp
    ${proj_src_dir}/geometry/surface_normal_visualizer.cc
    ${proj_inc_dir}/geometry/geometry_data.hpp
    ${proj_inc_dir}/geometry/geometry_factory.hpp
    ${proj_src_dir}/geometry/geometry_factory.cc
    ${proj_inc_dir}/vertex_format/vertex_format.hpp
    ${proj_inc_dir}/vertex_format/vertex_p.hpp
    ${proj_inc_dir}/vertex_format/vertex_pc.hpp
    ${proj_inc_dir}/vertex_format/vertex_pn.hpp
    ${proj_inc_dir}/vertex_format/vertex_pnt.hpp
    ${proj_inc_dir}/vertex_format/vertex_pt.hpp
    ${proj_inc_dir}/vertex_format/vertex_pntt.hpp
    ${proj_inc_dir}/render_stage.hpp
    ${proj_src_dir}/render_stage.cc
    ${proj_inc_dir}/debug_draw.hpp
    ${proj_src_dir}/debug_draw.cc
    ${proj_inc_dir}/mesh_loader.hpp
    ${proj_src_dir}/mesh_loader.cc
    ${proj_inc_dir}/mesh.hpp
    ${proj_src_dir}/mesh.cc
    ${proj_inc_dir}/texture_loader.hpp
    ${proj_src_dir}/texture_loader.cc)

set(gl_proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/rendering/opengl")
set(gl_proj_src_dir "${PROJECT_SOURCE_DIR}/opengl")

set(project_sources_gl
    ${gl_proj_inc_dir}/gl_handles.hpp
    ${gl_proj_src_dir}/gl_handles.cc
    ${gl_proj_inc_dir}/gl_misc.hpp
    ${gl_proj_inc_dir}/gpu_program.hpp
    ${gl_proj_src_dir}/gpu_program.cc
    ${gl_proj_inc_dir}/scoped_opengl_setting.hpp
    ${gl_proj_inc_dir}/program_pipeline.hpp
    ${gl_proj_src_dir}/program_pipeline.cc
    ${gl_proj_inc_dir}/scoped_resource_mapping.hpp
    ${gl_proj_src_dir}/scoped_resource_mapping.cc
    ${gl_proj_inc_dir}/scoped_state.hpp
    ${gl_proj_inc_dir}/shader_base.hpp
    ${gl_proj_src_dir}/shader_base.cc)

set(vk-proj-inc-dir
    "${CMAKE_SOURCE_DIR}/include/xray/rendering/vulkan.renderer")
set(vk-proj-src-dir "${PROJECT_SOURCE_DIR}/vulkan.renderer")
set(vk-project-sources
    ${vk-proj-inc-dir}/vulkan.renderer.hpp
    ${vk-proj-src-dir}/vulkan.renderer.cc
    ${vk-proj-inc-dir}/vulkan.pipeline.builder.hpp
    ${vk-proj-src-dir}/vulkan.pipeline.builder.cc)

add_library(${PROJECT_NAME} STATIC ${project_sources} ${project_sources_gl}
                                   ${vk-project-sources})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/xray/rendering>
  PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC XRAY_GRAPHICS_API_VULKAN
  PRIVATE $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC xray-glloader
         tl::optional
         xray-base
         mio::mio
         itlib::itlib
         Vulkan::Vulkan
         swl-variant
  PRIVATE TBB::tbb cpp-lazy::cpp-lazy Vulkan::shaderc_combined
          spirv-reflect-static $<$<PLATFORM_ID:Linux>:Xinerama>)
