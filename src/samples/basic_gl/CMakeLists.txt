project(basic-gl-app)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/src/samples/basic_gl")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/samples/basic_gl")

include_directories(${proj_inc_dir})

set(project_sources
    ${proj_inc_dir}/animated_plane.hpp
    ${proj_src_dir}/animated_plane.cc
    ${proj_inc_dir}/animated_plane_model.hpp
    ${proj_inc_dir}/colored_circle.hpp
    ${proj_src_dir}/colored_circle.cc
    ${proj_inc_dir}/demo_base.hpp
    ${proj_src_dir}/demo_base.cc
    ${proj_src_dir}/fractal.cc
    ${proj_inc_dir}/fractal.hpp
    ${proj_inc_dir}/helpers.hpp
    ${proj_inc_dir}/light_source.hpp
    ${proj_inc_dir}/lit_torus.hpp
    ${proj_src_dir}/lit_torus.cc
    ${proj_src_dir}/main.cc
    ${proj_inc_dir}/material.hpp
    ${proj_src_dir}/material.cc
    ${proj_inc_dir}/std_assets.hpp
    ${proj_inc_dir}/cap3/soubroutines/soubroutines_demo.hpp
    ${proj_src_dir}/cap3/soubroutines/soubroutines_demo.cc
    ${proj_inc_dir}/cap3/frag_discard/frag_discard_demo.hpp
    ${proj_inc_dir}/cap3/frag_discard/frag_discard_demo.cc
    ${proj_inc_dir}/cap4/multiple_lights/multiple_lights_demo.hpp
    ${proj_inc_dir}/cap4/multiple_lights/multiple_lights_demo.cc
    ${proj_inc_dir}/cap4/directional_lights/directional_lights_demo.hpp
    ${proj_inc_dir}/cap4/directional_lights/directional_lights_demo.cc
    ${proj_inc_dir}/cap4/per_fragment_lighting/per_fragment_lighting_demo.hpp
    ${proj_inc_dir}/cap4/per_fragment_lighting/per_fragment_lighting_demo.cc
    ${proj_inc_dir}/cap4/toon_shading/toon_shading_demo.hpp
    ${proj_inc_dir}/cap4/toon_shading/toon_shading_demo.cc
    ${proj_inc_dir}/cap4/spotlight/spotlight_demo.hpp
    ${proj_inc_dir}/cap4/spotlight/spotlight_demo.cc
    ${proj_inc_dir}/cap4/fog/fog_demo.hpp
    ${proj_inc_dir}/cap4/fog/fog_demo.cc
    ${proj_inc_dir}/cap5/textures/textures_demo.hpp
    ${proj_inc_dir}/cap5/textures/textures_demo.cc
    ${proj_inc_dir}/cap5/multiple_textures/multiple_textures_demo.hpp
    ${proj_inc_dir}/cap5/multiple_textures/multiple_textures_demo.cc
    ${proj_inc_dir}/cap5/discard_alphamap/discard_alphamap_demo.hpp
    ${proj_inc_dir}/cap5/discard_alphamap/discard_alphamap_demo.cc
    ${proj_inc_dir}/cap5/normal_map/normal_map_demo.hpp
    ${proj_inc_dir}/cap5/normal_map/normal_map_demo.cc
    ${proj_inc_dir}/cap5/reflection/reflection_demo.hpp
    ${proj_src_dir}/cap5/reflection/reflection_demo.cc
    ${proj_inc_dir}/cap5/refraction/refraction_demo.hpp
    ${proj_src_dir}/cap5/refraction/refraction_demo.cc
    ${proj_inc_dir}/cap5/render_texture/render_texture_demo.hpp
    ${proj_src_dir}/cap5/render_texture/render_texture_demo.cc
    ${proj_inc_dir}/cap6/edge_detect/edge_detect_demo.hpp
    ${proj_src_dir}/cap6/edge_detect/edge_detect_demo.cc
    # ${proj_inc_dir}/config_reader_base.hpp
    # ${proj_inc_dir}/config_reader_float3.hpp
    # ${proj_inc_dir}/config_reader_rgb_color.hpp
    # ${proj_inc_dir}/config_reader_scene.hpp
    # ${proj_src_dir}/config_reader_scene.cc
    )

set(shader_files
    ${proj_src_dir}/shaders/dbg/debug.vs
    ${proj_src_dir}/shaders/dbg/debug.gs
    ${proj_src_dir}/shaders/dbg/debug.fs
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
    ${GLFW_LIBRARY}
    ${SYS_DEP_LIBS})

add_custom_target(COPY_TBB ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${TBB_LIBRARY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(COPY_ASSIMP ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_BIN_LIBRARY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(COPY_SHADERS ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

add_custom_target(COPY_CONFIG_FILES ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config)
