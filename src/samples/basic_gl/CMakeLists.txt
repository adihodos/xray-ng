cmake_minimum_required(VERSION 3.20)
project(basic-gl-app)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/src/samples/basic_gl")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/samples/basic_gl")

set(project_sources
  # ${proj_inc_dir}/animated_plane.hpp
  # ${proj_src_dir}/animated_plane.cc
  # ${proj_inc_dir}/animated_plane_model.hpp
  #    ${proj_inc_dir}/cap2/colored_circle/colored_circle_demo.hpp
  #    ${proj_src_dir}/cap2/colored_circle/colored_circle_demo.cc
  ${proj_inc_dir}/demo_base.hpp
  ${proj_src_dir}/demo_base.cc
  #    ${proj_inc_dir}/debug_record.hpp
  #    ${proj_src_dir}/debug_record.cc

  ${proj_src_dir}/misc/fractals/fractal_demo.cc
  ${proj_inc_dir}/misc/fractals/fractal_demo.hpp

  ${proj_src_dir}/misc/texture_array/texture_array_demo.cc
  ${proj_inc_dir}/misc/texture_array/texture_array_demo.hpp

  ${proj_inc_dir}/misc/instanced_drawing/instanced_drawing_demo.hpp
  ${proj_src_dir}/misc/instanced_drawing/instanced_drawing_demo.cc

  ${proj_inc_dir}/misc/mesh/mesh_demo.hpp
  ${proj_src_dir}/misc/mesh/mesh_demo.cc

  #    ${proj_inc_dir}/misc/bufferless_draw/bufferless-draw-demo.hpp
  #    ${proj_src_dir}/misc/bufferless_draw/bufferless-draw-demo.cc

  #    ${proj_inc_dir}/misc/geometric_shapes/geometric_shapes_demo.hpp
  #    ${proj_src_dir}/misc/geometric_shapes/geometric_shapes_demo.cc

  ${proj_inc_dir}/misc/procedural_city/procedural_city_demo.hpp
  ${proj_src_dir}/misc/procedural_city/procedural_city_demo.cc

  ${proj_inc_dir}/lighting/directional_light_demo.hpp
  ${proj_src_dir}/lighting/directional_light_demo.cc

  # ${proj_inc_dir}/misc/terrain/basic/terrain_demo.hpp
  # ${proj_src_dir}/misc/terrain/basic/terrain_demo.cc


  #    ${proj_inc_dir}/lighting/point_light_demo.hpp
  #    ${proj_src_dir}/lighting/point_light_demo.cc

  ${proj_inc_dir}/fwd_app.hpp
  ${proj_inc_dir}/helpers.hpp
  # ${proj_inc_dir}/init_context.hpp
  ${proj_inc_dir}/light_source.hpp
  # ${proj_inc_dir}/lit_torus.hpp
  # ${proj_src_dir}/lit_torus.cc
  ${proj_src_dir}/main.cc

  ${proj_inc_dir}/mesh_lister.hpp
  ${proj_src_dir}/mesh_lister.cc


  # ${proj_inc_dir}/material.hpp
  # ${proj_src_dir}/material.cc
  # ${proj_inc_dir}/phong_material_component.hpp
  # ${proj_src_dir}/phong_material_component.cc
  # ${proj_inc_dir}/resize_context.hpp
  # ${proj_inc_dir}/std_assets.hpp
  # ${proj_inc_dir}/scene_loader.hpp
  # ${proj_src_dir}/scene_loader.cc
  # ${proj_inc_dir}/cap3/soubroutines/soubroutines_demo.hpp
  # ${proj_src_dir}/cap3/soubroutines/soubroutines_demo.cc
  # ${proj_inc_dir}/cap3/frag_discard/frag_discard_demo.hpp
  # ${proj_inc_dir}/cap3/frag_discard/frag_discard_demo.cc
  # ${proj_inc_dir}/cap4/multiple_lights/multiple_lights_demo.hpp
  # ${proj_inc_dir}/cap4/multiple_lights/multiple_lights_demo.cc
  # ${proj_inc_dir}/cap4/directional_lights/directional_lights_demo.hpp
  # ${proj_inc_dir}/cap4/directional_lights/directional_lights_demo.cc
  # ${proj_inc_dir}/cap4/per_fragment_lighting/per_fragment_lighting_demo.hpp
  # ${proj_inc_dir}/cap4/per_fragment_lighting/per_fragment_lighting_demo.cc
  # ${proj_inc_dir}/cap4/toon_shading/toon_shading_demo.hpp
  # ${proj_inc_dir}/cap4/toon_shading/toon_shading_demo.cc
  # ${proj_inc_dir}/cap4/spotlight/spotlight_demo.hpp
  # ${proj_inc_dir}/cap4/spotlight/spotlight_demo.cc
  # ${proj_inc_dir}/cap4/fog/fog_demo.hpp
  # ${proj_inc_dir}/cap4/fog/fog_demo.cc
  # ${proj_inc_dir}/cap5/textures/textures_demo.hpp
  # ${proj_inc_dir}/cap5/textures/textures_demo.cc
  # ${proj_inc_dir}/cap5/multiple_textures/multiple_textures_demo.hpp
  # ${proj_inc_dir}/cap5/multiple_textures/multiple_textures_demo.cc
  # ${proj_inc_dir}/cap5/discard_alphamap/discard_alphamap_demo.hpp
  # ${proj_inc_dir}/cap5/discard_alphamap/discard_alphamap_demo.cc
  # ${proj_inc_dir}/cap5/normal_map/normal_8map_demo.hpp
  # ${proj_inc_dir}/cap5/normal_map/normal_map_demo.cc
  # ${proj_inc_dir}/cap5/reflection/reflection_demo.hpp
  # ${proj_src_dir}/cap5/reflection/reflection_demo.cc
  # ${proj_inc_dir}/cap5/refraction/refraction_demo.hpp
  # ${proj_src_dir}/cap5/refraction/refraction_demo.cc
  # ${proj_inc_dir}/cap5/render_texture/render_texture_demo.hpp
  # ${proj_src_dir}/cap5/render_texture/render_texture_demo.cc
  # ${proj_inc_dir}/cap6/edge_detect/edge_detect_demo.hpp
  # ${proj_src_dir}/cap6/edge_detect/edge_detect_demo.cc
  # ${proj_inc_dir}/cap6/hdr_tonemapping/hdr_tonemapping_demo.hpp
  # ${proj_src_dir}/cap6/hdr_tonemapping/hdr_tonemapping_demo.cc
)

set(shader_files
  ${proj_src_dir}/shaders/dbg/debug.vs
  ${proj_src_dir}/shaders/dbg/debug.gs
  ${proj_src_dir}/shaders/dbg/debug.fs

  ${proj_src_dir}/shaders/misc/fractals/fractal_vert.glsl
  ${proj_src_dir}/shaders/misc/fractals/fractal_frag.glsl

  ${proj_src_dir}/shaders/misc/texture_array/shader.vert.glsl
  ${proj_src_dir}/shaders/misc/texture_array/shader.frag.glsl

  ${proj_src_dir}/shaders/misc/mesh/vs.glsl
  ${proj_src_dir}/shaders/misc/mesh/fs.glsl

  ${proj_src_dir}/shaders/misc/bufferless_draw/vs.glsl
  ${proj_src_dir}/shaders/misc/bufferless_draw/gs.glsl
  ${proj_src_dir}/shaders/misc/bufferless_draw/fs.glsl

  ${proj_src_dir}/shaders/misc/instanced_draw/vs.glsl
  ${proj_src_dir}/shaders/misc/instanced_draw/fs.glsl

  ${proj_src_dir}/shaders/misc/procedural_city/vs.glsl
  ${proj_src_dir}/shaders/misc/procedural_city/fs.glsl

  ${proj_src_dir}/shaders/misc/geometric_shapes/vs.glsl
  ${proj_src_dir}/shaders/misc/geometric_shapes/fs.glsl

  ${proj_src_dir}/shaders/misc/terrain/basic/vs.glsl
  ${proj_src_dir}/shaders/misc/terrain/basic/fs.glsl

  ${proj_src_dir}/shaders/less4/phong_lighting.glsl
  ${proj_src_dir}/shaders/less4/frag_passthrough.glsl
  ${proj_src_dir}/shaders/cap3/soubroutines/vert_shader.glsl
  ${proj_src_dir}/shaders/cap3/soubroutines/frag_shader.glsl
  ${proj_src_dir}/shaders/cap3/frag_discard/vert_shader.glsl
  ${proj_src_dir}/shaders/cap3/frag_discard/frag_shader.glsl
  ${proj_src_dir}/shaders/cap4/multiple_lights/vert_shader.glsl
  ${proj_src_dir}/shaders/cap4/multiple_lights/frag_shader.glsl
  ${proj_src_dir}/shaders/cap4/directional_lights/vert_shader.glsl
  ${proj_src_dir}/shaders/cap4/directional_lights/frag_shader.glsl
  ${proj_src_dir}/shaders/cap4/per_fragment_lighting/vert_shader.glsl
  ${proj_src_dir}/shaders/cap4/per_fragment_lighting/frag_shader.glsl
  ${proj_src_dir}/shaders/cap4/toon_shading/vert_shader.glsl
  ${proj_src_dir}/shaders/cap4/toon_shading/frag_shader.glsl
  ${proj_src_dir}/shaders/cap4/spotlight/shader.vert
  ${proj_src_dir}/shaders/cap4/spotlight/shader.frag
  ${proj_src_dir}/shaders/cap4/spotlight/displacement_shader.vert
  ${proj_src_dir}/shaders/cap4/fog/shader.vert
  ${proj_src_dir}/shaders/cap4/fog/shader.frag
  ${proj_src_dir}/shaders/cap5/textures/shader.vert
  ${proj_src_dir}/shaders/cap5/textures/shader.frag
  ${proj_src_dir}/shaders/cap5/multiple_textures/shader.vert
  ${proj_src_dir}/shaders/cap5/multiple_textures/shader.frag
  ${proj_src_dir}/shaders/cap5/discard_alphamap/shader.vert
  ${proj_src_dir}/shaders/cap5/discard_alphamap/shader.frag
  ${proj_src_dir}/shaders/cap5/normal_map/shader.vert
  ${proj_src_dir}/shaders/cap5/normal_map/shader.frag
  ${proj_src_dir}/shaders/cap5/reflection/shader.vert
  ${proj_src_dir}/shaders/cap5/reflection/shader.frag
  ${proj_src_dir}/shaders/cap5/refraction/shader.vert
  ${proj_src_dir}/shaders/cap5/refraction/shader.frag
  ${proj_src_dir}/shaders/cap5/render_texture/shader.vert
  ${proj_src_dir}/shaders/cap5/render_texture/shader.frag
  ${proj_src_dir}/shaders/cap6/edge_detect/shader.vert
  ${proj_src_dir}/shaders/cap6/edge_detect/shader.frag
)

set(proj_cfg_dir "${proj_src_dir}/config")

set(config_files
  ${proj_cfg_dir}/app_config.conf
  ${proj_cfg_dir}/cap5/render_texture/demo.conf
  ${proj_cfg_dir}/misc/terrain/cam_controller_spherical.conf
  ${proj_cfg_dir}/misc/instanced_demo/cam_controller_spherical.conf
  ${proj_cfg_dir}/misc/geometric_shapes/cam_controller_spherical.conf
  ${proj_cfg_dir}/misc/fractal_demo/cam_controller_spherical.conf
  ${proj_cfg_dir}/lighting/directional_light/cam_controller_spherical.conf
  ${proj_cfg_dir}/logging.conf
  ${proj_cfg_dir}/cap6/edge_detect/app.new.conf
  ${proj_cfg_dir}/cap6/edge_detect/app.conf
  ${proj_cfg_dir}/cap6/hdr_tonemapping/app.conf
  ${proj_cfg_dir}/cam_controller_spherical.conf
)

source_group(code FILES ${project_sources})
source_group(shaders FILES ${shader_files})
source_group(configuration FILES ${config_files})

add_executable(${PROJECT_NAME} ${project_sources} ${shader_files} ${config_files})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE ${proj_inc_dir} ${proj_src_dir}
)

target_compile_definitions(
  ${PROJECT_NAME}
  PRIVATE
  $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>
  XRAY_MATH_ENABLE_FMT_SUPPORT)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  xray-base
  xray-rendering
  xray-physics
  xray-scene
  xray-ui
  xray-math
  TBB::tbb
  itlib::itlib
  joboccara::pipes
  cppitertools::cppitertools
  reflectcpp
  spdlog::spdlog
  )

cmake_language(EVAL CODE [=[
file(GENERATE OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/app_config.override.conf"
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

add_custom_target(COPY_SHADERS ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
COMMENT "::: Copying shader files :::")

add_custom_target(COPY_CONFIG_FILES ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config
COMMENT "::: Copying configuration files :::")

add_custom_target( OVERWRITE_APP_CONFIG ALL
COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/app_config.override.conf" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/app_config.conf"
# DEPENDS COPY_CONFIG_FILES
COMMENT "::: WARNING: overwriting main configuration file with a custom one ! :::")

add_dependencies(OVERWRITE_APP_CONFIG COPY_CONFIG_FILES)
