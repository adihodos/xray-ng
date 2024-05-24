#pragma once

#include "xray/xray.hpp"
#include <vulkan/vulkan.h>

namespace xray::rendering {

#define FUNCTION_POINTERS_LIST                                                                                         \
    PFN_LIST_ENTRY(PFN_vkCreateDebugUtilsMessengerEXT, CreateDebugUtilsMessengerEXT)                                   \
    PFN_LIST_ENTRY(PFN_vkDestroyDebugUtilsMessengerEXT, DestroyDebugUtilsMessengerEXT)

struct vkfn
{
#define PFN_LIST_ENTRY(fnproto, name) static fnproto name;
    FUNCTION_POINTERS_LIST
#undef PFN_LIST_ENTRY
};

} // namespace xray::rendering
