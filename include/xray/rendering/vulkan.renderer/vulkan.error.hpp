#pragma once

#include "xray/xray.hpp"
#include <cstdint>
#if defined(__cpp_lib_source_location)
#include <source_location>
#endif

#include <fmt/core.h>

namespace xray::rendering {

struct VulkanError
{
    int32_t err_code;
    uint32_t line;
    const char* file;
    const char* function;

#if defined(__cpp_lib_source_location)
    VulkanError(const int32_t err_code, const std::source_location src_loc = std::source_location::current());
#endif

    VulkanError(const int32_t vk_result, const char* file, const char* funcname, const uint32_t line);
};

#if defined(__cpp_lib_source_location)
#define XR_MAKE_VULKAN_ERROR(vk_err_code) (tl::unexpected{ VulkanError{ vk_err_code } })
#else
#define XR_MAKE_VULKAN_ERROR(vk_err_code)                                                                              \
    (tl::unexpected{ VulkanError{ vk_err_code, __FILE__, XRAY_FUNCTION_NAME, __LINE__ } })
#endif

std::string vk_result_to_string(const int32_t vk_result);

} // namespace xray::rendering
  //

namespace fmt {
template<>
struct formatter<xray::rendering::VulkanError>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename Context>
    constexpr auto format(xray::rendering::VulkanError const& vk_err, Context& ctx) const
    {
        return fmt::format_to(ctx.out(),
                              "VulkanError = {{\n\t.error = {:#0x} ({}),\n\t.location {}:{}\n\t.function = {}\n}}",
                              vk_err.err_code,
                              xray::rendering::vk_result_to_string(vk_err.err_code),
                              vk_err.file,
                              vk_err.line,
                              vk_err.function);
    }
};
} // namespace fmt
