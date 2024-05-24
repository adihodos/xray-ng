#include "xray/rendering/renderer.vulkan/renderer.vulkan.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#include <fmt/core.h>
#include <itlib/small_vector.hpp>
#include <pipes/pipes.hpp>
#include <tl/optional.hpp>
#include <swl/variant.hpp>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include "xray/base/fnv_hash.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/rangeless/fn.hpp"
#include "xray/rendering/renderer.vulkan/vulkan.call.wrapper.hpp"
#include "xray/rendering/renderer.vulkan/vulkan.dynamic.dispatch.hpp"
#include "xray/rendering/renderer.vulkan/vulkan.pretty.print.hpp"
#include "xray/rendering/renderer.vulkan/vulkan.unique.resource.hpp"
#include "xray/rendering/renderer.vulkan/vulkan.window.platform.data.hpp"

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

template<typename Container, typename BinaryPredicate>
tl::optional<size_t>
r_find_pos(const Container& c, BinaryPredicate bp) noexcept
{
    const auto itr = find_if(cbegin(c), cend(c), bp);
    if (itr != cend(c)) {
        return tl::optional{ distance(cbegin(c), itr) };
    }
    return tl::nullopt;
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
                                              msg_buf.size(),
                                              "[vulkan]: {} :: {:#x} - {}",
                                              pCallbackData->pMessageIdName,
                                              pCallbackData->messageIdNumber,
                                              pCallbackData->pMessage);

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

using PresentToSurface = std::variant<PresentToWindowSurface, PresentToDisplaySurface>;

struct Presentation
{
    PresentToSurface surface;
};

tl::optional<PresentToSurface>
create_xcb_surface(const WindowPlatformDataXcb& win_platform_data, VkInstance instance)
{
    const VkXcbSurfaceCreateInfoKHR surface_create_info = { .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
                                                            .pNext = nullptr,
                                                            .flags = 0,
                                                            .connection = win_platform_data.connection,
                                                            .window = win_platform_data.window };

    xrUniqueVkSurfaceKHR surface_khr{ [instance, &surface_create_info]() {
                                         VkSurfaceKHR surface{};
                                         WRAP_VULKAN_FUNC(
                                             vkCreateXcbSurfaceKHR, instance, &surface_create_info, nullptr, &surface);
                                         return surface;
                                     }(),
                                      VkResourceDeleter_VkSurfaceKHR{ instance } };

    if (!surface_khr)
        return tl::nullopt;

    XR_LOG_INFO("Surface (XCB) created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(surface_khr)));
    return tl::make_optional<PresentToSurface>(PresentToWindowSurface{ win_platform_data, std::move(surface_khr) });
}

tl::optional<PresentToSurface>
create_xlib_surface(const WindowPlatformDataXlib& win_platform_data, VkInstance instance)
{
    const VkXlibSurfaceCreateInfoKHR surface_create_info = { .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                                                             .pNext = nullptr,
                                                             .flags = 0,
                                                             .dpy = win_platform_data.display,
                                                             .window = win_platform_data.window };

    xrUniqueVkSurfaceKHR surface_khr{ [&]() {
                                         VkSurfaceKHR surface{};
                                         WRAP_VULKAN_FUNC(
                                             vkCreateXlibSurfaceKHR, instance, &surface_create_info, nullptr, &surface);
                                         return surface;
                                     }(),
                                      VkResourceDeleter_VkSurfaceKHR{ instance } };

    if (!surface_khr)
        return tl::nullopt;

    XR_LOG_INFO("Surface created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(surface_khr)));
    return tl::make_optional<PresentToSurface>(PresentToWindowSurface{ win_platform_data, std::move(surface_khr) });
}

tl::optional<VulkanRenderer>
VulkanRenderer::create(const WindowPlatformData& win_data)
{
    const itlib::small_vector<VkExtensionProperties, 4> supported_extensions{ enumerate_instance_extensions() };
    supported_extensions >>= pipes::for_each(
        [](const VkExtensionProperties& e) { XR_LOG_INFO("extension: {} - {:#0x}", e.extensionName, e.specVersion); });

    const small_vec_4<const char*> extensions_list{ [&supported_extensions]() {
        small_vec_4<const char*> exts_list{
            VK_KHR_SURFACE_EXTENSION_NAME,     VK_KHR_XLIB_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        };

        static constexpr const initializer_list<const char*> display_extensions_list = {
            VK_KHR_DISPLAY_EXTENSION_NAME, VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME
        };

        display_extensions_list >>= pipes::filter([&supported_extensions](const char* ext_name) {
            return supported_extensions % fn::exists_where([ext_name](const VkExtensionProperties& e) {
                       return strcmp(e.extensionName, ext_name) == 0;
                   });
        }) >>= pipes::push_back(exts_list);

        return exts_list;
    }() };

    const VkApplicationInfo app_info = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                         .pNext = nullptr,
                                         .pApplicationName = "xray-ng-app",
                                         .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                         .pEngineName = "xray-engine",
                                         .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                         .apiVersion = VK_API_VERSION_1_3 };

    constexpr const char* const kRequiredLayers[] = {
        "VK_LAYER_KHRONOS_validation"
        // "VK_LAYER_LUNARG_standard_validation"
    };

    const VkDebugUtilsMessengerCreateInfoEXT dbg_utils_msg_create_ext = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = log_vk_debug_output,
        .pUserData = nullptr
    };

    const VkInstanceCreateInfo instance_create_info = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                                        .pNext = &dbg_utils_msg_create_ext,
                                                        .flags = 0,
                                                        .pApplicationInfo = &app_info,
                                                        .enabledLayerCount =
                                                            static_cast<uint32_t>(std::size(kRequiredLayers)),
                                                        .ppEnabledLayerNames = kRequiredLayers,
                                                        .enabledExtensionCount =
                                                            static_cast<uint32_t>(std::size(extensions_list)),
                                                        .ppEnabledExtensionNames = extensions_list.data() };

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

    struct PhysicalDeviceData
    {
        VkPhysicalDevice device;
        struct
        {
            VkPhysicalDeviceProperties2 base;
            VkPhysicalDeviceVulkan11Properties vk11;
            VkPhysicalDeviceVulkan12Properties vk12;
            VkPhysicalDeviceVulkan13Properties vk13;
        } properties;
        struct
        {
            VkPhysicalDeviceFeatures2 base;
            VkPhysicalDeviceVulkan11Features vk11;
            VkPhysicalDeviceVulkan12Features vk12;
            VkPhysicalDeviceVulkan13Features vk13;
        } features;

        VkPhysicalDeviceMemoryProperties memory;
        vector<VkQueueFamilyProperties> queue_props;

        explicit PhysicalDeviceData(VkPhysicalDevice dev)
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

        PhysicalDeviceData(const PhysicalDeviceData& rhs)
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

        PhysicalDeviceData& operator=(const PhysicalDeviceData& rhs)
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
    };

    auto check_physical_device_presentation_surface_support = [](const WindowPlatformData& win_data,
                                                                 VkInstance instance,
                                                                 VkPhysicalDevice device,
                                                                 const uint32_t queue_index) {
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

        assert(false && "Unhandled WindowPlatformData!");
        return false;
    };

    small_vec_4<tuple<PhysicalDeviceData, size_t, size_t>> phys_devices_data{};

    phys_devices >>= pipes::transform([instance = raw_ptr(vkinstance),
                                       &win_data,
                                       check_physical_device_presentation_surface_support](VkPhysicalDevice phys_device)
                                          -> tl::optional<tuple<PhysicalDeviceData, size_t, size_t>> {
        const PhysicalDeviceData pdd{ phys_device };

        XR_LOG_INFO("Checking device {} suitability ...", pdd.properties.base.properties.deviceName);

        constexpr const uint32_t suitable_device_types[] = { VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
                                                             VK_PHYSICAL_DEVICE_TYPE_CPU,
                                                             VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU };

        const bool is_requested_device_type{
            suitable_device_types % fn::exists_where([device_type = pdd.properties.base.properties.deviceType](
                                                         const uint32_t required) { return required == device_type; })
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
            XR_LOG_INFO("Rejecting device {}, no graphics queue support.", pdd.properties.base.properties.deviceName);
            return tl::nullopt;
        }

        // surface presentation support

        if (!check_physical_device_presentation_surface_support(win_data, instance, pdd.device, *graphics_q_idx)) {
            XR_LOG_INFO("Rejecting device {}, no surface presentation support (queue family index {})",
                        pdd.properties.base.properties.deviceName,
                        *graphics_q_idx);
            return tl::nullopt;
        }

        const auto transfer_q_idx = r_find_pos(
            pdd.queue_props, [](const VkQueueFamilyProperties& q) { return q.queueFlags & VK_QUEUE_TRANSFER_BIT; });

        if (!transfer_q_idx) {
            XR_LOG_INFO("Rejecting device {}, no graphics queue support.", pdd.properties.base.properties.deviceName);
            return tl::nullopt;
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
    }) >>=
        pipes::filter([](tl::optional<tuple<PhysicalDeviceData, size_t, size_t>> data) { return data.has_value(); }) >>=
        pipes::for_each([&phys_devices_data](tl::optional<tuple<PhysicalDeviceData, size_t, size_t>> pd) mutable {
            pd.take().map([&phys_devices_data](tuple<PhysicalDeviceData, size_t, size_t>&& pd) mutable {
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

    if (queue_graphics == queue_transfer) {
        queue_create_info[0].queueCount += 1;
    } else {
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
    };

    const VkDeviceCreateInfo device_create_info = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                                    .pNext = &phys_device.features.base,
                                                    .flags = 0,
                                                    .queueCreateInfoCount =
                                                        static_cast<uint32_t>(size(queue_create_info)),
                                                    .pQueueCreateInfos = queue_create_info.data(),
                                                    .enabledLayerCount = 0,
                                                    .ppEnabledLayerNames = nullptr,
                                                    .enabledExtensionCount =
                                                        static_cast<uint32_t>(size(device_extensions)),
                                                    .ppEnabledExtensionNames = device_extensions.begin(),
                                                    .pEnabledFeatures = nullptr };

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
    vkGetDeviceQueue(raw_ptr(logical_device), queue_transfer, (queue_transfer == queue_graphics), &queues[1]);

    queues % fn::for_each([](const VkQueue q) { XR_LOG_INFO("Queue {:#x}", (uintptr_t)q); });

    XR_LOG_INFO("Device created successfully");

    tl::optional<PresentToDisplaySurface> present_to_display{ [&extensions_list,
                                                               pd = phys_device.device,
                                                               instance = raw_ptr(vkinstance)]()
                                                                  -> tl::optional<PresentToDisplaySurface> {
        const bool has_display_ext = ranges::any_of(
            extensions_list, [](const char* e) { return strcmp(e, VK_KHR_DISPLAY_EXTENSION_NAME) == 0; });

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
                WRAP_VULKAN_FUNC(
                    vkGetDisplayModePropertiesKHR, pd, dpy_props.display, &modes_count, display_modes.data());

                attached_displays.emplace_back(dpy_props, std::move(display_modes));
            }

            return attached_displays;
        }() };

        if (attached_displays.empty())
            return tl::nullopt;

        attached_displays >>= pipes::for_each([](const DisplayData& dp) {
            XR_LOG_INFO("display : {}, physical dimension {}, physical resolution {}",
                        dp.properties.displayName,
                        dp.properties.physicalResolution,
                        dp.properties.physicalDimensions);

            for (const VkDisplayModePropertiesKHR& disp_mode : dp.display_modes) {
                XR_LOG_INFO("mode:: refresh rate {} Hz, visible region {}",
                            disp_mode.parameters.refreshRate,
                            disp_mode.parameters.visibleRegion);
            }
        });

        //
        // get list of display planes for this device
        const vector<DisplayPlaneData> display_plane_data{ [pd,
                                                            display_properties =
                                                                &attached_displays.front().properties]() {
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
                    WRAP_VULKAN_FUNC(vkGetDisplayPlaneCapabilitiesKHR,
                                     pd,
                                     dpm.displayMode,
                                     plane_index,
                                     &display_plane_capabilities);
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

        return best_choice.and_then([&attached_displays, &display_plane_data, instance](
                                        const DisplayDataDisplayPlaneCapsModeIndex& dddpi)
                                        -> tl::optional<PresentToDisplaySurface> {
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
                .imageExtent = dddpi.display_plane_caps.maxDstExtent
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
    }() };

    tl::optional<PresentToSurface> present_to_surface{ [&win_data, instance = raw_ptr(vkinstance)]() {
        if (const WindowPlatformDataXlib* xlib = swl::get_if<WindowPlatformDataXlib>(&win_data)) {
            return create_xlib_surface(*xlib, instance);
        }

        if (const WindowPlatformDataXcb* xcb = swl::get_if<WindowPlatformDataXcb>(&win_data)) {
            return create_xcb_surface(*xcb, instance);
        }

        assert(false && "Unhandled WindowPlatformData");
        return tl::optional<PresentToSurface>{};
    }() };

    if (!present_to_surface) {
        XR_LOG_CRITICAL("Cannot create a surface of any kind (display/window)");
        return tl::nullopt;
    }

    present_to_surface.map([phys_device, vkdev = raw_ptr(logical_device)](const PresentToSurface& ps) {
        VkSurfaceKHR surface = visit(
            [](auto&& var_arg) -> VkSurfaceKHR {
                using var_type = decay_t<decltype(var_arg)>;

                if constexpr (is_same_v<PresentToWindowSurface, var_type>) {
                    return raw_ptr(var_arg.surface);
                } else if constexpr (is_same_v<PresentToDisplaySurface, var_type>) {
                    return raw_ptr(var_arg.display_surface);
                } else {
                    static_assert(variant_type_is_not_handled<var_type>::error, "Non-exhaustive visitor!");
                }
            },
            ps);

        VkSurfaceCapabilitiesKHR surface_caps;
        if (const VkResult res =
                WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, phys_device.device, surface, &surface_caps);
            res != VK_SUCCESS) {
        }

        XR_LOG_INFO("Surface capabilities: {}", surface_caps);

        small_vec_4<VkSurfaceFormatKHR> supported_surface_fmts{ [device = phys_device.device, surface]() {
            small_vec_4<VkSurfaceFormatKHR> fmts;
            uint32_t fmts_count{};
            WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface, &fmts_count, nullptr);
            if (fmts_count) {
                fmts.resize(fmts_count);
                WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR, device, surface, &fmts_count, fmts.data());
            }

            return fmts;
        }() };

        supported_surface_fmts >>= pipes::for_each([](const VkSurfaceFormatKHR& fmt) {
            XR_LOG_INFO("{}", vk::to_string(static_cast<vk::Format>(fmt.format)));
        });

        const small_vec_4<VkPresentModeKHR> supported_presentation_modes{ [device = phys_device.device, surface]() {
            uint32_t present_modes_count{};
            WRAP_VULKAN_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface, &present_modes_count, nullptr);

            small_vec_4<VkPresentModeKHR> present_modes;
            present_modes.resize(present_modes_count);
            WRAP_VULKAN_FUNC(
                vkGetPhysicalDeviceSurfacePresentModesKHR, device, surface, &present_modes_count, present_modes.data());

            return present_modes;
        }() };

        XR_LOG_INFO("{}", supported_presentation_modes % fn::transform([](const VkPresentModeKHR pm) {
                              return fmt::format("{:#x} -> {} ",
                                                 static_cast<uint32_t>(pm),
                                                 vk::to_string(static_cast<vk::PresentModeKHR>(pm)));
                          }) % fn::foldl(string{ "supported presentation modes: " }, plus<string>{}));

        constexpr const VkPresentModeKHR preferred_presentation_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR,
                                                                            VK_PRESENT_MODE_IMMEDIATE_KHR,
                                                                            VK_PRESENT_MODE_FIFO_KHR };

        const auto best_preferred_supported_mode =
            ranges::find_first_of(preferred_presentation_modes, supported_presentation_modes);
        if (best_preferred_supported_mode == cend(supported_presentation_modes)) {
            XR_LOG_CRITICAL("None of the preferred presentation modes is suppored!");
        }

        XR_LOG_INFO("best preferred supported present mode is {:#x} -> {}",
                    static_cast<uint32_t>(*best_preferred_supported_mode),
                    vk::to_string(static_cast<vk::PresentModeKHR>(*best_preferred_supported_mode)));

        const uint32_t swapchain_image_count = min(surface_caps.minImageCount + 2, surface_caps.maxImageCount);

        const VkSwapchainCreateInfoKHR swapchain_create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .surface = surface,
            .minImageCount = swapchain_image_count,
            .imageFormat = supported_surface_fmts[0].format,
            .imageColorSpace = supported_surface_fmts[0].colorSpace,
            .imageExtent = surface_caps.maxImageExtent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .preTransform = surface_caps.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = *best_preferred_supported_mode,
            .clipped = false,
            .oldSwapchain = nullptr,
        };

        xrUniqueVkSwapchainKHR swapchain{
            [vkdev, &swapchain_create_info]() {
                VkSwapchainKHR swapchain{};
                WRAP_VULKAN_FUNC(vkCreateSwapchainKHR, vkdev, &swapchain_create_info, nullptr, &swapchain);
                return swapchain;
            }(),
            VkResourceDeleter_VkSwapchainKHR{ vkdev },
        };

        XR_LOG_INFO("Swapchain created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(swapchain)));

        small_vec_4<VkImage> swapchain_images{ [&]() {
            uint32_t swapchain_image_count{};
            WRAP_VULKAN_FUNC(vkGetSwapchainImagesKHR, vkdev, raw_ptr(swapchain), &swapchain_image_count, nullptr);

            small_vec_4<VkImage> swapchain_images{ swapchain_image_count };
            WRAP_VULKAN_FUNC(
                vkGetSwapchainImagesKHR, vkdev, raw_ptr(swapchain), &swapchain_image_count, swapchain_images.data());

            return swapchain_images;
        }() };

        const small_vec_4<xrUniqueVkImageView> swapchain_imageviews{
            swapchain_images %
            fn::transform([vkdev, image_format = supported_surface_fmts[0].format](VkImage swapchain_image) {
                const VkImageViewCreateInfo image_view_create_info = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext = nullptr,
                    .flags = 0,
                    .image = swapchain_image,
                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                    .format = image_format,
                    .components = VkComponentMapping{},
                    .subresourceRange = VkImageSubresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                 .baseMipLevel = 0,
                                                                 .levelCount = 1,
                                                                 .baseArrayLayer = 0,
                                                                 .layerCount = 1 }
                };

                VkImageView swapchain_image_view{};
                WRAP_VULKAN_FUNC(vkCreateImageView, vkdev, &image_view_create_info, nullptr, &swapchain_image_view);

                return xrUniqueVkImageView{ swapchain_image_view, VkResourceDeleter_VkImageView{ vkdev } };
            }) %
            fn::to(small_vec_4<xrUniqueVkImageView>{})
        };

        const bool some_failed =
            swapchain_imageviews % fn::exists_where([](const xrUniqueVkImageView& image_view) { return !image_view; });

        XR_LOG_INFO("Swapchain image views creation status : {}", !some_failed);
    });

    //
    return tl::nullopt;
}

} // namespace
