add_subdirectory(base)
add_subdirectory(math)
add_subdirectory(rendering)
add_subdirectory(scene)

if (NOT XRAY_USE_DIRECTX10)
    add_subdirectory(ui)
endif()    