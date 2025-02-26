project(xray-math)

set(project_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/math")

set(project_files
    ${project_inc_dir}/bidimensional_space_traits.hpp
    ${project_inc_dir}/handedness.hpp
    ${project_inc_dir}/objects/line.hpp
    ${project_inc_dir}/objects/line2.hpp
    ${project_inc_dir}/objects/line3.hpp
    ${project_inc_dir}/objects/oriented_bounding_box.hpp
    ${project_inc_dir}/objects/plane.hpp
    ${project_inc_dir}/objects/ray.hpp
    ${project_inc_dir}/objects/ray2.hpp
    ${project_inc_dir}/objects/ray3.hpp
    ${project_inc_dir}/objects/rectangle.hpp
    ${project_inc_dir}/objects/aabb3_math.hpp
    ${project_inc_dir}/objects/sphere.hpp
    ${project_inc_dir}/objects/sphere_math.hpp
    ${project_inc_dir}/objects/aabb3.hpp
    ${project_inc_dir}/scalar2x3.hpp
    ${project_inc_dir}/scalar2x3_math.hpp
    ${project_inc_dir}/scalar3_string_cast.hpp
    ${project_inc_dir}/scalar3x3.hpp
    ${project_inc_dir}/scalar3x3_math.hpp
    ${project_inc_dir}/scalar3x3_string_cast.hpp
    ${project_inc_dir}/space_traits.hpp
    ${project_inc_dir}/stdc_base.hpp
    ${project_inc_dir}/swizzle.hpp
    ${project_inc_dir}/transforms_r2.hpp
    ${project_inc_dir}/transforms_r3.hpp
    ${project_inc_dir}/tridimensional_space_traits.hpp
    ${project_inc_dir}/vector_type_tag.hpp
    ${project_inc_dir}/math_base.hpp
    ${project_inc_dir}/quaternion.hpp
    ${project_inc_dir}/quaternion_math.hpp
    ${project_inc_dir}/quaternion_string_cast.hpp
    ${project_inc_dir}/scalar4x4.hpp
    ${project_inc_dir}/scalar4x4_string_cast.hpp
    ${project_inc_dir}/scalar4x4_math.hpp
    ${project_inc_dir}/constants.hpp
    ${project_inc_dir}/orientation.hpp
    ${project_inc_dir}/scalar2_string_cast.hpp
    ${project_inc_dir}/scalar3.hpp
    ${project_inc_dir}/scalar3_math.hpp
    ${project_inc_dir}/scalar4.hpp
    ${project_inc_dir}/scalar4_math.hpp
    ${project_inc_dir}/serialization/parser.orientation.hpp
    ${project_inc_dir}/serialization/parser.quaternion.hpp
    ${project_inc_dir}/serialization/parser.scalar2.hpp
    ${project_inc_dir}/serialization/parser.scalar3.hpp
    ${project_inc_dir}/serialization/parser.scalar4.hpp
    ${project_inc_dir}/transforms_r4.hpp
    ${project_inc_dir}/projection.hpp
    ${project_inc_dir}/math.units.hpp
    ${project_inc_dir}/scalar2.hash.hpp
    ${project_inc_dir}/scalar2.hpp
    ${project_inc_dir}/math_std.hpp
    ${project_inc_dir}/scalar2_math.hpp
    ${project_inc_dir}/axis.aligned.bounding.box.r2.hpp
    ${project_inc_dir}/axis.aligned.bounding.box.hpp
    ${project_inc_dir}/quadtree.hpp)

add_library(${PROJECT_NAME} INTERFACE ${project_files})

target_include_directories(
  ${PROJECT_NAME} INTERFACE $<INSTALL_INTERFACE:include>
                            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)
target_link_libraries(${PROJECT_NAME} INTERFACE reflectcpp
                                                strong_type::strong_type)

source_group(xray::math FILES ${project_files})

# add_test(NAME MathTests COMMAND XrayMathTests)

# Add all the ADD_TEST for each test foreach(test ${TestsToRun})
# get_filename_component(TName ${test} NAME_WE) add_test(NAME ${TName} COMMAND
# CommonCxxTests ${TName}) endforeach()
