#pragma once

#include "xray/xray.hpp"
#include <vulkan/vulkan_core.h>
#include "vulkan.dynamic.dispatched.functions.hpp"

namespace xray::rendering {

struct vkfn {
#define PFN_LIST_ENTRY(fnproto, name, tag) static fnproto name;
	FUNCTION_POINTERS_LIST
#undef PFN_LIST_ENTRY
};

}  // namespace xray::rendering
