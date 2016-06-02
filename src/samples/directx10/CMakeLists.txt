project(basic-dx10-app)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/src/samples/directx10")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/samples/directx10")

set(project_sources
    ${proj_src_dir}/main.cc
    ${proj_inc_dir}/basic_window.hpp
    ${proj_src_dir}/basic_window.cc
    ${proj_inc_dir}/gpu_buffer.hpp
    ${proj_src_dir}/gpu_buffer.cc
    ${proj_inc_dir}/gpu_shader.hpp
    ${proj_inc_dir}/gpu_shader_common_core.hpp
    ${proj_src_dir}/gpu_shader_common_core.cc
    ${proj_inc_dir}/rasterizer_state.hpp
    ${proj_src_dir}/rasterizer_state.cc
    ${proj_inc_dir}/renderer_dx10.hpp
    ${proj_src_dir}/renderer_dx10.cc
    ${proj_inc_dir}/scoped_resource_mapping.hpp
    ${proj_src_dir}/scoped_resource_mapping.cc
    ${proj_inc_dir}/simple_object.hpp
    ${proj_src_dir}/simple_object.cc
    )

add_executable(basic-dx10-app ${project_sources})

set(DX_LIBS dxgi d3d10 d3dcompiler dxguid)

target_link_libraries(basic-dx10-app
    xray-base
    xray-geometry-factory
    xray-scene
#    xray-ui
    libconfig
    ${ASSIMP_LIBRARY}
    ${SYS_DEP_LIBS}
    ${DX_LIBS})


add_custom_target(COPY_TBB ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${TBB_LIBRARY} ${EXECUTABLE_OUTPUT_PATH})

add_custom_target(COPY_ASSIMP ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_LIBRARY} ${EXECUTABLE_OUTPUT_PATH})

add_custom_target(COPY_SHADERS ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${EXECUTABLE_OUTPUT_PATH}/shaders)

add_custom_target(COPY_CONFIG_FILES ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${EXECUTABLE_OUTPUT_PATH}/config)
