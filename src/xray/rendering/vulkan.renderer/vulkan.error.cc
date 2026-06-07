#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"

#include <vulkan/vulkan.h>

#include "xray/base/xray.fmt.hpp"

xray::rendering::VulkanError::VulkanError(
	const I32 vk_result, const char* e_file, const char* e_funcname, const I32 e_line
)
	: err_code{vk_result}, line{e_line}, file{e_file}, function{e_funcname} {}

#if defined(__cpp_lib_source_location)
xray::rendering::VulkanError::VulkanError(const I32 vk_result, const std::source_location src_loc)
	: xray::rendering::VulkanError{vk_result, src_loc.file_name(), src_loc.function_name(), (I32)src_loc.line()} {}
#endif

const char* xray::rendering::vk_result_to_string(const I32 vk_result) { return "TODO: fix this"; }

void xray::rendering::format_to(std::span<char> output, const VulkanError& vk_err) {
	base::format_to_n(
		output,
		"VulkanError = {{\n\t.error = %#08x (%s),\n\t.location %s:%d\n\t.function = %s\n}}",
		vk_err.err_code,
		xray::rendering::vk_result_to_string(vk_err.err_code),
		vk_err.file,
		vk_err.line,
		vk_err.function
	);
}
