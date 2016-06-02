project(xray-base)

set(proj_inc_dir "${CMAKE_SOURCE_DIR}/include/xray/base")
set(proj_src_dir "${CMAKE_SOURCE_DIR}/src/xray/base")

set(project_sources
    ${proj_inc_dir}/array_dimension.hpp
    ${proj_inc_dir}/app_config.hpp
    ${proj_src_dir}/app_config.cc
    ${proj_inc_dir}/basic_timer.hpp
    ${proj_inc_dir}/config_settings.hpp
    ${proj_src_dir}/config_settings.cc
    ${proj_inc_dir}/debug_output.hpp
    ${proj_inc_dir}/delegate_list.hpp
    ${proj_inc_dir}/fast_delegate.hpp
    ${proj_inc_dir}/fnv_hash.hpp
    ${proj_inc_dir}/maybe.hpp
    ${proj_inc_dir}/nothing.hpp
    ${proj_inc_dir}/pod_zero.hpp
    ${proj_inc_dir}/scoped_guard.hpp
    ${proj_inc_dir}/scoped_handle.hpp
    ${proj_inc_dir}/syscall_wrapper.hpp
    ${proj_inc_dir}/unique_handle.hpp
    ${proj_inc_dir}/logger.hpp
    ${proj_src_dir}/logger.cc
    ${proj_inc_dir}/unique_pointer.hpp
    ${proj_inc_dir}/dbg/debug_ext.hpp
    ${proj_src_dir}/dbg/debug_ext.cc
    ${proj_inc_dir}/perf_stats_collector.hpp
    ${proj_src_dir}/perf_stats_collector.cc
    ${proj_inc_dir}/windows/com_ptr.hpp
    ${proj_inc_dir}/windows/handle_traits.hpp)

if (WIN32)
    set(platform_libs pdh)
endif()

add_library(xray-base STATIC ${project_sources})
target_link_libraries(xray-base ${platform_libs} ${FMT_LIBRARY} libconfig)
