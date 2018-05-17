cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
cmake_policy(VERSION 2.8.8)
project(libktx)

set(TARGET "libktx.gl")

set(proj_inc_dir ${CMAKE_SOURCE_DIR}/third_party/ktx/include)
set(proj_src_dir ${CMAKE_SOURCE_DIR}/third_party/ktx/lib)

list(APPEND libktx_gl__c_srcs
  "${proj_src_dir}/checkheader.c"
  "${proj_src_dir}/errstr.c"
  "${proj_src_dir}/hashtable.c"
  "${proj_src_dir}/ktxfilestream.c"
  "${proj_src_dir}/ktxmemstream.c"
  "${proj_src_dir}/loader.c"
  "${proj_src_dir}/swap.c"
  "${proj_src_dir}/writer.c")

list(APPEND libktx_gl__cxx_srcs
  "${proj_src_dir}/etcdec.cxx"
  "${proj_src_dir}/etcunpack.cxx")

list(APPEND libktx_gl__other_srcs
  "${proj_inc_dir}/ktx.h"
  "${proj_src_dir}/gl_funcptrs.h"
  "${proj_src_dir}/gles1_funcptrs.h"
  "${proj_src_dir}/gles2_funcptrs.h"
  "${proj_src_dir}/gles3_funcptrs.h"
  "${proj_src_dir}/ktxfilestream.h"
  "${proj_src_dir}/ktxint.h"
  "${proj_src_dir}/ktxmemstream.h"
  "${proj_src_dir}/ktxstream.h"
  "${proj_src_dir}/uthash.h")

link_directories()

add_library(ktx_gl STATIC ${libktx_gl__c_srcs} ${libktx_gl__cxx_srcs} ${libktx_gl__other_srcs})
set_target_properties(ktx_gl PROPERTIES EXCLUDE_FROM_ALL "FALSE")
set_source_files_properties(${libktx_gl__other_srcs} PROPERTIES HEADER_FILE_ONLY "TRUE")

#set_target_properties(libktx_gl PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${builddir}/lib.${TOOLSET}")
set_target_properties(ktx_gl PROPERTIES PREFIX "lib")
set_target_properties(ktx_gl PROPERTIES LIBRARY_OUTPUT_NAME "ktx.gl")
set_target_properties(ktx_gl PROPERTIES SUFFIX ".a")
set_source_files_properties(${builddir}/lib.${TOOLSET}/libktx.gl.a PROPERTIES GENERATED "TRUE")

list(APPEND libktx_gl__include_dirs
  "${proj_inc_dir}"
  "${CMAKE_SOURCE_DIR}/third_party/ktx/other_include")
  
set_property(TARGET ktx_gl APPEND PROPERTY INCLUDE_DIRECTORIES ${libktx_gl__include_dirs})
set_target_properties(ktx_gl PROPERTIES COMPILE_DEFINITIONS "KTX_OPENGL=1;NDEBUG;")
set_target_properties(ktx_gl PROPERTIES COMPILE_FLAGS "-O3 -fPIC ")

add_dependencies(ktx_gl
  libgl
)

target_link_libraries(ktx_gl
  -lGL
)
