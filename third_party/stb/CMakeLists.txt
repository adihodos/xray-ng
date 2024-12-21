cmake_minimum_required(VERSION 3.28)
project(stb LANGUAGES C)

set(source_files
    ${PROJECT_SOURCE_DIR}/include/stb/stb_c_lexer.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_connected_components.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_divide.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_ds.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_dxt.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_easy_font.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_herringbone_wang_tile.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_hexwave.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_image.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_image_resize2.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_image_write.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_include.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_leakcheck.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_perlin.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_rect_pack.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_sprintf.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_textedit.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_tilemap_editor.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_truetype.h
    ${PROJECT_SOURCE_DIR}/include/stb/stb_voxel_render.h
    ${PROJECT_SOURCE_DIR}/src/stb_vorbis.c
    ${PROJECT_SOURCE_DIR}/src/stb_impl.c)

add_library(${PROJECT_NAME} STATIC ${source_files})
add_library(stb::stb ALIAS ${PROJECT_NAME})
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<INSTALL_INTERFACE:include>
                         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)
