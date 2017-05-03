project (xray-rendering)

if (XRAY_USE_DIRECTX11)
    add_subdirectory(directx)
else()
    add_subdirectory(opengl)
endif()

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/rendering")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/rendering")

set(project_sources
    ${proj_inc_dir}/colors/color_cast_rgb_hsv.hpp
    ${proj_src_dir}/colors/color_cast_rgb_hsv.cc
    ${proj_inc_dir}/colors/color_cast_rgb_hsl.hpp
    ${proj_src_dir}/colors/color_cast_rgb_hsl.cc
    ${proj_inc_dir}/colors/color_cast_rgb_xyz.hpp
    ${proj_src_dir}/colors/color_cast_rgb_xyz.cc
    ${proj_inc_dir}/colors/color_palettes.hpp
    ${proj_src_dir}/colors/color_palettes.cc

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

    ${proj_inc_dir}/texture_loader.hpp
    ${proj_src_dir}/texture_loader.cc

    ${proj_inc_dir}/mesh.hpp
    ${proj_src_dir}/mesh.cc
)

add_library(xray-rendering STATIC ${project_sources})
target_link_libraries(xray-rendering ${TBB_LIBRARY} xray-base stb tinyobj)
