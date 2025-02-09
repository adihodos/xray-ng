project(xray-base)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/base")
set(proj_src_dir "${PROJECT_SOURCE_DIR}")

set(project_headers
    ${proj_inc_dir}/base_fwd.hpp
    ${proj_inc_dir}/xray.misc.hpp
    ${proj_src_dir}/xray.misc.cc
    ${proj_inc_dir}/array_dimension.hpp
    ${proj_inc_dir}/app_config.hpp
    ${proj_inc_dir}/basic_timer.hpp
    ${proj_inc_dir}/config_settings.hpp
    ${proj_inc_dir}/containers/fixed_vector.hpp
    ${proj_inc_dir}/delegate.hpp
    ${proj_inc_dir}/fnv_hash.hpp
    ${proj_inc_dir}/maybe.hpp
    ${proj_inc_dir}/nothing.hpp
    ${proj_inc_dir}/pod_zero.hpp
    ${proj_inc_dir}/scoped_guard.hpp
    ${proj_inc_dir}/scoped_handle.hpp
    ${proj_inc_dir}/variant.helpers.hpp
    ${proj_inc_dir}/syscall_wrapper.hpp
    ${proj_inc_dir}/unique_handle.hpp
    ${proj_inc_dir}/logger.hpp
    ${proj_inc_dir}/shims/attribute/basic_path.hpp
    ${proj_inc_dir}/unique_pointer.hpp
    ${proj_inc_dir}/perf_stats_collector.hpp
    ${proj_inc_dir}/windows/com_ptr.hpp
    ${proj_inc_dir}/windows/handle_traits.hpp
    ${proj_inc_dir}/file_system_watcher.hpp
    ${proj_inc_dir}/concurrency/spin.mutex.hpp
    ${proj_inc_dir}/serialization/rfl.libconfig/config.load.hpp
    ${proj_inc_dir}/serialization/rfl.libconfig/config.parser.hpp
    ${proj_inc_dir}/serialization/rfl.libconfig/config.reader.hpp
    ${proj_inc_dir}/serialization/rfl.libconfig/config.save.hpp
    ${proj_inc_dir}/serialization/rfl.libconfig/config.writer.hpp)

set(project_sources
    ${proj_src_dir}/app_config.cc
    ${proj_src_dir}/config_settings.cc
    ${proj_src_dir}/logger.cc
    ${proj_src_dir}/perf_stats_collector.cc
    ${proj_src_dir}/file_system_watcher.cc
    ${proj_src_dir}/serialization/rfl.libconfig/config.writer.cc)

add_library(${PROJECT_NAME} STATIC ${project_sources} ${project_headers})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<INSTALL_INTERFACE:include>
         $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  PRIVATE)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC $<$<PLATFORM_ID:Linux>:XRAY_OS_IS_POSIX_FAMILY>
  PRIVATE $<$<CONFIG:Debug>:XRAY_IS_DEBUG_BUILD>)

target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC fmt::fmt spdlog::spdlog $<IF:$<PLATFORM_ID:Windows>,libconfig,config>
         reflectcpp swl-variant
  PRIVATE itlib::itlib joboccara::pipes)
