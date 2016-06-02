project(xray-directx-renderer)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/rendering/directx")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/rendering/directx")

set(project_sources
    ${proj_inc_dir}/dx11_renderer.hpp
    ${proj_src_dir}/dx11_renderer.cc
    ${proj_inc_dir}/gpu_shader.hpp
    ${proj_src_dir}/gpu_shader.cc
    ${proj_inc_dir}/gpu_shader_base.hpp
    ${proj_src_dir}/gpu_shader_base.cc
    ${proj_inc_dir}/scoped_mapping.hpp
    ${proj_src_dir}/scoped_mapping.cc
    ${proj_inc_dir}/scoped_state.hpp
    ${proj_src_dir}/scoped_state.cc
    ${proj_inc_dir}/vertex_shader.hpp
    ${proj_inc_dir}/pixel_shader.hpp
    ${proj_inc_dir}/DDSTextureLoader.h
    ${proj_src_dir}/DDSTextureLoader.cpp
    ${proj_inc_dir}/WICTextureLoader.h
    ${proj_src_dir}/WICTextureLoader.cpp
    )

add_library(xray-directx-renderer STATIC ${project_sources})
target_link_libraries(xray-directx-renderer dxgi d3d11 d3dcompiler dxguid)