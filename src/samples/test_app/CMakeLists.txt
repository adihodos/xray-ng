project(test-app)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/src/samples/test_app")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/samples/test_app")

set(project_sources
    ${proj_src_dir}/tests.cc
    ${proj_src_dir}/catch_main.cc
    )

# if (WIN32)
#     set(sys-libs dxgi user32)
# endif()    

add_executable(test-app ${project_sources})
target_link_libraries(test-app 
    xray-base xray-ui xray-opengl-renderer ${SYS_DEP_LIBS}
    )
