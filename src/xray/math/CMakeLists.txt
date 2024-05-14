project(xray-math)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/math")

set(project_sources
    ${proj_inc_dir}/constants.hpp
    ${proj_inc_dir}/handedness.hpp
    ${proj_inc_dir}/math_base.hpp
    ${proj_inc_dir}/math_std.hpp
    ${proj_inc_dir}/projection.hpp
    ${proj_inc_dir}/swizzle.hpp

    ${proj_inc_dir}/quaternion.hpp
    ${proj_inc_dir}/quaternion_math.hpp

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

    ${proj_inc_dir}/objects/aabb3.hpp
    ${proj_inc_dir}/objects/ray3.hpp
    ${proj_inc_dir}/objects/ray.hpp
    ${proj_inc_dir}/objects/plane.hpp
    ${proj_inc_dir}/objects/aabb3_math.hpp
    ${proj_inc_dir}/objects/sphere_math.hpp
    ${proj_inc_dir}/objects/line3.hpp
    ${proj_inc_dir}/objects/line.hpp
    ${proj_inc_dir}/objects/rectangle.hpp
    ${proj_inc_dir}/objects/line2.hpp
    ${proj_inc_dir}/objects/sphere.hpp
    ${proj_inc_dir}/objects/ray2.hpp    
)

add_library(${PROJECT_NAME} INTERFACE ${project_sources})

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_compile_features(
  ${PROJECT_NAME}
  INTERFACE cxx_std_20
)

source_group(math FILES ${project_sources})
