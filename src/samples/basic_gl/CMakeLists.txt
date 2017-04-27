cmake_minimum_required(VERSION 3.6)
project(basic-gl-app)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/src/samples/basic_gl")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/samples/basic_gl")

include_directories(${proj_inc_dir})

set(project_sources
    # ${proj_inc_dir}/animated_plane.hpp
    # ${proj_src_dir}/animated_plane.cc
    # ${proj_inc_dir}/animated_plane_model.hpp
    ${proj_inc_dir}/cap2/colored_circle/colored_circle_demo.hpp
    ${proj_src_dir}/cap2/colored_circle/colored_circle_demo.cc
    ${proj_inc_dir}/demo_base.hpp
    ${proj_src_dir}/demo_base.cc
    ${proj_inc_dir}/debug_record.hpp
    ${proj_src_dir}/debug_record.cc

    ${proj_src_dir}/misc/fractals/fractal_demo.cc
    ${proj_inc_dir}/misc/fractals/fractal_demo.hpp
    ${proj_src_dir}/misc/texture_array/texture_array_demo.cc
    ${proj_inc_dir}/misc/texture_array/texture_array_demo.hpp

    ${proj_inc_dir}/misc/mesh/mesh_demo.hpp
    ${proj_src_dir}/misc/mesh/mesh_demo.cc

    ${proj_inc_dir}/fwd_app.hpp
    ${proj_inc_dir}/helpers.hpp
    # ${proj_inc_dir}/init_context.hpp
    ${proj_inc_dir}/light_source.hpp
    # ${proj_inc_dir}/lit_torus.hpp
    # ${proj_src_dir}/lit_torus.cc
    ${proj_src_dir}/main.cc
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
    # ${proj_inc_dir}/cap5/normal_map/normal_map_demo.hpp
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

source_group(shaders FILES ${shader_files})

add_executable(basic-gl-app ${project_sources} ${shader_files})

message("Platform libs ${SYS_DEP_LIBS}")

target_link_libraries(basic-gl-app
    xray-base
    xray-opengl-renderer
    xray-rendering
    xray-scene
    xray-ui
    # libconfig
    # stb
    ${ASSIMP_LIBRARY}
    ${SYS_DEP_LIBS})

#add_custom_target(COPY_TBB ALL
#    COMMAND ${CMAKE_COMMAND} -E copy ${TBB_LIBRARY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
#
add_custom_target(COPY_ASSIMP ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_BIN_LIBRARY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(COPY_SHADERS ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

add_custom_target(COPY_CONFIG_FILES ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config)
