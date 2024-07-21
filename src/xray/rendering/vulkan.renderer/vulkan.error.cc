#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"

#include <fmt/core.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>

xray::rendering::VulkanError::VulkanError(const int32_t vk_result,
                                          const char* e_file,
                                          const char* e_funcname,
                                          const uint32_t e_line)
    : err_code{ vk_result }
    , line{ e_line }
    , file{ e_file }
    , function{ e_funcname }
{
}

#if defined(__cpp_lib_source_location)
xray::rendering::VulkanError::VulkanError(const int32_t vk_result, const std::source_location src_loc)
    : xray::rendering::VulkanError{ vk_result, src_loc.file_name(), src_loc.function_name(), src_loc.line() }
{
}
#endif

std::string
xray::rendering::vk_result_to_string(const int32_t vk_result)
{
    return vk::to_string(static_cast<vk::Result>(vk_result));
}
