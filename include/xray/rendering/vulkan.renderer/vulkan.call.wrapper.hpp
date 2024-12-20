#pragma once

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

#include <vulkan/vulkan.h>

#include "xray/base/logger.hpp"

#define WRAP_VULKAN_FUNC(vkfunc, ...)                                                                                  \
    (xray::rendering::vk_function_call(__FILE__, __LINE__, #vkfunc, vkfunc, ##__VA_ARGS__))

namespace xray::rendering {

std::string_view
format_vk_func_fail(const char* file, const int32_t line, const char* vkfunc_name, const VkResult result);

template<typename VkFunction, typename... FunctionParams>
auto inline vk_function_call(const char* file,
                             const int32_t line,
                             const char* vkfunc_name,
                             VkFunction vk_func,
                             FunctionParams... params) -> std::invoke_result_t<decltype(vk_func), FunctionParams...>
{
    if constexpr (std::is_same_v<void, std::invoke_result_t<decltype(vk_func), FunctionParams...>>) {
        vk_func(std::forward<FunctionParams>(params)...);
    } else {
        const VkResult result = vk_func(std::forward<FunctionParams>(params)...);
        if (result != VkResult::VK_SUCCESS) {
            XR_LOG_ERR("{}", format_vk_func_fail(file, line, vkfunc_name, result));
        }

        return result;
    }
}

} // namespace xray::rendering
