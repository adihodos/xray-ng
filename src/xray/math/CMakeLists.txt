project(xray-math)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/math")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/math")

set(project_sources
    ${proj_inc_dir}/constants.hpp
    ${proj_inc_dir}/handedness.hpp
    ${proj_inc_dir}/math_base.hpp
    ${proj_inc_dir}/math_std.hpp
    ${proj_inc_dir}/projection.hpp

    ${proj_inc_dir}/quaternion.hpp
    ${proj_inc_dir}/quaternion_math.hpp

    ${proj_inc_dir}/rectangle.hpp
    ${proj_inc_dir}/scalar2.hpp
    ${proj_inc_dir}/scalar2x3.hpp
    ${proj_inc_dir}/scalar2x3_math.hpp
    ${proj_inc_dir}/scalar2_math.hpp
    ${proj_inc_dir}/scalar3.hpp
    ${proj_inc_dir}/scalar3x3.hpp
    ${proj_inc_dir}/scalar3x3_math.hpp
    ${proj_inc_dir}/scalar3x3_string_cast.hpp
    ${proj_inc_dir}/scalar3_math.hpp
    ${proj_inc_dir}/scalar3_string_cast.hpp
    ${proj_inc_dir}/scalar4.hpp
    ${proj_inc_dir}/scalar4x4.hpp
    ${proj_inc_dir}/scalar4x4_math.hpp
    ${proj_inc_dir}/scalar4x4_string_cast.hpp
    ${proj_inc_dir}/scalar4_math.hpp
    ${proj_inc_dir}/stdc_base.hpp
    ${proj_inc_dir}/transforms_r2.hpp
    ${proj_inc_dir}/transforms_r3.hpp
    ${proj_inc_dir}/transforms_r4.hpp
    ${proj_inc_dir}/vector_type_tag.hpp

    ${proj_inc_dir}/objects/aabb3.hpp
    ${proj_inc_dir}/objects/aabb3_math.hpp
    ${proj_inc_dir}/objects/rectangle.hpp
    ${proj_inc_dir}/objects/sphere.hpp
    ${proj_inc_dir}/objects/sphere_math.hpp

    ${proj_inc_dir}/objects/plane.hpp
)

source_group(math FILES ${project_sources})

#add_library(xray-base STATIC ${project_sources})
#target_link_libraries(xray-base ${platform_libs} ${FMT_LIBRARY} libconfig)
