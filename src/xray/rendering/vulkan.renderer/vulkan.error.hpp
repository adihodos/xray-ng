#pragma once

#include "xray/xray.hpp"

#include <span>
#include <tl/expected.hpp>

// #if defined(__cpp_lib_source_location)
// #include <source_location>
// #endif

namespace xray::rendering {

struct VulkanError {
	I32 err_code;
	I32 line;
	const char* file;
	const char* function;

// #if defined(__cpp_lib_source_location)
	// VulkanError(const I32 err_code, const std::source_location src_loc = std::source_location::current());
// #endif

	VulkanError(const I32 vk_result, const char* file, const char* funcname, const I32 line);
};

// #if defined(__cpp_lib_source_location)
// #define XR_MAKE_VULKAN_ERROR(vk_err_code) (tl::unexpected{VulkanError{vk_err_code}})
// #else
#define XR_MAKE_VULKAN_ERROR(vk_err_code) \
	(tl::unexpected{xray::rendering::VulkanError{vk_err_code, __FILE__, XRAY_FUNCTION_NAME, __LINE__}})
// #endif

const char* vk_result_to_string(const I32 vk_result);

#define XR_VK_CHECK_RESULT(vkres)               \
	do {                                        \
		if (vkres != VK_SUCCESS) {              \
			return XR_MAKE_VULKAN_ERROR(vkres); \
		}                                       \
	} while (0)

#define XR_VK_PROPAGATE_ERROR(e)                       \
	do {                                               \
		if (!e) return tl::make_unexpected(e.error()); \
	} while (0)

#define XR_VK_COR_PROPAGATE_ERROR(e)                      \
	do {                                                  \
		if (!e) co_return tl::make_unexpected(e.error()); \
	} while (0)

void format_to(std::span<char> output, const VulkanError& e);

}  // namespace xray::rendering
