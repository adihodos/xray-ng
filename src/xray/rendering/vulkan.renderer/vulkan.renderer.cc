#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <vector>

#include <fmt/core.h>
#include <itlib/small_vector.hpp>
#include <tl/optional.hpp>
#include <swl/variant.hpp>
#include <mio/mmap.hpp>
#include <Lz/Lz.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_to_string.hpp>

#if defined(XRAY_OS_IS_WINDOWS)
#include <vulkan/vulkan_win32.h>
#endif

#include "xray/base/fnv_hash.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/rangeless/fn.hpp"
#include "xray/base/variant_visitor.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatch.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pretty.print.hpp"

using namespace xray::base;
using namespace std;

namespace fn = rangeless::fn;
using fn::operators::operator%;
using fn::operators::operator%=;

template<typename T>
using small_vec_2 = itlib::small_vector<T, 2>;

template<typename T>
using small_vec_4 = itlib::small_vector<T, 4>;

template<typename T>
struct variant_type_is_not_handled
{};

template<typename FwdIter, typename BinaryPredicate>
tl::optional<size_t>
r_find_pos(FwdIter first, FwdIter last, BinaryPredicate bp) noexcept
{
    const auto itr = find_if(first, last, bp);
    if (itr != last) {
        return tl::optional<size_t>{ distance(first, itr) };
    }
    return tl::nullopt;
}

template<typename Container, typename BinaryPredicate>
tl::optional<size_t>
r_find_pos(const Container& c, BinaryPredicate bp) noexcept
{
    return r_find_pos(cbegin(c), cend(c), bp);
}

template<typename ElementType, typename BinaryPredicate>
tl::optional<size_t>
r_find_pos(std::span<ElementType> s, BinaryPredicate bp) noexcept
{
    return r_find_pos(cbegin(s), cend(s), bp);
}

namespace std {
template<>
struct hash<VkExtensionProperties>
{
    size_t operator()(const VkExtensionProperties& e) const noexcept { return FNV::fnv1a_unrolled<2>(&e, sizeof(e)); }
};

}

inline bool
operator==(const VkExtensionProperties& ea, const VkExtensionProperties& eb) noexcept
{
    return strcmp(ea.extensionName, eb.extensionName) == 0 && ea.specVersion == eb.specVersion;
}

namespace xray::rendering {

#define PFN_LIST_ENTRY(fnproto, name) fnproto vkfn::name{ nullptr };
FUNCTION_POINTERS_LIST
#undef PFN_LIST_ENTRY

std::string_view
format_vk_func_fail(const char* file, const int32_t line, const char* vkfunc_name, const VkResult result)
{
    thread_local static array<char, 2048> scratch_buffer;
    const auto [itr, cch] = fmt::format_to_n(scratch_buffer.data(),
                                             scratch_buffer.size(),
                                             "[{}:{}] vulkan api: {} failed, error {:#0x} ({})",
                                             file,
                                             line,
                                             vkfunc_name,
                                             static_cast<uint32_t>(result),
                                             vk::to_string(static_cast<vk::Result>(result)));

    scratch_buffer[cch > 0 ? cch : 0] = 0;
    return string_view{ scratch_buffer.data(), static_cast<size_t>(cch) };
}

VkBool32
log_vk_debug_output(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                    void* pUserData)
{
    array<char, 2048> msg_buf;
    const auto [iter, cch] = fmt::format_to_n(msg_buf.data(),
                                              msg_buf.size() - 1,
                                              "[vulkan]: {} :: {:#x} - {}",
                                              pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "unknown",
                                              pCallbackData->messageIdNumber,
                                              pCallbackData->pMessage ? pCallbackData->pMessage : "unknown");

    if (cch > 0) {
        msg_buf[cch] = 0;

        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            XR_LOG_WARN("{}", msg_buf.data());
        } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            XR_LOG_ERR("{}", msg_buf.data());
        } else {
            XR_LOG_INFO("{}", msg_buf.data());
        }
    }

    return VK_FALSE;
}

small_vec_4<VkExtensionProperties>
enumerate_instance_extensions()
{
    uint32_t exts_count{};
    small_vec_4<VkExtensionProperties> exts_props{};

    vkEnumerateInstanceExtensionProperties(nullptr, &exts_count, nullptr);
    if (exts_count != 0) {
        exts_props.resize(exts_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &exts_count, exts_props.data());
    }

    return exts_props;
}

itlib::small_vector<VkPhysicalDevice, 4>
enumerate_physical_devices(VkInstance instance)
{
    uint32_t phys_devs_count{};
    itlib::small_vector<VkPhysicalDevice, 4> phys_devices{};

    vkEnumeratePhysicalDevices(instance, &phys_devs_count, nullptr);
    if (phys_devs_count) {
        phys_devices.resize(phys_devs_count);
        vkEnumeratePhysicalDevices(instance, &phys_devs_count, phys_devices.data());
    }

    return phys_devices;
}

detail::PhysicalDeviceData::PhysicalDeviceData(VkPhysicalDevice dev)
    : device{ dev }
{
    properties.base.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.base.pNext = &properties.vk11;

    properties.vk11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    properties.vk11.pNext = &properties.vk12;

    properties.vk12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    properties.vk12.pNext = &properties.vk13;

    properties.vk13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    properties.vk13.pNext = nullptr;

    features.base.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.base.pNext = &features.vk11;

    features.vk11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features.vk11.pNext = &features.vk12;

    features.vk12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features.vk12.pNext = &features.vk13;

    features.vk13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    features.vk13.pNext = nullptr;

    vkGetPhysicalDeviceFeatures2(device, &features.base);
    vkGetPhysicalDeviceProperties2(device, &properties.base);
    vkGetPhysicalDeviceMemoryProperties(device, &memory);

    uint32_t queue_fams{};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_fams, nullptr);
    queue_props.resize(queue_fams);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_fams, queue_props.data());
}

detail::PhysicalDeviceData::PhysicalDeviceData(const PhysicalDeviceData& rhs)
{
    memcpy(&this->features, &rhs.features, sizeof(this->features));
    memcpy(&this->properties, &rhs.properties, sizeof(this->properties));
    memcpy(&this->memory, &rhs.memory, sizeof(this->memory));

    features.base.pNext = &features.vk11;
    features.vk11.pNext = &features.vk12;
    features.vk12.pNext = &features.vk13;

    properties.base.pNext = &properties.vk11;
    properties.vk11.pNext = &properties.vk12;
    properties.vk12.pNext = &properties.vk13;
    queue_props = rhs.queue_props;
    device = rhs.device;
}

detail::PhysicalDeviceData&
detail::PhysicalDeviceData::operator=(const PhysicalDeviceData& rhs)
{
    if (this != &rhs) {
        memcpy(&this->features, &rhs.features, sizeof(this->features));
        memcpy(&this->properties, &rhs.properties, sizeof(this->properties));
        memcpy(&this->memory, &rhs.memory, sizeof(this->memory));

        features.base.pNext = &features.vk11;
        features.vk11.pNext = &features.vk12;
        features.vk12.pNext = &features.vk13;

        properties.base.pNext = &properties.vk11;
        properties.vk11.pNext = &properties.vk12;
        properties.vk12.pNext = &properties.vk13;
        queue_props = rhs.queue_props;
        device = rhs.device;
    }

    return *this;
}

struct PresentToWindowSurface
{
    WindowPlatformData window_data;
    xrUniqueVkSurfaceKHR surface;
};

struct DisplayData
{
    VkDisplayPropertiesKHR properties;
    vector<VkDisplayModePropertiesKHR> display_modes;
};

struct DisplayPlaneData
{
    uint32_t index;
    VkDisplayPlanePropertiesKHR properties;
    vector<VkDisplayKHR> supported_displays;
};

struct PresentToDisplaySurface
{
    DisplayData display_data;
    DisplayPlaneData display_plane_data;
    VkDisplayPlaneCapabilitiesKHR display_plane_caps;
    xrUniqueVkSurfaceKHR display_surface;
};

using PresentToSurface = swl::variant<PresentToWindowSurface, PresentToDisplaySurface>;

tl::optional<PresentToDisplaySurface>
check_display_presentation_support(span<const char*> extensions_list, VkPhysicalDevice pd, VkInstance instance)
{
    const bool has_display_ext =
        ranges::any_of(extensions_list, [](const char* e) { return strcmp(e, VK_KHR_DISPLAY_EXTENSION_NAME) == 0; });

    if (!has_display_ext) {
        return tl::nullopt;
    }

    //
    // get list of attached displays
    const vector<DisplayData> attached_displays{ [pd]() {
        uint32_t count{};
        WRAP_VULKAN_FUNC(vkGetPhysicalDeviceDisplayPropertiesKHR, pd, &count, nullptr);

        vector<VkDisplayPropertiesKHR> dpys;
        dpys.resize(count);
        WRAP_VULKAN_FUNC(vkGetPhysicalDeviceDisplayPropertiesKHR, pd, &count, dpys.data());

        vector<DisplayData> attached_displays{};
        for (const VkDisplayPropertiesKHR& dpy_props : dpys) {
            uint32_t modes_count{};
            WRAP_VULKAN_FUNC(vkGetDisplayModePropertiesKHR, pd, dpy_props.display, &modes_count, nullptr);

            if (!modes_count)
                continue;

            vector<VkDisplayModePropertiesKHR> display_modes{ modes_count };
            WRAP_VULKAN_FUNC(vkGetDisplayModePropertiesKHR, pd, dpy_props.display, &modes_count, display_modes.data());

            attached_displays.emplace_back(dpy_props, std::move(display_modes));
        }

        return attached_displays;
    }() };

    if (attached_displays.empty())
        return tl::nullopt;

    for (const DisplayData& dp : attached_displays) {
        XR_LOG_INFO("display : {}, physical dimension {}, physical resolution {}",
                    dp.properties.displayName,
                    dp.properties.physicalResolution,
                    dp.properties.physicalDimensions);

        for (const VkDisplayModePropertiesKHR& disp_mode : dp.display_modes) {
            XR_LOG_INFO("mode:: refresh rate {} Hz, visible region {}",
                        disp_mode.parameters.refreshRate,
                        disp_mode.parameters.visibleRegion);
        }
    }

    //
    // get list of display planes for this device
    const vector<DisplayPlaneData> display_plane_data{ [pd,
                                                        display_properties = &attached_displays.front().properties]() {
        uint32_t dpy_plane_count{};
        WRAP_VULKAN_FUNC(vkGetPhysicalDeviceDisplayPlanePropertiesKHR, pd, &dpy_plane_count, nullptr);

        vector<VkDisplayPlanePropertiesKHR> display_plane_properties{ dpy_plane_count };
        WRAP_VULKAN_FUNC(
            vkGetPhysicalDeviceDisplayPlanePropertiesKHR, pd, &dpy_plane_count, display_plane_properties.data());

        vector<DisplayPlaneData> display_plane_data{};
        for (uint32_t plane_index = 0; plane_index < static_cast<uint32_t>(display_plane_properties.size());
             ++plane_index) {
            const VkDisplayPlanePropertiesKHR& dpp = display_plane_properties[plane_index];

            uint32_t supported_displays_count{};
            WRAP_VULKAN_FUNC(
                vkGetDisplayPlaneSupportedDisplaysKHR, pd, plane_index, &supported_displays_count, nullptr);
            vector<VkDisplayKHR> supported_displays{ supported_displays_count };
            WRAP_VULKAN_FUNC(vkGetDisplayPlaneSupportedDisplaysKHR,
                             pd,
                             plane_index,
                             &supported_displays_count,
                             supported_displays.data());

            if (supported_displays_count) {
                display_plane_data.emplace_back(plane_index, dpp, std::move(supported_displays));
            }
        }

        return display_plane_data;
    }() };

    if (display_plane_data.empty())
        return tl::nullopt;

    //
    // find the best display plane
    struct DisplayDataDisplayPlaneCapsModeIndex
    {
        size_t display_data_idx;
        size_t display_plane_idx;
        size_t display_mode_idx;
        VkDisplayPlaneCapabilitiesKHR display_plane_caps;
    };

    tl::optional<DisplayDataDisplayPlaneCapsModeIndex> best_choice{};

    for (const DisplayPlaneData& dppd : display_plane_data) {
        XR_LOG_INFO("plane: display {:#x}, stack index: {}",
                    reinterpret_cast<uintptr_t>(dppd.properties.currentDisplay),
                    dppd.properties.currentStackIndex);

        const auto dpy_itr =
            ranges::find_if(attached_displays, [dpy = dppd.properties.currentDisplay](const DisplayData& dd) {
                return dd.properties.display == dpy;
            });

        if (dpy_itr == cend(attached_displays)) {
            continue;
        }

        const size_t display_data_idx = static_cast<size_t>(distance(cbegin(attached_displays), dpy_itr));

        const auto [display_mode_idx, display_plane_caps] =
            dpy_itr->display_modes %
            fn::transform([display_mode_idx = 0u, pd, plane_index = dppd.index](
                              const VkDisplayModePropertiesKHR& dpm) mutable {
                VkDisplayPlaneCapabilitiesKHR display_plane_capabilities;
                WRAP_VULKAN_FUNC(
                    vkGetDisplayPlaneCapabilitiesKHR, pd, dpm.displayMode, plane_index, &display_plane_capabilities);
                return make_tuple(display_mode_idx + 1, display_plane_capabilities);
            }) %
            fn::foldl(tuple<uint32_t, VkDisplayPlaneCapabilitiesKHR>{},
                      [](const tuple<uint32_t, VkDisplayPlaneCapabilitiesKHR>& out,
                         const tuple<uint32_t, VkDisplayPlaneCapabilitiesKHR>& in) {
                          const auto& [out_idx, out_caps] = out;
                          const auto& [in_idx, in_caps] = in;

                          if (in_caps.maxDstExtent.width > out_caps.maxDstExtent.width &&
                              in_caps.maxDstExtent.height > out_caps.maxDstExtent.height)
                              return in;

                          return out;
                      });

        best_choice = best_choice.take().map_or_else(
            [display_data_idx, display_mode_idx, display_plane_idx = dppd.index, display_plane_caps](
                DisplayDataDisplayPlaneCapsModeIndex dddpi) {
                if (dddpi.display_plane_caps.maxDstExtent.width < display_plane_caps.maxDstExtent.width &&
                    dddpi.display_plane_caps.maxDstExtent.height < display_plane_caps.maxDstExtent.height) {
                    return DisplayDataDisplayPlaneCapsModeIndex{
                        display_data_idx, display_plane_idx, display_mode_idx, display_plane_caps
                    };
                }
                return dddpi;
            },
            [display_data_idx, display_plane_idx = dppd.index, display_mode_idx, display_plane_caps]() {
                return DisplayDataDisplayPlaneCapsModeIndex{
                    display_data_idx, display_plane_idx, display_mode_idx, display_plane_caps
                };
            });
    }

    return best_choice.and_then(
        [&attached_displays, &display_plane_data, instance](
            const DisplayDataDisplayPlaneCapsModeIndex& dddpi) -> tl::optional<PresentToDisplaySurface> {
            XR_LOG_INFO("capabilities: min dst pos {}, max dst pos {}, min dst extent {}, max dst extent {}",
                        dddpi.display_plane_caps.minDstPosition,
                        dddpi.display_plane_caps.maxDstPosition,
                        dddpi.display_plane_caps.minDstExtent,
                        dddpi.display_plane_caps.maxDstExtent);

            const VkDisplaySurfaceCreateInfoKHR display_surface_create_info = {
                .sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .displayMode =
                    attached_displays[dddpi.display_data_idx].display_modes[dddpi.display_mode_idx].displayMode,
                .planeIndex = static_cast<uint32_t>(dddpi.display_plane_idx),
                .planeStackIndex = display_plane_data[dddpi.display_data_idx].properties.currentStackIndex,
                .transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .globalAlpha = 1.0f,
                .alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
                .imageExtent = dddpi.display_plane_caps.maxDstExtent,
            };

            VkSurfaceKHR display_surface{};
            if (const VkResult result = WRAP_VULKAN_FUNC(
                    vkCreateDisplayPlaneSurfaceKHR, instance, &display_surface_create_info, nullptr, &display_surface);
                result != VK_SUCCESS) {
                return tl::nullopt;
            }

            return tl::make_optional<PresentToDisplaySurface>(
                attached_displays[dddpi.display_data_idx],
                display_plane_data[dddpi.display_plane_idx],
                dddpi.display_plane_caps,
                xrUniqueVkSurfaceKHR{ display_surface, VkResourceDeleter_VkSurfaceKHR{ instance } });
        });
}

#if defined(XRAY_OS_IS_WINDOWS)
#else

tl::optional<PresentToSurface>
create_xcb_surface(const WindowPlatformDataXcb& win_platform_data, VkInstance instance)
{
    const VkXcbSurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .connection = win_platform_data.connection,
        .window = win_platform_data.window,
    };

    xrUniqueVkSurfaceKHR surface_khr{
        [instance, &surface_create_info]() {
            VkSurfaceKHR surface{};
            WRAP_VULKAN_FUNC(vkCreateXcbSurfaceKHR, instance, &surface_create_info, nullptr, &surface);
            return surface;
        }(),
        VkResourceDeleter_VkSurfaceKHR{ instance },
    };

    if (!surface_khr)
        return tl::nullopt;

    XR_LOG_INFO("Surface (XCB) created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(surface_khr)));
    return tl::make_optional<PresentToSurface>(PresentToWindowSurface{ win_platform_data, std::move(surface_khr) });
}

tl::optional<PresentToSurface>
create_xlib_surface(const WindowPlatformDataXlib& win_platform_data, VkInstance instance)
{
    const VkXlibSurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .dpy = win_platform_data.display,
        .window = win_platform_data.window,
    };

    xrUniqueVkSurfaceKHR surface_khr{
        [&]() {
            VkSurfaceKHR surface{};
            WRAP_VULKAN_FUNC(vkCreateXlibSurfaceKHR, instance, &surface_create_info, nullptr, &surface);
            return surface;
        }(),
        VkResourceDeleter_VkSurfaceKHR{ instance },
    };

    if (!surface_khr)
        return tl::nullopt;

    XR_LOG_INFO("Surface created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(surface_khr)));
    return tl::make_optional<PresentToSurface>(PresentToWindowSurface{
        win_platform_data,
        std::move(surface_khr),
    });
}

#endif

uint32_t
vk_find_allocation_memory_type(const VkPhysicalDeviceMemoryProperties& memory_properties,
                               const uint32_t memory_requirements,
                               const VkMemoryPropertyFlags required_flags)
{
    XR_LOG_TRACE("Memory required:\n{:0>32b}\n{:0>32b}", memory_requirements, required_flags);
    for (uint32_t memory_type = 0, memory_types_count = memory_properties.memoryTypeCount;
         memory_type < memory_types_count;
         ++memory_type) {
        const uint32_t memory_type_bits = 1 << memory_type;

        XR_LOG_TRACE("Device memory {:0>32b}, heap index {}, mem type {}",
                     memory_properties.memoryTypes[memory_type].propertyFlags,
                     memory_properties.memoryTypes[memory_type].heapIndex,
                     memory_type);

        const bool is_required_mem_type = memory_requirements & memory_type_bits;
        const VkMemoryPropertyFlags mem_prop_flags = memory_properties.memoryTypes[memory_type].propertyFlags;
        const bool has_required_properties = (mem_prop_flags & required_flags) == required_flags;

        if (is_required_mem_type && has_required_properties)
            return memory_type;
    }

    return 0xffffffffu;
}

struct SwapchainStateCreationInfo
{
    VkDevice device;
    VkSwapchainKHR retired_swapchain;
    VkSurfaceKHR surface;
    VkSurfaceCapabilitiesKHR surface_caps;
    VkSurfaceFormatKHR fmt;
    VkPresentModeKHR present_mode;
    std::reference_wrapper<const VkPhysicalDeviceMemoryProperties> mem_props;
    uint32_t image_count;
    VkExtent3D dimensions;
    VkFormat depth_att_format;
};

tl::optional<detail::SwapchainState>
create_swapchain_state(const SwapchainStateCreationInfo& create_info);

tl::optional<VulkanRenderer>
VulkanRenderer::create(const WindowPlatformData& win_data)
{
    const itlib::small_vector<VkExtensionProperties, 4> supported_extensions{ enumerate_instance_extensions() };
    for (const VkExtensionProperties& e : supported_extensions) {
        XR_LOG_INFO("extension: {} - {:#0x}", e.extensionName, e.specVersion);
    }

    const small_vec_4<const char*> extensions_list{ [&supported_extensions]() {
        small_vec_4<const char*> exts_list{
            VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(XRAY_OS_IS_WINDOWS)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        };

        static constexpr const initializer_list<const char*> display_extensions_list = {
            VK_KHR_DISPLAY_EXTENSION_NAME, VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME
        };

#if 0
        lz::chain(display_extensions_list)
            .filter([&supported_extensions](const char* ext_name) {
                return supported_extensions % fn::exists_where([ext_name](const VkExtensionProperties& e) {
                           return strcmp(e.extensionName, ext_name) == 0;
                       });
            })
            .copyTo(std::back_inserter(exts_list));
#else
        for (const char* ext : display_extensions_list) {
            const bool is_supported = supported_extensions % fn::exists_where([ext](const VkExtensionProperties& e) {
                                          return strcmp(e.extensionName, ext) == 0;
                                      });
            if (is_supported)
                exts_list.push_back(ext);
        }
#endif

        return exts_list;
    }() };

    const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "xray-ng-app",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "xray-engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };

    constexpr const char* const kRequiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

    const VkDebugUtilsMessengerCreateInfoEXT dbg_utils_msg_create_ext = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = log_vk_debug_output,
        .pUserData = nullptr,
    };

    const VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = &dbg_utils_msg_create_ext,
        .flags = 0,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(std::size(kRequiredLayers)),
        .ppEnabledLayerNames = kRequiredLayers,
        .enabledExtensionCount = static_cast<uint32_t>(std::size(extensions_list)),
        .ppEnabledExtensionNames = extensions_list.data(),
    };

    xrUniqueVkInstance vkinstance{ [&]() {
                                      VkInstance instance{};
                                      WRAP_VULKAN_FUNC(vkCreateInstance, &instance_create_info, nullptr, &instance);
                                      return instance;
                                  }(),
                                   VkResourceDeleter_VkInstance{ NotOwnedVulkanResource{} } };

    if (!vkinstance)
        return tl::nullopt;

    XR_LOG_INFO("Vulkan instance created.");

#define XRAY_MK1(tok) XRAY_STRINGIZE_a(tok)
#define XRAY_MK0(a, b) XRAY_MK1(a##b)
#define XR_MAKE_VK_FUNC(name) XRAY_MK0(vk, name)

#define PFN_LIST_ENTRY(fnproto, name)                                                                                  \
    do {                                                                                                               \
        vkfn::name = reinterpret_cast<fnproto>(vkGetInstanceProcAddr(raw_ptr(vkinstance), XR_MAKE_VK_FUNC(name)));     \
        XR_LOG_INFO("Querying function pointer {} -> {:#x}", #fnproto, (uintptr_t)vkfn::name);                         \
        assert(vkfn::name != nullptr);                                                                                 \
    } while (0);

    FUNCTION_POINTERS_LIST
#undef PFN_LIST_ENTRY
#undef FUNCTION_POINTERS_LIST

    xrUniqueVkDebugUtilsMessengerEXT dbg_msgr{
        [instance = raw_ptr(vkinstance), &dbg_utils_msg_create_ext]() {
            VkDebugUtilsMessengerEXT msg{};
            WRAP_VULKAN_FUNC(vkfn::CreateDebugUtilsMessengerEXT, instance, &dbg_utils_msg_create_ext, nullptr, &msg);
            return msg;
        }(),
        VkResourceDeleter_VkDebugUtilsMessengerEXT{ raw_ptr(vkinstance) }
    };

    const itlib::small_vector<VkPhysicalDevice, 4> phys_devices{ enumerate_physical_devices(raw_ptr(vkinstance)) };

    auto check_physical_device_presentation_surface_support = [](const WindowPlatformData& win_data,
                                                                 VkInstance instance,
                                                                 VkPhysicalDevice device,
                                                                 const uint32_t queue_index) {
#if defined(XRAY_OS_IS_WINDOWS)
        auto get_win32_presentation_support = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));

        assert(get_win32_presentation_support != nullptr);
        return get_win32_presentation_support(device, queue_index);
#else
        if (const WindowPlatformDataXlib* xlib = swl::get_if<WindowPlatformDataXlib>(&win_data)) {
            PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR get_physical_device_xlib_presentation_support_khr =
                reinterpret_cast<PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR>(
                    vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR"));

            assert(get_physical_device_xlib_presentation_support_khr != nullptr);
            return get_physical_device_xlib_presentation_support_khr(
                       device, queue_index, xlib->display, xlib->visual) == VK_TRUE;
        }

        if (const WindowPlatformDataXcb* xcb = swl::get_if<WindowPlatformDataXcb>(&win_data)) {
            PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR get_physical_device_xcb_presentation_support_khr =
                reinterpret_cast<PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR>(
                    vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR"));

            assert(get_physical_device_xcb_presentation_support_khr != nullptr);
            return get_physical_device_xcb_presentation_support_khr(
                       device, queue_index, xcb->connection, xcb->visual) == VK_TRUE;
        }
#endif
    };

    small_vec_4<tuple<detail::PhysicalDeviceData, size_t, size_t>> phys_devices_data{};

    lz::chain(phys_devices)
        .map([instance = raw_ptr(vkinstance), &win_data, check_physical_device_presentation_surface_support](
                 VkPhysicalDevice phys_device) -> tl::optional<tuple<detail::PhysicalDeviceData, size_t, size_t>> {
            const detail::PhysicalDeviceData pdd{ phys_device };

            XR_LOG_INFO("Checking device {} suitability ...", pdd.properties.base.properties.deviceName);
            for (size_t qid = 0, max_queues = pdd.queue_props.size(); qid < max_queues; ++qid) {
                const VkQueueFamilyProperties* q = &pdd.queue_props[qid];
                XR_LOG_INFO("Queue family {}, queue count {}, queue flags {}",
                            qid,
                            q->queueCount,
                            vk::to_string(static_cast<vk::QueueFlags>(q->queueFlags)));
            }

            constexpr const uint32_t suitable_device_types[] = {
                VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
                VK_PHYSICAL_DEVICE_TYPE_CPU,
                VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
            };

            const bool is_requested_device_type{
                suitable_device_types %
                fn::exists_where([device_type = pdd.properties.base.properties.deviceType](const uint32_t required) {
                    return required == device_type;
                })
            };

            if (!is_requested_device_type) {
                XR_LOG_INFO("Rejecting device {}, unsuitable type {:#x}",
                            pdd.properties.base.properties.deviceName,
                            static_cast<uint32_t>(pdd.properties.base.properties.deviceType));
                return tl::nullopt;
            }

            const auto graphics_q_idx = r_find_pos(
                pdd.queue_props, [](const VkQueueFamilyProperties& q) { return q.queueFlags & VK_QUEUE_GRAPHICS_BIT; });

            if (!graphics_q_idx) {
                XR_LOG_INFO("Rejecting device {}, no graphics queue support.",
                            pdd.properties.base.properties.deviceName);
                return tl::nullopt;
            }

            // surface presentation support

            if (!check_physical_device_presentation_surface_support(win_data, instance, pdd.device, *graphics_q_idx)) {
                XR_LOG_INFO("Rejecting device {}, no surface presentation support (queue family index {})",
                            pdd.properties.base.properties.deviceName,
                            *graphics_q_idx);
                return tl::nullopt;
            }

            auto transfer_q_idx = r_find_pos(
                pdd.queue_props, [](const VkQueueFamilyProperties& q) { return q.queueFlags & VK_QUEUE_TRANSFER_BIT; });

            if (!transfer_q_idx) {
                XR_LOG_INFO("Rejecting device {}, no graphics queue support.",
                            pdd.properties.base.properties.deviceName);
                return tl::nullopt;
            }

            if (transfer_q_idx == graphics_q_idx) {
                // try to find a different queue with transfer if possible

                if (const auto different_transfer_queue = r_find_pos(
                        std::span{ std::cbegin(pdd.queue_props) + *transfer_q_idx, std::cend(pdd.queue_props) },
                        [](const VkQueueFamilyProperties& q) { return q.queueFlags & VK_QUEUE_TRANSFER_BIT; })) {
                    transfer_q_idx = different_transfer_queue;
                }
            }

            small_vec_4<VkExtensionProperties> device_extensions{ [dev = pdd.device]() {
                uint32_t extensions_count{};
                WRAP_VULKAN_FUNC(vkEnumerateDeviceExtensionProperties, dev, nullptr, &extensions_count, nullptr);
                small_vec_4<VkExtensionProperties> device_extensions{ extensions_count };
                WRAP_VULKAN_FUNC(
                    vkEnumerateDeviceExtensionProperties, dev, nullptr, &extensions_count, device_extensions.data());

                return device_extensions;
            }() };

            device_extensions % fn::for_each([](const VkExtensionProperties& ext_props) {
                XR_LOG_INFO("{} - {:#x}", ext_props.extensionName, ext_props.specVersion);
            });

            return tl::make_optional(make_tuple(pdd, *graphics_q_idx, *transfer_q_idx));
        })
        .filter([](tl::optional<tuple<detail::PhysicalDeviceData, size_t, size_t>> data) { return data.has_value(); })
        .forEach([&phys_devices_data](tl::optional<tuple<detail::PhysicalDeviceData, size_t, size_t>> pd) mutable {
            pd.take().map([&phys_devices_data](tuple<detail::PhysicalDeviceData, size_t, size_t>&& pd) mutable {
                phys_devices_data.push_back(pd);
            });
        });

    if (phys_devices_data.empty()) {
        XR_LOG_ERR("No suitable physical devices present in the system");
        return tl::nullopt;
    }

    const auto& [phys_device, queue_graphics, queue_transfer] = phys_devices_data.front();
    const float queue_priorities[] = { 1.0f, 1.0f };

    XR_LOG_INFO("Using device {}, vendor {:#x}",
                phys_device.properties.base.properties.deviceName,
                phys_device.properties.base.properties.vendorID);

    small_vec_2<VkDeviceQueueCreateInfo> queue_create_info{};

    queue_create_info.emplace_back(VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = static_cast<uint32_t>(queue_graphics),
        .queueCount = 1,
        .pQueuePriorities = queue_priorities,
    });

    if (queue_graphics != queue_transfer) {
        queue_create_info.emplace_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = static_cast<uint32_t>(queue_transfer),
            .queueCount = 1,
            .pQueuePriorities = queue_priorities,
        });
    }

    static constexpr initializer_list<const char*> device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
        VK_EXT_DEBUG_MARKER_EXTENSION_NAME,
    };

    const VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &phys_device.features.base,
        .flags = 0,
        .queueCreateInfoCount = static_cast<uint32_t>(size(queue_create_info)),
        .pQueueCreateInfos = queue_create_info.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(size(device_extensions)),
        .ppEnabledExtensionNames = device_extensions.begin(),
        .pEnabledFeatures = nullptr,
    };

    xrUniqueVkDevice logical_device{
        [physical_device = phys_device.device, &device_create_info]() {
            VkDevice logical_device{};
            WRAP_VULKAN_FUNC(vkCreateDevice, physical_device, &device_create_info, nullptr, &logical_device);
            return logical_device;
        }(),
        VkResourceDeleter_VkDevice{ NotOwnedVulkanResource{} }
    };

    if (!logical_device)
        return tl::nullopt;

    array<VkQueue, 2> queues{};
    vkGetDeviceQueue(raw_ptr(logical_device), queue_graphics, 0, &queues[0]);
    vkGetDeviceQueue(raw_ptr(logical_device), queue_transfer, (queue_transfer != queue_graphics), &queues[1]);
    XR_LOG_INFO("Queue ids: graphics {}, transfer {}",
                static_cast<const void*>(queues[0]),
                static_cast<const void*>(queues[1]));

    XR_LOG_INFO("Device created successfully");

    tl::optional<PresentToSurface> present_to_surface{ [&win_data, instance = raw_ptr(vkinstance)]() -> tl::optional<PresentToSurface> {
#if defined(XRAY_OS_IS_WINDOWS)
        if (const WindowPlatformDataWin32* wp = swl::get_if<WindowPlatformDataWin32>(&win_data)) {
            const VkWin32SurfaceCreateInfoKHR create_info{ .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                                                           .pNext = nullptr,
                                                           .flags = 0,
                                                           .hinstance =
                                                               reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr)),
                                                           .hwnd = wp->window };

            xrUniqueVkSurfaceKHR surface{ [&]() {
                                             VkSurfaceKHR surface{};
                                             WRAP_VULKAN_FUNC(
                                                 vkCreateWin32SurfaceKHR, instance, &create_info, nullptr, &surface);
                                             return surface;
                                         }(),
                                          VkResourceDeleter_VkSurfaceKHR{ instance } };

            if (!surface)
                return tl::nullopt;

            XR_LOG_INFO("Surface created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(surface)));
            return tl::make_optional<PresentToSurface>(PresentToWindowSurface{ *wp, std::move(surface) });
        }

#else
        if (const WindowPlatformDataXlib* xlib = swl::get_if<WindowPlatformDataXlib>(&win_data)) {
            return create_xlib_surface(*xlib, instance);
        }

        if (const WindowPlatformDataXcb* xcb = swl::get_if<WindowPlatformDataXcb>(&win_data)) {
            return create_xcb_surface(*xcb, instance);
        }
#endif
    }() };

    if (!present_to_surface) {
        XR_LOG_CRITICAL("Cannot create a surface of any kind (display/window)");
        return tl::nullopt;
    }

    struct SurfaceInfo
    {
        VkSurfaceCapabilitiesKHR caps;
        VkSurfaceFormatKHR format;
        VkPresentModeKHR present_mode;
        VkFormat depth_stencil_format;
    };

    struct SwapchainStateWithSurfaceInfo
    {
        detail::SwapchainState swapchain_state;
        SurfaceInfo surface_info;
    };

    tl::optional<SwapchainStateWithSurfaceInfo> swapchain_state =
        present_to_surface.and_then([pd = &phys_device, &win_data, vkdev = raw_ptr(logical_device)](
                                        const PresentToSurface& ps) -> tl::optional<SwapchainStateWithSurfaceInfo> {
            VkSurfaceKHR surface =
                swl::visit(VariantVisitor{
                               [](const PresentToWindowSurface& win) { return raw_ptr(win.surface); },
                               [](const PresentToDisplaySurface& display) { return raw_ptr(display.display_surface); },
                           },
                           ps);

            VkSurfaceCapabilitiesKHR surface_caps;
            if (const VkResult res =
                    WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, pd->device, surface, &surface_caps);
                res != VK_SUCCESS) {
                return tl::nullopt;
            }

            XR_LOG_INFO("Surface capabilities: {}", surface_caps);

            small_vec_4<VkSurfaceFormatKHR> supported_surface_fmts{ [device = pd->device, surface]() {
                small_vec_4<VkSurfaceFormatKHR> fmts;
                uint32_t fmts_count{};
                WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface, &fmts_count, nullptr);
                if (fmts_count) {
                    fmts.resize(fmts_count);
                    WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface, &fmts_count, fmts.data());
                }

                return fmts;
            }() };

            for (const VkSurfaceFormatKHR& fmt : supported_surface_fmts) {
                XR_LOG_INFO("{}", vk::to_string(static_cast<vk::Format>(fmt.format)));
            }

            const small_vec_4<VkPresentModeKHR> supported_presentation_modes{ [device = pd->device, surface]() {
                uint32_t present_modes_count{};
                WRAP_VULKAN_FUNC(
                    vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface, &present_modes_count, nullptr);

                small_vec_4<VkPresentModeKHR> present_modes;
                present_modes.resize(present_modes_count);
                WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR,
                                 device,
                                 surface,
                                 &present_modes_count,
                                 present_modes.data());

                return present_modes;
            }() };

            XR_LOG_INFO("{}", supported_presentation_modes % fn::transform([](const VkPresentModeKHR pm) {
                                  return fmt::format("{:#x} -> {} ",
                                                     static_cast<uint32_t>(pm),
                                                     vk::to_string(static_cast<vk::PresentModeKHR>(pm)));
                              }) % fn::foldl(string{ "supported presentation modes: " }, plus<string>{}));

            constexpr const VkPresentModeKHR preferred_presentation_modes[] = {
                VK_PRESENT_MODE_MAILBOX_KHR,
                VK_PRESENT_MODE_IMMEDIATE_KHR,
                VK_PRESENT_MODE_FIFO_RELAXED_KHR,
                VK_PRESENT_MODE_FIFO_KHR,
            };

            const auto best_preferred_supported_mode =
                ranges::find_first_of(preferred_presentation_modes, supported_presentation_modes);

            if (best_preferred_supported_mode == cend(supported_presentation_modes)) {
                XR_LOG_CRITICAL("None of the preferred presentation modes is suppored!");
                return tl::nullopt;
            }

            XR_LOG_INFO("best preferred supported present mode is {:#x} -> {}",
                        static_cast<uint32_t>(*best_preferred_supported_mode),
                        vk::to_string(static_cast<vk::PresentModeKHR>(*best_preferred_supported_mode)));

            const uint32_t swapchain_image_count = [&surface_caps]() {
                if (surface_caps.maxImageCount == 0) {
                    //
                    // no limit for the maximum number of images
                    return surface_caps.minImageCount + 4;
                }
                return min(surface_caps.minImageCount + 4, surface_caps.maxImageCount);
            }();

            const VkExtent3D swapchain_dimensions = swl::visit(
                VariantVisitor{
#if defined(XRAY_OS_IS_WINDOWS)
                    [](const WindowPlatformDataWin32& win32) {
                        return VkExtent3D{ .width = win32.width, .height = win32.height, .depth = 1 };
                    }
#else
                    [](const WindowPlatformDataXcb& xcb) {
                        return VkExtent3D{ .width = xcb.width, .height = xcb.height, .depth = 1 };
                    },
                    [](const WindowPlatformDataXlib& xlib) {
                        return VkExtent3D{ .width = xlib.width, .height = xlib.height, .depth = 1 };
                    },
#endif
                },
                win_data);

            const SwapchainStateCreationInfo swapchain_state_create_info{
                .device = vkdev,
                .retired_swapchain = nullptr,
                .surface = surface,
                .surface_caps = surface_caps,
                .fmt = supported_surface_fmts[0],
                .present_mode = *best_preferred_supported_mode,
                .mem_props = pd->memory,
                .image_count = swapchain_image_count,
                .dimensions = swapchain_dimensions,
                .depth_att_format = VK_FORMAT_D32_SFLOAT_S8_UINT,
            };

            return create_swapchain_state(swapchain_state_create_info)
                .and_then([&swapchain_state_create_info](detail::SwapchainState&& swapchain_state) {
                    return tl::make_optional<SwapchainStateWithSurfaceInfo>(
                        std::move(swapchain_state),
                        SurfaceInfo{
                            .caps = swapchain_state_create_info.surface_caps,
                            .format = swapchain_state_create_info.fmt,
                            .present_mode = swapchain_state_create_info.present_mode,
                            .depth_stencil_format = swapchain_state_create_info.depth_att_format,
                        });
                });
        });

    if (!swapchain_state) {
        XR_LOG_ERR("Oy blyat ! failed to create rendering state!");
        return tl::nullopt;
    }

    //
    // descriptor pool
    xrUniqueVkDescriptorPool dpool{
        [device = raw_ptr(logical_device)]() {
            const VkDescriptorPoolSize pool_sizes[] = {
                { .type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = 1024 },
                { .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1024 },
                { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1024 },
            };

            const VkDescriptorPoolCreateInfo pool_create_info = {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
                .maxSets = 1024,
                .poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes)),
                .pPoolSizes = pool_sizes,
            };

            VkDescriptorPool pool{ nullptr };
            WRAP_VULKAN_FUNC(vkCreateDescriptorPool, device, &pool_create_info, nullptr, &pool);
            return pool;
        }(),
        VkResourceDeleter_VkDescriptorPool{ raw_ptr(logical_device) },
    };

    if (!dpool) {
        XR_LOG_ERR("Failed to create descriptor pool !");
        return tl::nullopt;
    }

    //
    // queues
    vector<detail::Queue> qs{ [&]() {
        const array<uint32_t, 2> queue_flags{ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                              VK_COMMAND_POOL_CREATE_TRANSIENT_BIT };
        const array<size_t, 2> queue_indices{ queue_graphics, queue_transfer };
        vector<detail::Queue> qs;

        for (size_t i = 0; i < 2; ++i) {
            const VkCommandPoolCreateInfo cmd_pool_create_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext = nullptr,
                .flags = queue_flags[i],
                .queueFamilyIndex = static_cast<uint32_t>(queue_indices[i]),
            };

            VkCommandPool cmd_pool{};
            WRAP_VULKAN_FUNC(vkCreateCommandPool, raw_ptr(logical_device), &cmd_pool_create_info, nullptr, &cmd_pool);

            qs.emplace_back(
                static_cast<uint32_t>(queue_indices[i]),
                queues[i],
                xrUniqueVkCommandPool{ cmd_pool, VkResourceDeleter_VkCommandPool{ raw_ptr(logical_device) } });
        }

        return qs;
    }() };

    //
    // command buffers
    vector<VkCommandBuffer> command_buffers{ [&]() {
        const VkCommandBufferAllocateInfo cmd_buff_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext = nullptr,
            .commandPool = raw_ptr(qs[0].cmd_pool),
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = static_cast<uint32_t>(swapchain_state->swapchain_state.swapchain_imageviews.size()),
        };

        vector<VkCommandBuffer> cmd_buffers{ swapchain_state->swapchain_state.swapchain_imageviews.size() };
        WRAP_VULKAN_FUNC(vkAllocateCommandBuffers, raw_ptr(logical_device), &cmd_buff_alloc_info, cmd_buffers.data());

        return cmd_buffers;
    }() };

    const uint32_t max_frames{ static_cast<uint32_t>(swapchain_state->swapchain_state.swapchain_images.size()) };

    auto [swapchain, surface_info] = std::move(*swapchain_state.take());

    auto work_pkg_queue{ WorkPackageState::create(raw_ptr(logical_device), qs[0].handle, qs[0].index) };
    if (!work_pkg_queue)
        return tl::nullopt;

    auto bindless_sys{ BindlessSystem::create(raw_ptr(logical_device)) };
    if (!bindless_sys) {
        return tl::nullopt;
    }

    return tl::make_optional<VulkanRenderer>(
        PrivateConstructionToken{},
        detail::InstanceState{
            std::move(vkinstance),
            std::move(dbg_msgr),
        },
        detail::RenderState{
            phys_device,
            std::move(logical_device),
            std::move(qs),
            detail::RenderingAttachments{
                .view_mask = 0,
                .attachments = { surface_info.format.format,
                                 surface_info.depth_stencil_format,
                                 surface_info.depth_stencil_format },
            },
        },
        detail::PresentationState{
            0,
            max_frames,
            0,
            0,
            detail::SurfaceState{
                swl::visit(VariantVisitor{
                               [](PresentToWindowSurface&& win_surface) { return std::move(win_surface.surface); },
                               [](PresentToDisplaySurface&& display_surface) {
                                   return std::move(display_surface.display_surface);
                               },
                           },
                           std::move(*present_to_surface.take())),
                surface_info.caps,
                surface_info.format,
                surface_info.present_mode,
                surface_info.depth_stencil_format,
            },
            std::move(swapchain),
            std::move(command_buffers),
        },
        detail::DescriptorPoolState{ std::move(dpool) },
        std::move(*work_pkg_queue),
        std::move(*bindless_sys));
}

VulkanRenderer::VulkanRenderer(VulkanRenderer::PrivateConstructionToken,
                               detail::InstanceState instance_state,
                               detail::RenderState render_state,
                               detail::PresentationState presentation_state,
                               detail::DescriptorPoolState dpool,
                               WorkPackageState work_pkg_queue,
                               BindlessSystem bindless)
    : _instance_state{ std::move(instance_state) }
    , _render_state{ std::move(render_state) }
    , _presentation_state{ std::move(presentation_state) }
    , _dpool_state{ std::move(dpool) }
    , _work_queue{ std::move(work_pkg_queue) }
    , _bindless{ std::move(bindless) }
{
}

FrameRenderData
VulkanRenderer::begin_rendering()
{
    //
    // wait for previously submitted work to finish
    const VkFence fences[] = { raw_ptr(
        _presentation_state.swapchain_state.sync.fences[_presentation_state.frame_index]) };
    const VkResult wait_fence_result{ WRAP_VULKAN_FUNC(
        vkWaitForFences, raw_ptr(_render_state.dev_logical), 1, fences, true, numeric_limits<uint64_t>::max()) };

    if (wait_fence_result != VK_SUCCESS) {
        XR_LOG_CRITICAL("Failed to wait on fence!");
        // TODO: handle device lost
    }

    WRAP_VULKAN_FUNC(vkResetFences, raw_ptr(_render_state.dev_logical), 1, fences);

    const VkResult acquire_image_result{ WRAP_VULKAN_FUNC(
        vkAcquireNextImageKHR,
        raw_ptr(_render_state.dev_logical),
        raw_ptr(_presentation_state.swapchain_state.swapchain),
        numeric_limits<uint64_t>::max(),
        raw_ptr(_presentation_state.swapchain_state.sync.present_sem[_presentation_state.frame_index]),
        nullptr,
        &_presentation_state.acquired_image) };

    if (acquire_image_result == VK_SUBOPTIMAL_KHR || acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR ||
        _presentation_state.state_bits & detail::PresentationState::STATE_SWAPCHAIN_SUBOPTIMAL) {

        XR_LOG_INFO("Swapchain is sub-optimal/out-of-date, trying to recreate ...");

        VkSurfaceCapabilitiesKHR surface_caps;
        const VkResult query_result = WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
                                                       _render_state.dev_physical.device,
                                                       raw_ptr(_presentation_state.surface_state.surface),
                                                       &surface_caps);

        if (query_result == VK_SUCCESS &&
            (surface_caps.currentExtent.width != _presentation_state.surface_state.caps.currentExtent.width ||
             surface_caps.currentExtent.height != _presentation_state.surface_state.caps.currentExtent.height)) {
            XR_LOG_INFO("Suboptimal/out of date : surface caps = {}", surface_caps);

            if (surface_caps.currentExtent.width == numeric_limits<uint32_t>::max() ||
                surface_caps.currentExtent.height == numeric_limits<uint32_t>::max()) {
                surface_caps.currentExtent = _presentation_state.surface_state.caps.currentExtent;
            }

            wait_device_idle();

            const SwapchainStateCreationInfo swapchain_state_creation_info{
                .device = raw_ptr(_render_state.dev_logical),
                .retired_swapchain = raw_ptr(_presentation_state.swapchain_state.swapchain),
                .surface = raw_ptr(_presentation_state.surface_state.surface),
                .surface_caps = surface_caps,
                .fmt = _presentation_state.surface_state.format,
                .present_mode = _presentation_state.surface_state.present_mode,
                .mem_props = _render_state.dev_physical.memory,
                .image_count = static_cast<uint32_t>(_presentation_state.max_frames),
                .dimensions =
                    VkExtent3D{
                        .width = surface_caps.currentExtent.width,
                        .height = surface_caps.currentExtent.height,
                        .depth = 1,
                    },
                .depth_att_format = _presentation_state.surface_state.depth_stencil_format,
            };

            create_swapchain_state(swapchain_state_creation_info)
                .map_or_else(
                    [&](detail::SwapchainState new_swapchain_state) {
                        _presentation_state.swapchain_state = std::move(new_swapchain_state);
                        _presentation_state.state_bits &= ~detail::PresentationState::STATE_SWAPCHAIN_SUBOPTIMAL;
                        _presentation_state.surface_state.caps = surface_caps;

                        const uint32_t previous_acquired_image{ _presentation_state.acquired_image };

                        WRAP_VULKAN_FUNC(
                            vkAcquireNextImageKHR,
                            raw_ptr(_render_state.dev_logical),
                            raw_ptr(_presentation_state.swapchain_state.swapchain),
                            numeric_limits<uint64_t>::max(),
                            raw_ptr(
                                _presentation_state.swapchain_state.sync.present_sem[_presentation_state.frame_index]),
                            nullptr,
                            &_presentation_state.acquired_image);

                        XR_LOG_INFO("swapchain recreation previous acquired image {}, re-acquired image {}",
                                    previous_acquired_image,
                                    _presentation_state.acquired_image);

                        const VkFence reset_fences[] = { raw_ptr(
                            _presentation_state.swapchain_state.sync.fences[_presentation_state.frame_index]) };
                        WRAP_VULKAN_FUNC(vkResetFences, this->device(), 1, reset_fences);
                    },
                    []() { XR_LOG_CRITICAL("Could not create new swapchain state"); });
        }
    }

    const uint32_t acquired_image{ _presentation_state.acquired_image };
    //
    // reset command buffer
    WRAP_VULKAN_FUNC(vkResetCommandBuffer, _presentation_state.command_buffers[_presentation_state.frame_index], 0);
    const VkCommandBufferBeginInfo cmd_buf_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    WRAP_VULKAN_FUNC(vkBeginCommandBuffer,
                     _presentation_state.command_buffers[_presentation_state.frame_index],
                     &cmd_buf_begin_info);

    const VkRenderingAttachmentInfo color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = raw_ptr(_presentation_state.swapchain_state.swapchain_imageviews[acquired_image]),
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = nullptr,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = VkClearValue{},
    };

    const VkRenderingAttachmentInfo depth_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext = nullptr,
        .imageView = raw_ptr(_presentation_state.swapchain_state.depth_stencil_image_views[acquired_image]),
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE,
        .resolveImageView = nullptr,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = VkClearValue{},
    };

    const VkRect2D render_area{
        .offset = VkOffset2D{ .x = 0, .y = 0 },
        .extent = _presentation_state.surface_state.caps.currentExtent,
    };

    const VkRenderingInfo rendering_info = VkRenderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = render_area,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = &depth_attachment,
        .pStencilAttachment = &depth_attachment,
    };

    const VkImageMemoryBarrier2 attachments_to_optimal[] = {
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = _presentation_state.swapchain_state.swapchain_images[acquired_image],
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        },
        VkImageMemoryBarrier2{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .pNext = nullptr,
            .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .image = raw_ptr(_presentation_state.swapchain_state.depth_stencil_images[acquired_image].image),
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        },
    };

    const VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = size(attachments_to_optimal),
        .pImageMemoryBarriers = attachments_to_optimal,
    };

    //
    // move rendering attachments from undefined to optimal layout
    WRAP_VULKAN_FUNC(
        vkCmdPipelineBarrier2, _presentation_state.command_buffers[_presentation_state.frame_index], &dependency_info);

    WRAP_VULKAN_FUNC(
        vkCmdBeginRendering, _presentation_state.command_buffers[_presentation_state.frame_index], &rendering_info);

    return FrameRenderData{
        .id = _presentation_state.frame_index,
        .max_frames = _presentation_state.max_frames,
        .cmd_buf = _presentation_state.command_buffers[_presentation_state.frame_index],
        .fbsize = _presentation_state.surface_state.caps.currentExtent,
    };
}

void
VulkanRenderer::end_rendering()
{
    vkCmdEndRendering(_presentation_state.command_buffers[_presentation_state.frame_index]);

    //
    // move rendered image from ATTACHMENT_OPTIMAL to SRC_PRESENT
    const VkImageMemoryBarrier2 color_attachment_optimal_to_src_present = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext = nullptr,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = 0,
        .dstQueueFamilyIndex = 0,
        .image = _presentation_state.swapchain_state.swapchain_images[_presentation_state.acquired_image],
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
    };

    const VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext = nullptr,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
        .memoryBarrierCount = 0,
        .pMemoryBarriers = nullptr,
        .bufferMemoryBarrierCount = 0,
        .pBufferMemoryBarriers = nullptr,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &color_attachment_optimal_to_src_present,
    };

    WRAP_VULKAN_FUNC(
        vkCmdPipelineBarrier2, _presentation_state.command_buffers[_presentation_state.frame_index], &dependency_info);

    WRAP_VULKAN_FUNC(vkEndCommandBuffer, _presentation_state.command_buffers[_presentation_state.frame_index]);

    const VkSemaphoreSubmitInfo semaphore_wait_img_available = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = raw_ptr(_presentation_state.swapchain_state.sync.present_sem[_presentation_state.frame_index]),
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .deviceIndex = 0,
    };

    const VkSemaphoreSubmitInfo semaphore_signal_rendering_done = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = raw_ptr(_presentation_state.swapchain_state.sync.rendering_sem[_presentation_state.frame_index]),
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .deviceIndex = 0,
    };

    const VkCommandBufferSubmitInfo cmd_buffer_submit_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = _presentation_state.command_buffers[_presentation_state.frame_index],
        .deviceMask = 0,
    };

    const VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext = nullptr,
        .flags = 0,
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &semaphore_wait_img_available,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmd_buffer_submit_info,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &semaphore_signal_rendering_done,
    };

    const VkResult submit_result{ WRAP_VULKAN_FUNC(
        vkQueueSubmit2,
        _render_state.queues[0].handle,
        1,
        &submit_info,
        raw_ptr(_presentation_state.swapchain_state.sync.fences[_presentation_state.frame_index])) };

    if (submit_result != VK_SUCCESS) {
        // TODO: handle submit error ??!!
    }

    const VkSwapchainKHR swap_chains[] = { raw_ptr(_presentation_state.swapchain_state.swapchain) };
    const VkSemaphore present_wait_semaphores[] = { raw_ptr(
        _presentation_state.swapchain_state.sync.rendering_sem[_presentation_state.frame_index]) };
    const uint32_t swapchain_image_index[] = { _presentation_state.acquired_image };

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = static_cast<uint32_t>(size(present_wait_semaphores)),
        .pWaitSemaphores = present_wait_semaphores,
        .swapchainCount = static_cast<uint32_t>(size(swap_chains)),
        .pSwapchains = swap_chains,
        .pImageIndices = swapchain_image_index,
        .pResults = nullptr,
    };

    const VkResult present_result{ WRAP_VULKAN_FUNC(vkQueuePresentKHR, _render_state.queues[0].handle, &present_info) };
    _presentation_state.frame_index = (_presentation_state.frame_index + 1) % _presentation_state.max_frames;

    if (present_result != VK_SUCCESS) {
        if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
            //
            // this is handled when begin_rendering is called next frame ...
            _presentation_state.state_bits |= detail::PresentationState::STATE_SWAPCHAIN_SUBOPTIMAL;
        }
    }
}

void
VulkanRenderer::clear_attachments(VkCommandBuffer cmd_buf,
                                  const float red,
                                  const float green,
                                  const float blue,
                                  const float depth,
                                  const uint32_t stencil)
{
    const VkClearAttachment clear_attachments[] = {
        VkClearAttachment{
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .colorAttachment = 0,
            .clearValue = VkClearValue{ .color = VkClearColorValue{ red, green, blue, 1.0f } },
        },
        VkClearAttachment{
            .aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
            .colorAttachment = 0,
            .clearValue = VkClearValue{ .depthStencil = VkClearDepthStencilValue{ depth, stencil } },
        }
    };

    const VkExtent2D surface_extent = _presentation_state.surface_state.caps.currentExtent;
    const VkRect2D clear_area{ .offset = { 0, 0 }, .extent = surface_extent };

    const VkClearRect clear_rects[] = {
        VkClearRect{ .rect = clear_area, .baseArrayLayer = 0, .layerCount = 1 },
        VkClearRect{ .rect = clear_area, .baseArrayLayer = 0, .layerCount = 1 },
    };

    vkCmdClearAttachments(cmd_buf,
                          static_cast<uint32_t>(size(clear_attachments)),
                          clear_attachments,
                          static_cast<uint32_t>(size(clear_rects)),
                          clear_rects);
}

void
VulkanRenderer::wait_device_idle() noexcept
{
    XR_LOG_INFO("Waiting GPU device idle ...");
    WRAP_VULKAN_FUNC(vkDeviceWaitIdle, this->device());
}

uint32_t
xray::rendering::VulkanRenderer::find_allocation_memory_type(const uint32_t memory_requirements,
                                                             const VkMemoryPropertyFlags required_flags) const noexcept
{
    return vk_find_allocation_memory_type(_render_state.dev_physical.memory, memory_requirements, required_flags);
}

tl::optional<UniqueMemoryMapping>
UniqueMemoryMapping::create(VkDevice device,
                            VkDeviceMemory memory,
                            const uint64_t offset,
                            const uint64_t size,
                            const VkMemoryMapFlags flags)

{
    void* mapped_addr{};
    WRAP_VULKAN_FUNC(vkMapMemory, device, memory, offset, size == 0 ? VK_WHOLE_SIZE : size, flags, &mapped_addr);
    if (!mapped_addr) {
        return tl::nullopt;
    }

    return tl::make_optional<UniqueMemoryMapping>(mapped_addr, memory, device, size, offset);
}

UniqueMemoryMapping::~UniqueMemoryMapping()
{
    if (_mapped_memory) {
        const VkMappedMemoryRange mapped_memory_range{
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .pNext = nullptr,
            .memory = _device_memory,
            .offset = _mapped_offset,
            .size = _mapped_size,
        };

        WRAP_VULKAN_FUNC(vkFlushMappedMemoryRanges, _device, 1, &mapped_memory_range);
        WRAP_VULKAN_FUNC(vkUnmapMemory, _device, _device_memory);
    }
}

tl::optional<detail::SwapchainState>
create_swapchain_state(const SwapchainStateCreationInfo& create_info)
{
    //
    // SwapchainKHR object
    xrUniqueVkSwapchainKHR swapchain{
        [&]() {
            const VkSwapchainCreateInfoKHR swapchain_create_info = {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .surface = create_info.surface,
                .minImageCount = create_info.image_count,
                .imageFormat = create_info.fmt.format,
                .imageColorSpace = create_info.fmt.colorSpace,
                .imageExtent = create_info.surface_caps.maxImageExtent,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount = 0,
                .pQueueFamilyIndices = nullptr,
                .preTransform = create_info.surface_caps.currentTransform,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = create_info.present_mode,
                .clipped = false,
                .oldSwapchain = create_info.retired_swapchain,
            };

            VkSwapchainKHR swapchain{};
            WRAP_VULKAN_FUNC(vkCreateSwapchainKHR, create_info.device, &swapchain_create_info, nullptr, &swapchain);
            return swapchain;
        }(),
        VkResourceDeleter_VkSwapchainKHR{ create_info.device },
    };

    XR_LOG_INFO("Swapchain created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(swapchain)));

    //
    // acquire images
    vector<VkImage> swapchain_images{ [&]() {
        uint32_t swapchain_image_count{};
        WRAP_VULKAN_FUNC(
            vkGetSwapchainImagesKHR, create_info.device, raw_ptr(swapchain), &swapchain_image_count, nullptr);

        vector<VkImage> swapchain_images{ swapchain_image_count };
        WRAP_VULKAN_FUNC(vkGetSwapchainImagesKHR,
                         create_info.device,
                         raw_ptr(swapchain),
                         &swapchain_image_count,
                         swapchain_images.data());

        return swapchain_images;
    }() };

    //
    // create image_views
    vector<xrUniqueVkImageView> swapchain_image_views =
        swapchain_images % fn::transform([&](VkImage image) {
            const VkImageViewCreateInfo imageview_create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .image = image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = create_info.fmt.format,
                .components = VkComponentMapping{},
                .subresourceRange =
                    VkImageSubresourceRange{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };

            VkImageView image_view{};
            WRAP_VULKAN_FUNC(vkCreateImageView, create_info.device, &imageview_create_info, nullptr, &image_view);

            return xrUniqueVkImageView{
                image_view,
                VkResourceDeleter_VkImageView{ create_info.device },
            };
        }) %
        fn::where([](const xrUniqueVkImageView& img_view) { return static_cast<bool>(img_view); }) %
        fn::to(vector<xrUniqueVkImageView>{});

    if (swapchain_image_views.size() != create_info.image_count) {
        XR_LOG_ERR_FILE_LINE("Failed to create image views (needed {}, created {})",
                             create_info.image_count,
                             swapchain_image_views.size());
        return tl::nullopt;
    }

    //
    // depth stencil images + image views
    vector<xrUniqueVkImageView> depth_stencil_image_views;
    vector<UniqueImage> depth_stencil_images;

    for (uint32_t idx = 0; idx < create_info.image_count; ++idx) {
        const VkImageCreateInfo image_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = create_info.depth_att_format,
            .extent = VkExtent3D{ .width = create_info.dimensions.width,
                                  .height = create_info.dimensions.height,
                                  .depth = 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        xrUniqueVkImage ds_image{
            [&]() {
                VkImage image{};
                WRAP_VULKAN_FUNC(vkCreateImage, create_info.device, &image_create_info, nullptr, &image);
                if (!image) {
                    XR_LOG_CRITICAL("Failed to create depth-stencil attachment");
                }
                return image;
            }(),
            VkResourceDeleter_VkImage{ create_info.device },
        };

        xrUniqueVkDeviceMemory ds_image_memory{
            [&]() {
                VkMemoryRequirements mem_req;
                vkGetImageMemoryRequirements(create_info.device, raw_ptr(ds_image), &mem_req);
                const VkMemoryAllocateInfo mem_alloc_info = {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .pNext = nullptr,
                    .allocationSize = mem_req.size,
                    .memoryTypeIndex = vk_find_allocation_memory_type(
                        create_info.mem_props, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                };

                VkDeviceMemory image_memory{};
                WRAP_VULKAN_FUNC(vkAllocateMemory, create_info.device, &mem_alloc_info, nullptr, &image_memory);
                if (image_memory)
                    WRAP_VULKAN_FUNC(vkBindImageMemory, create_info.device, raw_ptr(ds_image), image_memory, 0);
                return image_memory;
            }(),
            VkResourceDeleter_VkDeviceMemory{ create_info.device },
        };

        const VkImageViewCreateInfo depth_stencil_view_create_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = raw_ptr(ds_image),
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = create_info.depth_att_format,
            .components = VkComponentMapping{},
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        xrUniqueVkImageView ds_image_view{
            [&]() {
                VkImageView image_view{};
                WRAP_VULKAN_FUNC(
                    vkCreateImageView, create_info.device, &depth_stencil_view_create_info, nullptr, &image_view);
                return image_view;
            }(),
            VkResourceDeleter_VkImageView{ create_info.device },
        };

        if (ds_image && ds_image_view) {
            depth_stencil_images.emplace_back(std::move(ds_image), std::move(ds_image_memory));
            depth_stencil_image_views.push_back(std::move(ds_image_view));
        }
    }

    if (depth_stencil_image_views.size() != create_info.image_count &&
        depth_stencil_images.size() != create_info.image_count) {
        XR_LOG_ERR("Failed to create all the required {} depth stencil images and image views",
                   create_info.image_count);

        return tl::nullopt;
    }

    //
    // sync objects
    const size_t sync_objects_count = swapchain_image_views.size();
    vector<xrUniqueVkFence> fences = [sync_objects_count, &create_info]() {
        vector<xrUniqueVkFence> fences{};
        fences.reserve(sync_objects_count);

        for (size_t idx = 0; idx < sync_objects_count; ++idx) {
            const VkFenceCreateInfo fence_create_info = {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };

            VkFence fence{};
            WRAP_VULKAN_FUNC(vkCreateFence, create_info.device, &fence_create_info, nullptr, &fence);

            if (!fence)
                break;

            fences.emplace_back(fence, VkResourceDeleter_VkFence{ create_info.device });
        }

        return fences;
    }();

    auto make_semaphores_fn = [sync_objects_count, device = create_info.device]() {
        vector<xrUniqueVkSemaphore> semaphores{};
        semaphores.reserve(sync_objects_count);

        for (size_t idx = 0; idx < sync_objects_count; ++idx) {
            const VkSemaphoreCreateInfo semaphore_create_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
            };

            VkSemaphore semaphore{};
            WRAP_VULKAN_FUNC(vkCreateSemaphore, device, &semaphore_create_info, nullptr, &semaphore);

            if (!semaphore)
                break;

            semaphores.emplace_back(semaphore, VkResourceDeleter_VkSemaphore{ device });
        }

        return semaphores;
    };

    vector<xrUniqueVkSemaphore> rendering_semaphores{ make_semaphores_fn() };
    vector<xrUniqueVkSemaphore> presentation_semaphores{ make_semaphores_fn() };

    assert(rendering_semaphores.size() == sync_objects_count && presentation_semaphores.size() == sync_objects_count);

    return tl::make_optional<detail::SwapchainState>(std::move(swapchain),
                                                     std::move(swapchain_images),
                                                     std::move(swapchain_image_views),
                                                     std::move(depth_stencil_images),
                                                     std::move(depth_stencil_image_views),
                                                     detail::SyncState{
                                                         std::move(fences),
                                                         std::move(presentation_semaphores),
                                                         std::move(rendering_semaphores),
                                                     });
}

tl::expected<WorkPackageSetup, VulkanError>
VulkanRenderer::create_work_package()
{
    const WorkPackageHandle pkg_handle{ static_cast<uint32_t>(_work_queue.packages.size()) };

    const VkCommandBufferAllocateInfo cmd_buff_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = _work_queue.cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd_buf{};
    const VkResult alloc_result =
        WRAP_VULKAN_FUNC(vkAllocateCommandBuffers, raw_ptr(_render_state.dev_logical), &cmd_buff_alloc_info, &cmd_buf);
    XR_VK_CHECK_RESULT(alloc_result);

    const VkCommandBufferBeginInfo cmd_buf_begin{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };
    vkBeginCommandBuffer(cmd_buf, &cmd_buf_begin);

    _work_queue.command_buffers.push_back(cmd_buf);
    const CommandBufferHandle cmd_buf_handle{ static_cast<uint32_t>(_work_queue.command_buffers.size() - 1) };

    const VkFenceCreateInfo fence_create_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VkFence fence{};
    const VkResult fence_create_result =
        WRAP_VULKAN_FUNC(vkCreateFence, raw_ptr(_render_state.dev_logical), &fence_create_info, nullptr, &fence);

    const FenceHandle fence_handle{ static_cast<uint32_t>(_work_queue.fences.size()) };
    _work_queue.fences.push_back(fence);
    _work_queue.packages.insert({ pkg_handle, WorkPackageTrackingInfo{ {}, fence_handle, cmd_buf_handle } });

    return WorkPackageSetup{ pkg_handle, cmd_buf };
}

tl::expected<StagingBuffer, VulkanError>
VulkanRenderer::create_staging_buffer(WorkPackageHandle pkg, const size_t bytes_size)
{
    WorkPackageTrackingInfo* trackinfo = &_work_queue.packages.find(pkg)->second;

    const VkBufferCreateInfo buffer_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = static_cast<VkDeviceSize>(bytes_size),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VkDevice device{ raw_ptr(_render_state.dev_logical) };
    VkBuffer staging_buffer{};
    const VkResult result = WRAP_VULKAN_FUNC(vkCreateBuffer, device, &buffer_create_info, nullptr, &staging_buffer);
    XR_VK_CHECK_RESULT(result);

    _work_queue.staging_buffers.emplace_back(staging_buffer);

    const VkBufferMemoryRequirementsInfo2 buf_mem_req_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        .pNext = nullptr,
        .buffer = staging_buffer,
    };

    VkMemoryRequirements2 buf_mem_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = nullptr,
    };
    vkGetBufferMemoryRequirements2(device, &buf_mem_req_info, &buf_mem_info);

    const VkMemoryAllocateFlagsInfo mem_alloc_flags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = 0,
        .deviceMask = 0,
    };

    const VkMemoryAllocateInfo mem_alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &mem_alloc_flags,
        .allocationSize = buf_mem_info.memoryRequirements.size,
        .memoryTypeIndex =
            find_allocation_memory_type(buf_mem_info.memoryRequirements.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    };

    VkDeviceMemory allocated_memory{};
    const VkResult mem_alloc_result =
        WRAP_VULKAN_FUNC(vkAllocateMemory, device, &mem_alloc_info, nullptr, &allocated_memory);

    XR_VK_CHECK_RESULT(mem_alloc_result);

    _work_queue.staging_memory.emplace_back(allocated_memory);

    const VkBindBufferMemoryInfo bind_memory_info{
        .sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
        .pNext = nullptr,
        .buffer = staging_buffer,
        .memory = allocated_memory,
        .memoryOffset = 0,
    };

    const VkResult bind_mem_result = WRAP_VULKAN_FUNC(vkBindBufferMemory2, device, 1, &bind_memory_info);
    XR_VK_CHECK_RESULT(bind_mem_result);

    return StagingBuffer{ staging_buffer, allocated_memory };
}

tl::expected<WorkPackageState, VulkanError>
WorkPackageState::create(VkDevice device, VkQueue queue, const uint32_t queue_family_idx)
{
    const VkCommandPoolCreateInfo cmd_pool_create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_idx,
    };

    VkCommandPool cmd_pool{};
    const VkResult cmd_pool_create_result =
        WRAP_VULKAN_FUNC(vkCreateCommandPool, device, &cmd_pool_create_info, nullptr, &cmd_pool);
    XR_VK_CHECK_RESULT(cmd_pool_create_result);

    return WorkPackageState{ device, queue, cmd_pool };
}

WorkPackageState::WorkPackageState(VkDevice dev, VkQueue q, VkCommandPool cmdpool)
    : device{ dev }
    , queue{ q }
    , cmd_pool{ cmdpool }
{
}

WorkPackageState::WorkPackageState(WorkPackageState&& rhs)
    : device{ rhs.device }
    , queue{ rhs.queue }
    , cmd_pool{ rhs.cmd_pool }
    , command_buffers{ std::move(rhs.command_buffers) }
    , staging_buffers{ std::move(rhs.staging_buffers) }
    , staging_memory{ std::move(rhs.staging_memory) }
{
    rhs.cmd_pool = VK_NULL_HANDLE;
}

WorkPackageState::~WorkPackageState()
{
    if (!command_buffers.empty()) {
        WRAP_VULKAN_FUNC(vkFreeCommandBuffers,
                         device,
                         cmd_pool,
                         static_cast<uint32_t>(command_buffers.size()),
                         command_buffers.data());
    }

    lz::chain(staging_buffers).forEach([d = this->device](VkBuffer buf) {
        WRAP_VULKAN_FUNC(vkDestroyBuffer, d, buf, nullptr);
    });

    lz::chain(staging_memory).forEach([d = this->device](VkDeviceMemory mem) {
        WRAP_VULKAN_FUNC(vkFreeMemory, d, mem, nullptr);
    });
}

void
VulkanRenderer::submit_work_package(const WorkPackageHandle pkg_handle)
{
    auto itr_pkg = _work_queue.packages.find(pkg_handle);
    assert(itr_pkg != std::cend(_work_queue.packages));

    WorkPackageTrackingInfo* track_info = &itr_pkg->second;

    const VkCommandBuffer cmd_buf = _work_queue.command_buffers[track_info->cmd_buf.value_of()];
    vkEndCommandBuffer(cmd_buf);

    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd_buf,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr,
    };

    WRAP_VULKAN_FUNC(vkQueueSubmit,
                     _render_state.queues[0].handle,
                     1,
                     &submit_info,
                     _work_queue.fences[track_info->fence.value_of()]);
}

void
VulkanRenderer::wait_on_packages(std::initializer_list<WorkPackageHandle> pkgs) const noexcept
{
#if 0
    itlib::small_vector<VkFence> fences{
        lz::chain(pkgs)
            .map([this](WorkPackageHandle pkg) {
                return _work_queue.fences[_work_queue.packages.find(pkg)->second.fence.value_of()];
            })
            .to<itlib::small_vector<VkFence>>(),
    };
#else
    itlib::small_vector<VkFence> fences{};
    for (const WorkPackageHandle pkg : pkgs) {
        fences.push_back(_work_queue.fences[_work_queue.packages.find(pkg)->second.fence.value_of()]);
    }
#endif

    WRAP_VULKAN_FUNC(vkWaitForFences,
                     raw_ptr(_render_state.dev_logical),
                     static_cast<uint32_t>(fences.size()),
                     fences.data(),
                     true,
                     0xffffffff);
}

void
VulkanRenderer::release_staging_resources()
{
}

void
VulkanRenderer::dbg_set_object_name(const uint64_t object,
                                    const VkDebugReportObjectTypeEXT object_type,
                                    const char* name) noexcept
{
    const VkDebugMarkerObjectNameInfoEXT obj_name{
        .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = object_type,
        .object = object,
        .pObjectName = name,
    };
    WRAP_VULKAN_FUNC(vkfn::DebugMarkerSetObjectNameEXT, raw_ptr(_render_state.dev_logical), &obj_name);
}

void
VulkanRenderer::dbg_marker_begin(VkCommandBuffer cmd_buf, const char* name, const rgb_color color) noexcept
{
    const VkDebugMarkerMarkerInfoEXT debug_marker{
        .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
        .pNext = nullptr,
        .pMarkerName = name,
        .color = { color.r, color.g, color.b, color.a },
    };

    vkfn::CmdDebugMarkerBeginEXT(cmd_buf, &debug_marker);
}

void
VulkanRenderer::dbg_marker_end(VkCommandBuffer cmd_buf) noexcept
{
    vkfn::CmdDebugMarkerEndEXT(cmd_buf);
}

void
VulkanRenderer::dbg_marker_insert(VkCommandBuffer cmd_buf, const char* name, const rgb_color color) noexcept
{
    const VkDebugMarkerMarkerInfoEXT debug_marker{
        .sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT,
        .pNext = nullptr,
        .pMarkerName = name,
        .color = { color.r, color.g, color.b, color.a },
    };

    vkfn::CmdDebugMarkerInsertEXT(cmd_buf, &debug_marker);
}

uint32_t
vk_format_bytes_size(const VkFormat format)
{
    uint32_t result = 0;
    switch (format) {
        case VK_FORMAT_UNDEFINED:
            result = 0;
            break;
        case VK_FORMAT_R4G4_UNORM_PACK8:
            result = 1;
            break;
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            result = 2;
            break;
        case VK_FORMAT_R8_UNORM:
            result = 1;
            break;
        case VK_FORMAT_R8_SNORM:
            result = 1;
            break;
        case VK_FORMAT_R8_USCALED:
            result = 1;
            break;
        case VK_FORMAT_R8_SSCALED:
            result = 1;
            break;
        case VK_FORMAT_R8_UINT:
            result = 1;
            break;
        case VK_FORMAT_R8_SINT:
            result = 1;
            break;
        case VK_FORMAT_R8_SRGB:
            result = 1;
            break;
        case VK_FORMAT_R8G8_UNORM:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SNORM:
            result = 2;
            break;
        case VK_FORMAT_R8G8_USCALED:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SSCALED:
            result = 2;
            break;
        case VK_FORMAT_R8G8_UINT:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SINT:
            result = 2;
            break;
        case VK_FORMAT_R8G8_SRGB:
            result = 2;
            break;
        case VK_FORMAT_R8G8B8_UNORM:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SNORM:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_USCALED:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SSCALED:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_UINT:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SINT:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8_SRGB:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_UNORM:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SNORM:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_USCALED:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SSCALED:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_UINT:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SINT:
            result = 3;
            break;
        case VK_FORMAT_B8G8R8_SRGB:
            result = 3;
            break;
        case VK_FORMAT_R8G8B8A8_UNORM:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SNORM:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_USCALED:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SSCALED:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_UINT:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SINT:
            result = 4;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_UNORM:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SNORM:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_USCALED:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SSCALED:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_UINT:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SINT:
            result = 4;
            break;
        case VK_FORMAT_B8G8R8A8_SRGB:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_R16_UNORM:
            result = 2;
            break;
        case VK_FORMAT_R16_SNORM:
            result = 2;
            break;
        case VK_FORMAT_R16_USCALED:
            result = 2;
            break;
        case VK_FORMAT_R16_SSCALED:
            result = 2;
            break;
        case VK_FORMAT_R16_UINT:
            result = 2;
            break;
        case VK_FORMAT_R16_SINT:
            result = 2;
            break;
        case VK_FORMAT_R16_SFLOAT:
            result = 2;
            break;
        case VK_FORMAT_R16G16_UNORM:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SNORM:
            result = 4;
            break;
        case VK_FORMAT_R16G16_USCALED:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SSCALED:
            result = 4;
            break;
        case VK_FORMAT_R16G16_UINT:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SINT:
            result = 4;
            break;
        case VK_FORMAT_R16G16_SFLOAT:
            result = 4;
            break;
        case VK_FORMAT_R16G16B16_UNORM:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SNORM:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_USCALED:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SSCALED:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_UINT:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SINT:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16_SFLOAT:
            result = 6;
            break;
        case VK_FORMAT_R16G16B16A16_UNORM:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SNORM:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_USCALED:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SSCALED:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_UINT:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SINT:
            result = 8;
            break;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            result = 8;
            break;
        case VK_FORMAT_R32_UINT:
            result = 4;
            break;
        case VK_FORMAT_R32_SINT:
            result = 4;
            break;
        case VK_FORMAT_R32_SFLOAT:
            result = 4;
            break;
        case VK_FORMAT_R32G32_UINT:
            result = 8;
            break;
        case VK_FORMAT_R32G32_SINT:
            result = 8;
            break;
        case VK_FORMAT_R32G32_SFLOAT:
            result = 8;
            break;
        case VK_FORMAT_R32G32B32_UINT:
            result = 12;
            break;
        case VK_FORMAT_R32G32B32_SINT:
            result = 12;
            break;
        case VK_FORMAT_R32G32B32_SFLOAT:
            result = 12;
            break;
        case VK_FORMAT_R32G32B32A32_UINT:
            result = 16;
            break;
        case VK_FORMAT_R32G32B32A32_SINT:
            result = 16;
            break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            result = 16;
            break;
        case VK_FORMAT_R64_UINT:
            result = 8;
            break;
        case VK_FORMAT_R64_SINT:
            result = 8;
            break;
        case VK_FORMAT_R64_SFLOAT:
            result = 8;
            break;
        case VK_FORMAT_R64G64_UINT:
            result = 16;
            break;
        case VK_FORMAT_R64G64_SINT:
            result = 16;
            break;
        case VK_FORMAT_R64G64_SFLOAT:
            result = 16;
            break;
        case VK_FORMAT_R64G64B64_UINT:
            result = 24;
            break;
        case VK_FORMAT_R64G64B64_SINT:
            result = 24;
            break;
        case VK_FORMAT_R64G64B64_SFLOAT:
            result = 24;
            break;
        case VK_FORMAT_R64G64B64A64_UINT:
            result = 32;
            break;
        case VK_FORMAT_R64G64B64A64_SINT:
            result = 32;
            break;
        case VK_FORMAT_R64G64B64A64_SFLOAT:
            result = 32;
            break;
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            result = 4;
            break;
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            result = 4;
            break;

        default:
            assert(false && "Unhandled VK_FORMAT");
            break;
    }
    return result;
}

} // namespace xray::rendering
