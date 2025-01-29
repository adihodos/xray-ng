cmake_minimum_required(VERSION 3.20.0 FATAL_ERROR)
project(noise)

set(proj_inc_dir ${CMAKE_SOURCE_DIR}/third_party/libnoise/include)
set(proj_src_dir ${CMAKE_SOURCE_DIR}/third_party/libnoise/src)

list(APPEND SOURCES "${proj_src_dir}/latlon.cpp" "${proj_src_dir}/noisegen.cpp"
     "${proj_src_dir}/noiseutils.cpp")

list(APPEND SOURCES_MODEL "${proj_src_dir}/model/cylinder.cpp"
     "${proj_src_dir}/model/line.cpp" "${proj_src_dir}/model/plane.cpp"
     "${proj_src_dir}/model/sphere.cpp")

list(
  APPEND
  SOURCES_MODULE
  "${proj_src_dir}/module/abs.cpp"
  "${proj_src_dir}/module/add.cpp"
  "${proj_src_dir}/module/billow.cpp"
  "${proj_src_dir}/module/blend.cpp"
  "${proj_src_dir}/module/cache.cpp"
  "${proj_src_dir}/module/checkerboard.cpp"
  "${proj_src_dir}/module/clamp.cpp"
  "${proj_src_dir}/module/const.cpp"
  "${proj_src_dir}/module/curve.cpp"
  "${proj_src_dir}/module/cylinders.cpp"
  "${proj_src_dir}/module/displace.cpp"
  "${proj_src_dir}/module/exponent.cpp"
  "${proj_src_dir}/module/invert.cpp"
  "${proj_src_dir}/module/max.cpp"
  "${proj_src_dir}/module/min.cpp"
  "${proj_src_dir}/module/modulebase.cpp"
  "${proj_src_dir}/module/multiply.cpp"
  "${proj_src_dir}/module/perlin.cpp"
  "${proj_src_dir}/module/power.cpp"
  "${proj_src_dir}/module/ridgedmulti.cpp"
  "${proj_src_dir}/module/rotatepoint.cpp"
  "${proj_src_dir}/module/scalebias.cpp"
  "${proj_src_dir}/module/select.cpp"
  "${proj_src_dir}/module/spheres.cpp"
  "${proj_src_dir}/module/terrace.cpp"
  "${proj_src_dir}/module/translatepoint.cpp"
  "${proj_src_dir}/module/turbulence.cpp"
  "${proj_src_dir}/module/voronoi.cpp")

add_library(noise STATIC ${SOURCES} ${SOURCES_MODEL} ${SOURCES_MODULE})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(
  ${PROJECT_NAME} PUBLIC $<INSTALL_INTERFACE:include>
                         $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  # PRIVATE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

set_target_properties(noise PROPERTIES LIBRARY_OUTPUT_NAME "noise")
set_target_properties(noise PROPERTIES SUFFIX ".a")
