project(basic-dx11-app)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/src/samples/directx11")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/samples/directx11")

include_directories(${proj_inc_dir})

set(project_sources
    ${proj_src_dir}/main.cc
    ${proj_inc_dir}/cap1/test_object.hpp
    ${proj_src_dir}/cap1/test_object.cc
    )

set(shader_files
    ${proj_src_dir}/shaders/cap1/vert.hlsl
    ${proj_src_dir}/shaders/cap1/pixel.hlsl
    )

source_group(shaders FILES ${shader_files})
add_executable(basic-dx11-app WIN32 ${project_sources})

target_link_libraries(basic-dx11-app
    xray-base
    xray-rendering
    xray-scene
    xray-directx-renderer
    xray-ui
    xray-imgui
    libconfig
    ${ASSIMP_LIBRARY}
    ${SYS_DEP_LIBS}
    )

add_custom_target(COPY_TBB ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${TBB_BIN_LIB} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(COPY_ASSIMP ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSIMP_BIN_LIBRARY} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_custom_target(COPY_SHADERS ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

add_custom_target(COPY_CONFIG_FILES ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config)

file( WRITE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user"
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>     \
    <Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">
    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\">
        <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
    </PropertyGroup>
    <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">
        <LocalDebuggerWorkingDirectory>$(OutDir)</LocalDebuggerWorkingDirectory>
        <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
    </PropertyGroup>
    </Project>")