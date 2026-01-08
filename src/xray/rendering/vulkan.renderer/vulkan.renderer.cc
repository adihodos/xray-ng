#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <string>
#include <tuple>
#include <vector>
#include <mutex>
#include <stacktrace>

#include <fmt/core.h>
#include <tl/optional.hpp>
#include <swl/variant.hpp>
#include <mio/mmap.hpp>

#include <Lz/algorithm/find_if.hpp>
#include <Lz/algorithm/transform.hpp>
#include <Lz/algorithm/index_of_if.hpp>
#include <Lz/filter.hpp>
#include <Lz/map.hpp>
#include <Lz/procs/to.hpp>

#include <tracy/Tracy.hpp>

#if defined(XRAY_OS_IS_WINDOWS)
#include <vulkan/vulkan_win32.h>
#elif defined(XRAY_OS_IS_POSIX_FAMILY)
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#else
#error "Unsupported OS"
#endif

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include "xray/base/xray.misc.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/variant.helpers.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/rangeless/fn.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/thread.local.context.hpp"
#include "xray/base/containers/arena.string.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/containers/arena.unorderered_map.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.call.wrapper.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatch.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.window.platform.data.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pretty.print.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.config.hpp"

#define XRAY_MK1(tok) XRAY_STRINGIZE_a(tok)
#define XRAY_MK0(a, b) XRAY_MK1(a##b)
#define XR_MAKE_VK_FUNC(name) XRAY_MK0(vk, name)

using namespace xray::base;
using namespace std;

namespace fn = rangeless::fn;
using fn::operators::operator%;
using fn::operators::operator%=;

template <typename T>
struct variant_type_is_not_handled {};

template <typename FwdIter, typename BinaryPredicate>
tl::optional<size_t> r_find_pos(FwdIter first, FwdIter last, BinaryPredicate bp) noexcept {
	const auto itr = find_if(first, last, bp);
	if (itr != last) {
		return tl::optional<size_t>{distance(first, itr)};
	}
	return tl::nullopt;
}

template <typename Container, typename BinaryPredicate>
tl::optional<size_t> r_find_pos(const Container& c, BinaryPredicate bp) noexcept {
	return r_find_pos(cbegin(c), cend(c), bp);
}

template <typename ElementType, typename BinaryPredicate>
tl::optional<size_t> r_find_pos(std::span<ElementType> s, BinaryPredicate bp) noexcept {
	return r_find_pos(cbegin(s), cend(s), bp);
}

namespace std {
template <>
struct hash<VkExtensionProperties> {
	size_t operator()(const VkExtensionProperties& e) const noexcept { return FNV::fnv1a_unrolled<2>(&e, sizeof(e)); }
};

}  // namespace std

inline bool operator==(const VkExtensionProperties& ea, const VkExtensionProperties& eb) noexcept {
	return strcmp(ea.extensionName, eb.extensionName) == 0 && ea.specVersion == eb.specVersion;
}

namespace xray::rendering {

namespace details {

template <typename StructType>
concept VulkanChainableType = requires(StructType chained_struct) {
	{ chained_struct.pNext };
	{ std::is_same_v<decltype(chained_struct.pNext), void*> };
};

template <VulkanChainableType... VulkanChainedStructs>
void chain_structs(VulkanChainedStructs&... chained_structs) {
	void* next_chained = nullptr;
	(
		[&next_chained](auto&& current_struct) {
			current_struct.pNext = next_chained;
			next_chained		 = &current_struct;
		}(std::forward<VulkanChainedStructs>(chained_structs)),
		...
	);
}

bool vk_renderer_check_physical_device_presentation_surface_support(
	const WindowPlatformData& win_data, VkInstance instance, VkPhysicalDevice device, const uint32_t queue_index
) {
#if defined(XRAY_OS_IS_WINDOWS)
	auto get_win32_presentation_support = reinterpret_cast<PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR>(
		vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR")
	);

	assert(get_win32_presentation_support != nullptr);
	return get_win32_presentation_support(device, queue_index);
#else
	if (const WindowPlatformDataXlib* xlib = swl::get_if<WindowPlatformDataXlib>(&win_data)) {
		PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR get_physical_device_xlib_presentation_support_khr =
			reinterpret_cast<PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR>(
				vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR")
			);

		assert(get_physical_device_xlib_presentation_support_khr != nullptr);
		return get_physical_device_xlib_presentation_support_khr(
				   device, queue_index, reinterpret_cast<Display*>(xlib->display), xlib->visual
			   ) == VK_TRUE;
	}

	if (const WindowPlatformDataXcb* xcb = swl::get_if<WindowPlatformDataXcb>(&win_data)) {
		PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR get_physical_device_xcb_presentation_support_khr =
			reinterpret_cast<PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR>(
				vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR")
			);

		assert(get_physical_device_xcb_presentation_support_khr != nullptr);
		return get_physical_device_xcb_presentation_support_khr(
				   device, queue_index, reinterpret_cast<xcb_connection_t*>(xcb->connection), xcb->visual
			   ) == VK_TRUE;
	}

	XR_LOG_ERR("Unsupported Windowing system ...");
	return false;
#endif
}
}  // namespace details

struct VulkanInstanceExtensionTag {};
struct VulkanDeviceExtensionTag {};

template <typename VkFnProto, typename VkProcAddrHolder>
VkFnProto load_vulkan_proc(VkProcAddrHolder proc_provider, const char* proc_name) {
	using provider_type = std::remove_cvref_t<decltype(proc_provider)>;
	if constexpr (std::is_same_v<VkInstance, provider_type>) {
		auto inst_proc_addr = reinterpret_cast<VkFnProto>(vkGetInstanceProcAddr(proc_provider, proc_name));
		XR_LOG_INFO("Request for instance proc {} resolved @ {:p}", proc_name, fmt::ptr(inst_proc_addr));
		return inst_proc_addr;
	} else if constexpr (std::is_same_v<VkDevice, provider_type>) {
		auto device_proc_addr = reinterpret_cast<VkFnProto>(vkGetDeviceProcAddr(proc_provider, proc_name));
		XR_LOG_INFO("Request for device proc {} resolved @ {:p}", proc_name, fmt::ptr(device_proc_addr));
		return device_proc_addr;
	} else {
		static_assert(false && "Wrong argument passed for proc address resolver.");
	}
}

#define PFN_LIST_ENTRY(fnproto, name, tag) fnproto vkfn::name{nullptr};
#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatched.functions.hpp"
FUNCTION_POINTERS_LIST
#undef PFN_LIST_ENTRY

std::string_view format_vk_func_fail(
	const char* file, const int32_t line, const char* vkfunc_name, const VkResult result
) {
	thread_local static array<char, 2048> scratch_buffer;
	const auto [itr, cch] = fmt::format_to_n(
		scratch_buffer.data(),
		scratch_buffer.size(),
		"[{}:{}] vulkan api: {} failed, error {:#0x} ({})",
		file,
		line,
		vkfunc_name,
		static_cast<uint32_t>(result),
		vk::to_string(static_cast<vk::Result>(result))
	);

	scratch_buffer[cch > 0 ? cch : 0] = 0;
	return string_view{scratch_buffer.data(), static_cast<size_t>(cch)};
}

namespace details {

thread_local std::byte kTisButALocalScratch[xray::base::megabytes(1)];

VkBool32 r_vk_debug_msg_output(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
) {
	xray::base::MemoryArena scratch_arena{kTisButALocalScratch};
	xray::base::containers::string dbg_str{scratch_arena};

	dbg_str = "[Vulkan]";
	struct SeverityFlagWithName {
		VkDebugUtilsMessageSeverityFlagBitsEXT flag;
		const char* name;
		xray::base::LogLevel log_level;
	};

	constexpr SeverityFlagWithName severity_flags_names[] = {
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT, "VERBOSE", xray::base::LogLevel::Trace},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT, "INFO", xray::base::LogLevel::Debug},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT, "WARNING", xray::base::LogLevel::Warn},
		{VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, "ERROR", xray::base::LogLevel::Err},
	};

	LogLevel log_level = xray::base::LogLevel::Trace;
	for (const auto [flag, name, level] : severity_flags_names) {
		if (message_severity & flag) {
			fmt::format_to(std::back_inserter(dbg_str), "[{}]", name);
			log_level = level;
		}
	}

	struct MessageTypeFlagWithName {
		VkDebugUtilsMessageTypeFlagsEXT flag;
		const char* name;
	};

	constexpr MessageTypeFlagWithName msg_type_flags_names[] = {
		{VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT, "GENERAL"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT, "VALIDATION"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, "PERFORMANCE"},
		{VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT, "ADDRESS BINDING"},
	};

	for (const auto [flag, name] : msg_type_flags_names) {
		if (messageTypes & flag) {
			fmt::format_to(std::back_inserter(dbg_str), "[{}]", name);
		}
	}

	std::format_to(
		std::back_inserter(dbg_str),
		"[{}][{}]: {}\n",
		pCallbackData->pMessageIdName,
		pCallbackData->messageIdNumber,
		pCallbackData->pMessage
	);

	xray::base::log_fwd(log_level, "{}", dbg_str);

	const uint32_t kSeverityStackTraces = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
		// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		;

	if (message_severity & kSeverityStackTraces) {
		using stacktrace_allocator_type = xray::base::MemoryArenaAllocator<std::stacktrace_entry>;
		using vk_stacktrace				= std::basic_stacktrace<stacktrace_allocator_type>;
		const auto stack_trace =
			vk_stacktrace::current(xray::base::MemoryArenaAllocator<std::stacktrace_entry>{scratch_arena});

		dbg_str.clear();
		for (const std::stacktrace_entry& e : stack_trace) {
			fmt::format_to(
				std::back_inserter(dbg_str), "{}:{} {}\n", e.source_file(), e.source_line(), e.description()
			);
		}
		xray::base::log_fwd(log_level, "{}", dbg_str);
	}
	return VK_FALSE;
}

}  // namespace details

struct PresentToWindowSurface {
	WindowPlatformData window_data;
	xrUniqueVkSurfaceKHR surface;
};

struct DisplayData {
	VkDisplayPropertiesKHR properties;
	vector<VkDisplayModePropertiesKHR> display_modes;
};

struct DisplayPlaneData {
	uint32_t index;
	VkDisplayPlanePropertiesKHR properties;
	vector<VkDisplayKHR> supported_displays;
};

struct PresentToDisplaySurface {
	DisplayData display_data;
	DisplayPlaneData display_plane_data;
	VkDisplayPlaneCapabilitiesKHR display_plane_caps;
	xrUniqueVkSurfaceKHR display_surface;
};

using PresentToSurface = swl::variant<PresentToWindowSurface, PresentToDisplaySurface>;

tl::optional<PresentToDisplaySurface> check_display_presentation_support(
	span<const char*> extensions_list, VkPhysicalDevice pd, VkInstance instance
) {
	const bool has_display_ext =
		ranges::any_of(extensions_list, [](const char* e) { return strcmp(e, VK_KHR_DISPLAY_EXTENSION_NAME) == 0; });

	if (!has_display_ext) {
		return tl::nullopt;
	}

	//
	// get list of attached displays
	const vector<DisplayData> attached_displays{[pd]() {
		uint32_t count{};
		WRAP_VULKAN_FUNC(vkGetPhysicalDeviceDisplayPropertiesKHR, pd, &count, nullptr);

		vector<VkDisplayPropertiesKHR> dpys;
		dpys.resize(count);
		WRAP_VULKAN_FUNC(vkGetPhysicalDeviceDisplayPropertiesKHR, pd, &count, dpys.data());

		vector<DisplayData> attached_displays{};
		for (const VkDisplayPropertiesKHR& dpy_props : dpys) {
			uint32_t modes_count{};
			WRAP_VULKAN_FUNC(vkGetDisplayModePropertiesKHR, pd, dpy_props.display, &modes_count, nullptr);

			if (!modes_count) continue;

			vector<VkDisplayModePropertiesKHR> display_modes{modes_count};
			WRAP_VULKAN_FUNC(vkGetDisplayModePropertiesKHR, pd, dpy_props.display, &modes_count, display_modes.data());

			attached_displays.emplace_back(dpy_props, std::move(display_modes));
		}

		return attached_displays;
	}()};

	if (attached_displays.empty()) return tl::nullopt;

	for (const DisplayData& dp : attached_displays) {
		XR_LOG_INFO(
			"display : {}, physical dimension {}, physical resolution {}",
			dp.properties.displayName,
			dp.properties.physicalResolution,
			dp.properties.physicalDimensions
		);

		for (const VkDisplayModePropertiesKHR& disp_mode : dp.display_modes) {
			XR_LOG_INFO(
				"mode:: refresh rate {} Hz, visible region {}",
				disp_mode.parameters.refreshRate,
				disp_mode.parameters.visibleRegion
			);
		}
	}

	//
	// get list of display planes for this device
	const vector<DisplayPlaneData> display_plane_data{[pd,
													   display_properties = &attached_displays.front().properties]() {
		uint32_t dpy_plane_count{};
		WRAP_VULKAN_FUNC(vkGetPhysicalDeviceDisplayPlanePropertiesKHR, pd, &dpy_plane_count, nullptr);

		vector<VkDisplayPlanePropertiesKHR> display_plane_properties{dpy_plane_count};
		WRAP_VULKAN_FUNC(
			vkGetPhysicalDeviceDisplayPlanePropertiesKHR, pd, &dpy_plane_count, display_plane_properties.data()
		);

		vector<DisplayPlaneData> display_plane_data{};
		for (uint32_t plane_index = 0; plane_index < static_cast<uint32_t>(display_plane_properties.size());
			 ++plane_index) {
			const VkDisplayPlanePropertiesKHR& dpp = display_plane_properties[plane_index];

			uint32_t supported_displays_count{};
			WRAP_VULKAN_FUNC(
				vkGetDisplayPlaneSupportedDisplaysKHR, pd, plane_index, &supported_displays_count, nullptr
			);
			vector<VkDisplayKHR> supported_displays{supported_displays_count};
			WRAP_VULKAN_FUNC(
				vkGetDisplayPlaneSupportedDisplaysKHR,
				pd,
				plane_index,
				&supported_displays_count,
				supported_displays.data()
			);

			if (supported_displays_count) {
				display_plane_data.emplace_back(plane_index, dpp, std::move(supported_displays));
			}
		}

		return display_plane_data;
	}()};

	if (display_plane_data.empty()) return tl::nullopt;

	//
	// find the best display plane
	struct DisplayDataDisplayPlaneCapsModeIndex {
		size_t display_data_idx;
		size_t display_plane_idx;
		size_t display_mode_idx;
		VkDisplayPlaneCapabilitiesKHR display_plane_caps;
	};

	tl::optional<DisplayDataDisplayPlaneCapsModeIndex> best_choice{};

	for (const DisplayPlaneData& dppd : display_plane_data) {
		XR_LOG_INFO(
			"plane: display {:#x}, stack index: {}",
			reinterpret_cast<uintptr_t>(dppd.properties.currentDisplay),
			dppd.properties.currentStackIndex
		);

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
			fn::transform([display_mode_idx = 0u, pd, plane_index = dppd.index](const VkDisplayModePropertiesKHR& dpm
						  ) mutable {
				VkDisplayPlaneCapabilitiesKHR display_plane_capabilities;
				WRAP_VULKAN_FUNC(
					vkGetDisplayPlaneCapabilitiesKHR, pd, dpm.displayMode, plane_index, &display_plane_capabilities
				);
				return make_tuple(display_mode_idx + 1, display_plane_capabilities);
			}) %
			fn::foldl(
				tuple<uint32_t, VkDisplayPlaneCapabilitiesKHR>{},
				[](const tuple<uint32_t, VkDisplayPlaneCapabilitiesKHR>& out,
				   const tuple<uint32_t, VkDisplayPlaneCapabilitiesKHR>& in) {
					const auto& [out_idx, out_caps] = out;
					const auto& [in_idx, in_caps]	= in;

					if (in_caps.maxDstExtent.width > out_caps.maxDstExtent.width &&
						in_caps.maxDstExtent.height > out_caps.maxDstExtent.height)
						return in;

					return out;
				}
			);

		best_choice = best_choice.take().map_or_else(
			[display_data_idx, display_mode_idx, display_plane_idx = dppd.index, display_plane_caps](
				DisplayDataDisplayPlaneCapsModeIndex dddpi
			) {
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
			}
		);
	}

	return best_choice.and_then(
		[&attached_displays, &display_plane_data, instance](const DisplayDataDisplayPlaneCapsModeIndex& dddpi
		) -> tl::optional<PresentToDisplaySurface> {
			XR_LOG_INFO(
				"capabilities: min dst pos {}, max dst pos {}, min dst extent {}, max dst extent {}",
				dddpi.display_plane_caps.minDstPosition,
				dddpi.display_plane_caps.maxDstPosition,
				dddpi.display_plane_caps.minDstExtent,
				dddpi.display_plane_caps.maxDstExtent
			);

			const VkDisplaySurfaceCreateInfoKHR display_surface_create_info = {
				.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR,
				.pNext = nullptr,
				.flags = 0,
				.displayMode =
					attached_displays[dddpi.display_data_idx].display_modes[dddpi.display_mode_idx].displayMode,
				.planeIndex		 = static_cast<uint32_t>(dddpi.display_plane_idx),
				.planeStackIndex = display_plane_data[dddpi.display_data_idx].properties.currentStackIndex,
				.transform		 = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
				.globalAlpha	 = 1.0f,
				.alphaMode		 = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR,
				.imageExtent	 = dddpi.display_plane_caps.maxDstExtent,
			};

			VkSurfaceKHR display_surface{};
			if (const VkResult result = WRAP_VULKAN_FUNC(
					vkCreateDisplayPlaneSurfaceKHR, instance, &display_surface_create_info, nullptr, &display_surface
				);
				result != VK_SUCCESS) {
				return tl::nullopt;
			}

			return tl::make_optional<PresentToDisplaySurface>(
				attached_displays[dddpi.display_data_idx],
				display_plane_data[dddpi.display_plane_idx],
				dddpi.display_plane_caps,
				xrUniqueVkSurfaceKHR{display_surface, VkResourceDeleter_VkSurfaceKHR{instance}}
			);
		}
	);
}

#if defined(XRAY_OS_IS_WINDOWS)
#else

tl::optional<PresentToSurface> create_xcb_surface(const WindowPlatformDataXcb& win_platform_data, VkInstance instance) {
	const VkXcbSurfaceCreateInfoKHR surface_create_info = {
		.sType		= VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext		= nullptr,
		.flags		= 0,
		.connection = reinterpret_cast<xcb_connection_t*>(win_platform_data.connection),
		.window		= static_cast<xcb_window_t>(win_platform_data.window),
	};

	xrUniqueVkSurfaceKHR surface_khr{
		[instance, &surface_create_info]() {
			VkSurfaceKHR surface{};
			WRAP_VULKAN_FUNC(vkCreateXcbSurfaceKHR, instance, &surface_create_info, nullptr, &surface);
			return surface;
		}(),
		VkResourceDeleter_VkSurfaceKHR{instance},
	};

	if (!surface_khr) return tl::nullopt;

	XR_LOG_INFO("Surface (XCB) created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(surface_khr)));
	return tl::make_optional<PresentToSurface>(PresentToWindowSurface{win_platform_data, std::move(surface_khr)});
}

tl::optional<PresentToSurface> create_xlib_surface(
	const WindowPlatformDataXlib& win_platform_data, VkInstance instance
) {
	const VkXlibSurfaceCreateInfoKHR surface_create_info = {
		.sType	= VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
		.pNext	= nullptr,
		.flags	= 0,
		.dpy	= reinterpret_cast<Display*>(win_platform_data.display),
		.window = win_platform_data.window,
	};

	xrUniqueVkSurfaceKHR surface_khr{
		[&]() {
			VkSurfaceKHR surface{};
			WRAP_VULKAN_FUNC(vkCreateXlibSurfaceKHR, instance, &surface_create_info, nullptr, &surface);
			return surface;
		}(),
		VkResourceDeleter_VkSurfaceKHR{instance},
	};

	if (!surface_khr) return tl::nullopt;

	XR_LOG_INFO("Surface created: {:#x}", reinterpret_cast<uintptr_t>(raw_ptr(surface_khr)));
	return tl::make_optional<PresentToSurface>(PresentToWindowSurface{
		win_platform_data,
		std::move(surface_khr),
	});
}

#endif

uint32_t vk_find_allocation_memory_type(
	const VkPhysicalDeviceMemoryProperties& memory_properties,
	const uint32_t memory_requirements,
	const VkMemoryPropertyFlags required_flags
) {
	// XR_LOG_TRACE("Memory required:\n{:0>32b}\n{:0>32b}", memory_requirements, required_flags);
	for (uint32_t memory_type = 0, memory_types_count = memory_properties.memoryTypeCount;
		 memory_type < memory_types_count;
		 ++memory_type) {
		const uint32_t memory_type_bits = 1 << memory_type;

		// XR_LOG_TRACE("Device memory {:0>32b}, heap index {}, mem type {}",
		//              memory_properties.memoryTypes[memory_type].propertyFlags,
		//              memory_properties.memoryTypes[memory_type].heapIndex,
		//              memory_type);

		const bool is_required_mem_type			   = memory_requirements & memory_type_bits;
		const VkMemoryPropertyFlags mem_prop_flags = memory_properties.memoryTypes[memory_type].propertyFlags;
		const bool has_required_properties		   = (mem_prop_flags & required_flags) == required_flags;

		if (is_required_mem_type && has_required_properties) return memory_type;
	}

	return 0xffffffffu;
}

struct SwapchainStateCreationInfo {
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

tl::optional<detail::SwapchainState> create_swapchain_state(const SwapchainStateCreationInfo& create_info);

struct QueueFamilyIndices {
	uint32_t graphics;
	uint32_t compute;
	uint32_t transfer;
};

tl::optional<QueueFamilyIndices> vk_renderer_pick_queue_families(
	xray::base::MemoryArena& arena, VkPhysicalDevice phys_device, VkSurfaceKHR surface
) {
	ScratchPadArena scratch_pad = ThreadLocalContext::acquire_scratchpad({&arena});

	uint32_t queue_families_count{};
	vkGetPhysicalDeviceQueueFamilyProperties2(phys_device, &queue_families_count, nullptr);
	if (queue_families_count == 0) {
		return tl::nullopt;
	}

	base::containers::vector<VkQueueFamilyProperties2> queue_family_props{*scratch_pad.arena};
	queue_family_props.resize(
		queue_families_count,
		VkQueueFamilyProperties2{
			.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
			.pNext = nullptr,
		}
	);

	vkGetPhysicalDeviceQueueFamilyProperties2(phys_device, &queue_families_count, queue_family_props.data());
	if (queue_families_count == 0) {
		return tl::nullopt;
	}

	tl::optional<uint32_t> q_graphics{};
	tl::optional<uint32_t> q_compute{};
	tl::optional<uint32_t> q_transfer{};

	for (size_t queue_index = 0; queue_index < queue_family_props.size(); ++queue_index) {
		if (q_graphics && q_compute && q_transfer) {
			break;
		}

		const auto& queue_props	 = queue_family_props[queue_index];
		uint32_t max_queue_count = queue_props.queueFamilyProperties.queueCount;
		if (!q_graphics && (queue_props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			VkBool32 has_wsi_support	= VK_FALSE;
			const VkResult query_result = vkGetPhysicalDeviceSurfaceSupportKHR(
				phys_device, static_cast<uint32_t>(queue_index), surface, &has_wsi_support
			);
			if (query_result == VK_SUCCESS && has_wsi_support) {
				q_graphics = static_cast<uint32_t>(queue_index);
				max_queue_count -= 1;
			}
		}

		if (max_queue_count == 0) {
			continue;
		}

		if (!q_compute && (queue_props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			q_compute = static_cast<uint32_t>(queue_index);
			max_queue_count -= 1;
		}

		if (max_queue_count == 0) {
			continue;
		}

		if (!q_transfer && (queue_props.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
			q_transfer = static_cast<uint32_t>(queue_index);
			max_queue_count -= 1;
		}
	}

	if (q_graphics && q_compute && q_transfer) {
		return QueueFamilyIndices{
			.graphics = *q_graphics,
			.compute  = *q_compute,
			.transfer = *q_transfer,
		};
	}

	return tl::nullopt;
}

tl::optional<PresentToSurface> vk_renderer_create_surface(const WindowPlatformData& win_data, VkInstance instance) {
#if defined(XRAY_OS_IS_WINDOWS)
	if (const WindowPlatformDataWin32* wp = swl::get_if<WindowPlatformDataWin32>(&win_data)) {
		const VkWin32SurfaceCreateInfoKHR create_info{
			.sType	   = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.pNext	   = nullptr,
			.flags	   = 0,
			.hinstance = reinterpret_cast<HINSTANCE>(wp->module),
			.hwnd	   = reinterpret_cast<HWND>(wp->window),
		};

		xrUniqueVkSurfaceKHR surface{
			[&]() {
				VkSurfaceKHR surface{};
				WRAP_VULKAN_FUNC(vkCreateWin32SurfaceKHR, instance, &create_info, nullptr, &surface);
				return surface;
			}(),
			VkResourceDeleter_VkSurfaceKHR{instance},
		};

		if (!surface) return tl::nullopt;

		return tl::make_optional<PresentToSurface>(PresentToWindowSurface{*wp, std::move(surface)});
	}

#else
	if (const WindowPlatformDataXlib* xlib = swl::get_if<WindowPlatformDataXlib>(&win_data)) {
		return create_xlib_surface(*xlib, instance);
	}

	if (const WindowPlatformDataXcb* xcb = swl::get_if<WindowPlatformDataXcb>(&win_data)) {
		return create_xcb_surface(*xcb, instance);
	}
#endif
	return tl::nullopt;
}

struct R_PhysicalDeviceSetup {
	VkPhysicalDevice physical;
	VkPhysicalDeviceProperties2 properties;
	VkPhysicalDeviceDescriptorIndexingProperties descriptor_indexing_properties;
	VkPhysicalDeviceVulkan11Properties p_vk11;
	VkPhysicalDeviceVulkan12Properties p_vk12;
	VkPhysicalDeviceVulkan13Properties p_vk13;
	VkPhysicalDeviceMemoryProperties2 memory_properties;
	VkSurfaceCapabilitiesKHR surface_caps;
	VkPhysicalDeviceFeatures2 f_device;
	VkPhysicalDeviceDescriptorBufferFeaturesEXT f_descriptor_buffer;
	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT f_dynstate3;
	VkPhysicalDeviceVulkan11Features f_vk11;
	VkPhysicalDeviceVulkan12Features f_vk12;
	VkPhysicalDeviceVulkan13Features f_vk13;
	QueueFamilyIndices queue_indices;
	VkPresentModeKHR present_mode;
	VkSurfaceFormatKHR surface_format;
};

tl::optional<R_PhysicalDeviceSetup> vk_renderer_pick_physical_device(
	xray::base::MemoryArena& arena, VkInstance instance, VkSurfaceKHR surface
) {
	ScratchPadArena scratch_pad = ThreadLocalContext::acquire_scratchpad({&arena});

	uint32_t phys_devs_count{};

	vkEnumeratePhysicalDevices(instance, &phys_devs_count, nullptr);
	if (phys_devs_count == 0) {
		return tl::nullopt;
	}

	base::containers::vector<VkPhysicalDevice> phys_devices{*scratch_pad.arena};
	phys_devices.resize(phys_devs_count);
	vkEnumeratePhysicalDevices(instance, &phys_devs_count, phys_devices.data());
	if (phys_devs_count == 0) {
		return tl::nullopt;
	}

	VkPhysicalDeviceExtendedDynamicState3FeaturesEXT f_dynstate3{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT,
	};

	VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
	};
	VkPhysicalDeviceVulkan13Features vk13features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
	};

	VkPhysicalDeviceVulkan12Features vk12features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
	};

	VkPhysicalDeviceVulkan11Features vk11features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
	};

	VkPhysicalDeviceFeatures2 phys_device_features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
	};

	details::chain_structs(
		f_dynstate3, descriptor_buffer_features, vk13features, vk12features, vk11features, phys_device_features
	);
	VkSurfaceCapabilitiesKHR surface_caps{};

	VkPhysicalDeviceVulkan11Properties p_vk11{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
	};
	VkPhysicalDeviceVulkan12Properties p_vk12{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
	};
	VkPhysicalDeviceVulkan13Properties p_vk13{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
	};

	VkPhysicalDeviceDescriptorIndexingProperties descriptor_indexing_properties{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES,
	};

	VkPhysicalDeviceProperties2 phys_dev_properties{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
	};

	details::chain_structs(p_vk13, p_vk12, p_vk11, descriptor_indexing_properties, phys_dev_properties);

	for (VkPhysicalDevice phys_device : phys_devices) {
		vkGetPhysicalDeviceProperties2(phys_device, &phys_dev_properties);
		XR_LOG_INFO("Checking device {} ...", phys_dev_properties.properties.deviceName);

		if (phys_dev_properties.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			XR_LOG_INFO("Rejecting {}, not a discrete GPU", phys_dev_properties.properties.deviceName);
			continue;
		}

		vkGetPhysicalDeviceFeatures2(phys_device, &phys_device_features);

		const bool is_suitable_device = descriptor_buffer_features.descriptorBuffer != 0 and
										phys_device_features.features.multiDrawIndirect != 0 and
										vk11features.shaderDrawParameters != 0 and
										vk12features.drawIndirectCount != 0 and vk12features.descriptorIndexing != 0 and
										vk12features.descriptorBindingPartiallyBound != 0 and
										vk12features.descriptorBindingVariableDescriptorCount != 0;

		if (!is_suitable_device) {
			XR_LOG_INFO("Rejecting {}, missing some required features", phys_dev_properties.properties.deviceName);
			continue;
		}

		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys_device, surface, &surface_caps) != VK_SUCCESS) {
			continue;
		}

		if (!(surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
			continue;
		}

		const tl::optional<VkSurfaceFormatKHR> surface_format = [&]() -> tl::optional<VkSurfaceFormatKHR> {
			uint32_t surface_fmts_count{};
			vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &surface_fmts_count, nullptr);
			if (surface_fmts_count == 0) {
				XR_LOG_INFO("No supported surface format found ...");
				return tl::nullopt;
			}

			ScratchPadArena scratch_scope{scratch_pad.arena};
			containers::vector<VkSurfaceFormatKHR> surface_format_list{*scratch_scope.arena};
			surface_format_list.resize(surface_fmts_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(phys_device, surface, &surface_fmts_count, surface_format_list.data());

			containers::string dbg_str{*scratch_scope.arena};
			dbg_str.reserve(2048);
			for (const VkSurfaceFormatKHR& fmt : surface_format_list) {
				fmt::format_to(
					std::back_inserter(dbg_str), " :: {}", vk::to_string(static_cast<vk::Format>(fmt.format))
				);
			}

			XR_LOG_INFO("Supporte surface format list: {}", dbg_str);

			constexpr const VkFormat required_format_list[] = {
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_FORMAT_B8G8R8A8_UNORM,
			};

			for (const VkFormat req_fmt : required_format_list) {
				const auto fmt_pos = lz::index_of_if(surface_format_list, [req_fmt](const VkSurfaceFormatKHR& s_fmt) {
					return s_fmt.format == req_fmt;
				});
				if (fmt_pos != lz::npos) {
					return surface_format_list[fmt_pos];
				}
			}

			return tl::nullopt;
		}();

		if (!surface_format) {
			return tl::nullopt;
		}

		const tl::optional<VkPresentModeKHR> present_mode = [&]() -> tl::optional<VkPresentModeKHR> {
			uint32_t present_modes_count{};
			vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_modes_count, nullptr);
			if (present_modes_count == 0) {
				XR_LOG_INFO("No presentation modes supported");
				return tl::nullopt;
			}

			ScratchPadArena scratch_scope{scratch_pad.arena};
			containers::vector<VkPresentModeKHR> present_modes{*scratch_scope.arena};
			present_modes.resize(present_modes_count);

			vkGetPhysicalDeviceSurfacePresentModesKHR(phys_device, surface, &present_modes_count, present_modes.data());

			containers::string dbg_str{*scratch_scope.arena};
			dbg_str.reserve(2048);
			dbg_str.append("present modes");

			for (const VkPresentModeKHR pres_mode : present_modes) {
				fmt::format_to(
					std::back_inserter(dbg_str), " :: {}", vk::to_string(static_cast<vk::PresentModeKHR>(pres_mode))
				);
			}

			constexpr const VkPresentModeKHR preferred_presentation_modes[] = {
				VK_PRESENT_MODE_FIFO_KHR,
				VK_PRESENT_MODE_MAILBOX_KHR,
				VK_PRESENT_MODE_IMMEDIATE_KHR,
				VK_PRESENT_MODE_FIFO_RELAXED_KHR,
			};
			const auto best_preferred_supported_mode =
				ranges::find_first_of(preferred_presentation_modes, present_modes);

			if (best_preferred_supported_mode == cend(preferred_presentation_modes)) {
				XR_LOG_CRITICAL("None of the preferred presentation modes is suppored!");
				return tl::nullopt;
			}

			return tl::optional<VkPresentModeKHR>{*best_preferred_supported_mode};
		}();

		if (!present_mode) {
			return tl::nullopt;
		}

		XR_LOG_INFO(
			"best present mode is {:#x} -> {}",
			static_cast<uint32_t>(*present_mode),
			vk::to_string(static_cast<vk::PresentModeKHR>(*present_mode))
		);

		const tl::optional<QueueFamilyIndices> queue_families =
			vk_renderer_pick_queue_families(arena, phys_device, surface);
		if (!queue_families) {
			return tl::nullopt;
		}

		VkPhysicalDeviceMemoryProperties2 memory_properties{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2,
		};

		vkGetPhysicalDeviceMemoryProperties2(phys_device, &memory_properties);

		//
		// list device extensions
		{
			uint32_t extensions_count{};
			vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extensions_count, nullptr);
			if (extensions_count != 0) {
				containers::vector<VkExtensionProperties> device_exts_list{*scratch_pad.arena};

				device_exts_list.resize(extensions_count);
				vkEnumerateDeviceExtensionProperties(phys_device, nullptr, &extensions_count, device_exts_list.data());

				containers::string dbg_str{*scratch_pad.arena};
				dbg_str.reserve(2048);

				for (const VkExtensionProperties& ext_props : device_exts_list) {
					fmt::format_to(
						back_inserter(dbg_str), "{} - {:#x}, ", ext_props.extensionName, ext_props.specVersion
					);
				}
				XR_LOG_INFO("Found device extensions: {}", dbg_str);
			}
		}

		return tl::optional<R_PhysicalDeviceSetup>{
			tl::in_place,
			phys_device,
			phys_dev_properties,
			descriptor_indexing_properties,
			p_vk11,
			p_vk12,
			p_vk13,
			memory_properties,
			surface_caps,
			phys_device_features,
			descriptor_buffer_features,
			f_dynstate3,
			vk11features,
			vk12features,
			vk13features,
			*queue_families,
			*present_mode,
			*surface_format,
		};
	}

	return tl::nullopt;
}

struct RQueue_t {
	uint32_t index{};
	VkQueue handle{};
	VkCommandPool cmd_pool{};
};

struct R_LogicalDeviceSetup {
	VkDevice device_handle;
	std::array<RQueue_t, 3> queues;
};

tl::optional<R_LogicalDeviceSetup> vk_renderer_setup_logical_device(
	xray::base::MemoryArena& arena, const R_PhysicalDeviceSetup& physical
) {
	ScratchPadArena scratch_pad		   = ThreadLocalContext::acquire_scratchpad({&arena});
	constexpr float queue_priorities[] = {1.0f, 1.0f, 1.0f};

	struct QueueCreationData {
		VkDeviceQueueCreateInfo create_info;
		uint32_t queue_index;
	};

	struct QueueRetrievalData {
		uint32_t family_index;
		uint32_t queue_index;
	};

	containers::vector<QueueRetrievalData> queue_retrieve_data{*scratch_pad.arena};
	queue_retrieve_data.reserve(3);

	containers::unordered_map<uint32_t, QueueCreationData> queue_create_list{*scratch_pad.arena};

	const uint32_t queue_indices[] = {
		physical.queue_indices.graphics, physical.queue_indices.transfer, physical.queue_indices.compute
	};

	for (const uint32_t queue_family_index : queue_indices) {
		auto itr = queue_create_list.find(queue_family_index);
		if (itr != end(queue_create_list)) {
			itr->second.queue_index += 1;
			itr->second.create_info.queueCount += 1;
		} else {
			queue_create_list[queue_family_index] = QueueCreationData{
				.create_info =
					VkDeviceQueueCreateInfo{
						.sType			  = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
						.pNext			  = nullptr,
						.flags			  = 0,
						.queueFamilyIndex = queue_family_index,
						.queueCount		  = 1,
						.pQueuePriorities = queue_priorities,
					},
				.queue_index = 0,
			};
		}

		queue_retrieve_data.push_back(QueueRetrievalData{
			.family_index = queue_family_index,
			.queue_index  = queue_create_list[queue_family_index].queue_index,
		});
	}

	containers::vector<VkDeviceQueueCreateInfo> queue_create_infos{*scratch_pad.arena};
	queue_create_infos.reserve(queue_create_list.size());

	for (const auto& [q_family_index, q_create_info] : queue_create_list) {
		queue_create_infos.push_back(q_create_info.create_info);
	}

	auto phys_features		 = physical.f_device;
	auto f_11				 = physical.f_vk11;
	auto f_12				 = physical.f_vk12;
	auto f_13				 = physical.f_vk13;
	auto f_descriptor_buffer = physical.f_descriptor_buffer;
	auto f_dynstate3		 = physical.f_dynstate3;

	details::chain_structs(f_dynstate3, f_descriptor_buffer, f_13, f_12, f_11, phys_features);

	static constexpr initializer_list<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
		VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
	};

	const VkDeviceCreateInfo device_create_info = {
		.sType					 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext					 = &phys_features,
		.flags					 = 0,
		.queueCreateInfoCount	 = static_cast<uint32_t>(size(queue_create_infos)),
		.pQueueCreateInfos		 = queue_create_infos.data(),
		.enabledLayerCount		 = 0,
		.ppEnabledLayerNames	 = nullptr,
		.enabledExtensionCount	 = static_cast<uint32_t>(size(device_extensions)),
		.ppEnabledExtensionNames = device_extensions.begin(),
		.pEnabledFeatures		 = nullptr,
	};

	VkDevice logical_device{};
	if (vkCreateDevice(physical.physical, &device_create_info, nullptr, &logical_device) != VK_SUCCESS) {
		return tl::nullopt;
	}

	//
	// Load device extensions
#define PFN_LIST_ENTRY(fnproto, name, tag)                                                 \
	do {                                                                                   \
		if (std::is_same_v<tag, VulkanDeviceExtensionTag>) {                               \
			vkfn::name = load_vulkan_proc<fnproto>(logical_device, XR_MAKE_VK_FUNC(name)); \
		}                                                                                  \
	} while (0);

#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatched.functions.hpp"
	FUNCTION_POINTERS_LIST
#undef PFN_LIST_ENTRY
#undef FUNCTION_POINTERS_LIST

	//
	// setup queues
	const array<uint32_t, 3> queue_flags{
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	};

	array<RQueue_t, 3> queues{};

	for (size_t i = 0; i < 3; ++i) {
		const QueueRetrievalData& qrd = queue_retrieve_data[i];
		RQueue_t& q_out				  = queues[i];

		vkGetDeviceQueue(logical_device, qrd.family_index, qrd.queue_index, &q_out.handle);

		const VkCommandPoolCreateInfo cmd_pool_create_info = {
			.sType			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext			  = nullptr,
			.flags			  = queue_flags[i],
			.queueFamilyIndex = static_cast<uint32_t>(qrd.family_index),
		};

		if (vkCreateCommandPool(logical_device, &cmd_pool_create_info, nullptr, &q_out.cmd_pool) != VK_SUCCESS) {
			return tl::nullopt;
		}
	}

	return tl::optional<R_LogicalDeviceSetup>{tl::in_place, logical_device, queues};
}

struct R_InstanceState {
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug;
};

tl::optional<R_InstanceState> vk_renderer_setup_instance(xray::base::MemoryArena& arena) {
	uint32_t instance_version{};
	vkEnumerateInstanceVersion(&instance_version);

	XR_LOG_INFO(
		"Vulkan version {}.{}.{}.{}\n",
		VK_API_VERSION_VARIANT(instance_version),
		VK_API_VERSION_MAJOR(instance_version),
		VK_API_VERSION_MINOR(instance_version),
		VK_API_VERSION_PATCH(instance_version)
	);

	ScratchPadArena scratch_pad = ThreadLocalContext::acquire_scratchpad({&arena});

	//
	// output present extensions info
	{
		uint32_t property_count{};
		vkEnumerateInstanceExtensionProperties(nullptr, &property_count, nullptr);

		if (property_count) {
			base::containers::vector<VkExtensionProperties> ext_props{property_count, *scratch_pad.arena};
			vkEnumerateInstanceExtensionProperties(nullptr, &property_count, ext_props.data());

			for (const VkExtensionProperties& ext : ext_props) {
				XR_LOG_INFO("extension: {} - {:#0x}", ext.extensionName, ext.specVersion);
			}
		}
	}

	//
	// output layers info
	{
		uint32_t layer_count{};
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		if (layer_count != 0) {
			base::containers::vector<VkLayerProperties> layer_props{layer_count, *scratch_pad.arena};
			vkEnumerateInstanceLayerProperties(&layer_count, layer_props.data());
			for (const VkLayerProperties& layer_prop : layer_props) {
				XR_LOG_INFO("Layer: {}, desc {}\n", layer_prop.layerName, layer_prop.description);
			}
		}
	}

	//
	// TODO: add device buffer address support
	const char* const extensions_list[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#if defined(XRAY_OS_IS_WINDOWS)
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};

	//
	// Layers and validation features are controlled through vk_layer_settings.txt
	const VkApplicationInfo app_info{
		.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext				= nullptr,
		.pApplicationName	= "xray-ng-app",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName		= "xray-engine",
		.engineVersion		= VK_MAKE_VERSION(1, 0, 0),
		.apiVersion			= VK_API_VERSION_1_3,
	};

	const VkDebugUtilsMessengerCreateInfoEXT dbg_utils_msg_create_ext{
		.sType			 = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext			 = nullptr,
		.flags			 = 0,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
						   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = details::r_vk_debug_msg_output,
		.pUserData		 = nullptr,
	};

	const VkValidationFeatureEnableEXT enabled_validation_features[] = {
		VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		// VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
		// VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT					  ,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	};

	const VkValidationFeaturesEXT validation_features{
		.sType							= VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
		.pNext							= &dbg_utils_msg_create_ext,
		.enabledValidationFeatureCount	= static_cast<uint32_t>(std::size(enabled_validation_features)),
		.pEnabledValidationFeatures		= enabled_validation_features,
		.disabledValidationFeatureCount = 0,
		.pDisabledValidationFeatures	= nullptr,
	};

	constexpr const char* const enabled_layers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	const VkInstanceCreateInfo instance_create_info = {
		.sType					 = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext					 = &validation_features,
		.flags					 = 0,
		.pApplicationInfo		 = &app_info,
		.enabledLayerCount		 = static_cast<uint32_t>(std::size(enabled_layers)),
		.ppEnabledLayerNames	 = enabled_layers,
		.enabledExtensionCount	 = static_cast<uint32_t>(std::size(extensions_list)),
		.ppEnabledExtensionNames = extensions_list,
	};

	VkInstance vkinstance{};
	if (vkCreateInstance(&instance_create_info, nullptr, &vkinstance) != VK_SUCCESS) {
		return tl::nullopt;
	}

	XR_LOG_INFO("Vulkan instance created @ {:p}", fmt::ptr(vkinstance));

	//
	// Load instance extensions
#define PFN_LIST_ENTRY(fnproto, name, tag)                                             \
	do {                                                                               \
		if (std::is_same_v<tag, VulkanInstanceExtensionTag>) {                         \
			vkfn::name = load_vulkan_proc<fnproto>(vkinstance, XR_MAKE_VK_FUNC(name)); \
		}                                                                              \
	} while (0);

#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatched.functions.hpp"
	FUNCTION_POINTERS_LIST
#undef PFN_LIST_ENTRY
#undef FUNCTION_POINTERS_LIST

	VkDebugUtilsMessengerEXT dbg_msgr{};
	vkfn::CreateDebugUtilsMessengerEXT(vkinstance, &dbg_utils_msg_create_ext, nullptr, &dbg_msgr);
	if (!dbg_msgr) {
		XR_LOG_ERR("Failed to create debug messenger. No Vulkan debugging output will be visible.");
	}

	return tl::optional<R_InstanceState>{tl::in_place, vkinstance, dbg_msgr};
}

tl::optional<VulkanRenderer> VulkanRenderer::create(
	xray::base::MemoryArena& arena, const WindowPlatformData& win_data, const RendererConfig& cfg
) {
	using namespace xray::base;
	ScratchPadArena scratch_pad = ThreadLocalContext::acquire_scratchpad({&arena});

	tl::optional<R_InstanceState> instance = vk_renderer_setup_instance(arena);

	//
	// surface
	tl::optional<PresentToSurface> present_to_surface = vk_renderer_create_surface(win_data, instance->instance);
	if (!present_to_surface) {
		return tl::nullopt;
	}

	VkSurfaceKHR surface = swl::visit(
		VariantVisitor{
			[](const PresentToWindowSurface& win) { return raw_ptr(win.surface); },
			[](const PresentToDisplaySurface& display) { return raw_ptr(display.display_surface); },
		},
		*present_to_surface
	);

	//
	// physical device
	tl::optional<R_PhysicalDeviceSetup> phys_device =
		vk_renderer_pick_physical_device(arena, instance->instance, surface);
	if (!phys_device) {
		XR_LOG_INFO("No suitable device present in the system.");
		return tl::nullopt;
	}

	XR_LOG_INFO(
		"Using device {}, vendor {:#x}",
		phys_device->properties.properties.deviceName,
		phys_device->properties.properties.vendorID
	);

	tl::optional<R_LogicalDeviceSetup> logical_device = vk_renderer_setup_logical_device(arena, *phys_device);
	if (!logical_device) return tl::nullopt;

	tl::optional<detail::SwapchainState> swapchain_state = [&]() -> tl::optional<detail::SwapchainState> {
		const uint32_t swapchain_image_count = [&]() {
			if (phys_device->surface_caps.maxImageCount == 0) {
				//
				// no limit for the maximum number of images
				return phys_device->surface_caps.minImageCount + 1;
			}
			return min(phys_device->surface_caps.minImageCount + 1, phys_device->surface_caps.maxImageCount);
		}();

		const VkExtent3D swapchain_dimensions = swl::visit(
			VariantVisitor{
#if defined(XRAY_OS_IS_WINDOWS)
				[](const WindowPlatformDataWin32& win32) {
					return VkExtent3D{.width = win32.width, .height = win32.height, .depth = 1};
				}
#else
				[](const WindowPlatformDataXcb& xcb) {
					return VkExtent3D{.width = xcb.width, .height = xcb.height, .depth = 1};
				},
				[](const WindowPlatformDataXlib& xlib) {
					return VkExtent3D{.width = xlib.width, .height = xlib.height, .depth = 1};
				},
#endif
			},
			win_data
		);

		const SwapchainStateCreationInfo swapchain_state_create_info{
			.device			   = logical_device->device_handle,
			.retired_swapchain = nullptr,
			.surface		   = surface,
			.surface_caps	   = phys_device->surface_caps,
			.fmt			   = phys_device->surface_format,
			.present_mode	   = phys_device->present_mode,
			.mem_props		   = phys_device->memory_properties.memoryProperties,
			.image_count	   = swapchain_image_count,
			.dimensions		   = swapchain_dimensions,
			.depth_att_format  = VK_FORMAT_D32_SFLOAT_S8_UINT,
		};

		return create_swapchain_state(swapchain_state_create_info);
	}();

	if (!swapchain_state) {
		XR_LOG_ERR("Failed to create swapchain state!");
		return tl::nullopt;
	}

	//
	// command buffers
	vector<VkCommandBuffer> command_buffers{[&]() {
		const VkCommandBufferAllocateInfo cmd_buff_alloc_info = {
			.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext				= nullptr,
			.commandPool		= logical_device->queues[to_underlying(QueueType::Graphics)].cmd_pool,
			.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = static_cast<uint32_t>(swapchain_state->swapchain_imageviews.size()),
		};

		vector<VkCommandBuffer> cmd_buffers{swapchain_state->swapchain_imageviews.size()};
		WRAP_VULKAN_FUNC(
			vkAllocateCommandBuffers, logical_device->device_handle, &cmd_buff_alloc_info, cmd_buffers.data()
		);

		return cmd_buffers;
	}()};

	const uint32_t max_frames{static_cast<uint32_t>(swapchain_state->swapchain_images.size())};

	const VkPushConstantRange graphics_bindless_push_consts[] = {
		VkPushConstantRange{
			.stageFlags = VK_SHADER_STAGE_ALL,
			.offset		= 0,
			.size		= static_cast<uint32_t>(sizeof(uint32_t)),
		},
	};

	const LayoutBindingsByResourceType graphics_bindless_set_layouts[] = {
		LayoutBindingsByResourceType{
			.res_type		  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptor_count = 16,
			.stage_flags	  = VK_SHADER_STAGE_ALL,
			.tag			  = "DS_uniform_buffer.ubo",
		},
		LayoutBindingsByResourceType{
			.res_type		  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptor_count = 512,
			.stage_flags	  = VK_SHADER_STAGE_ALL,
			.tag			  = "DS_storage_buffer",
		},
		LayoutBindingsByResourceType{
			.res_type		  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptor_count = 512,
			.stage_flags	  = VK_SHADER_STAGE_ALL,
			.tag			  = "DS_combined_sampler",
		},
		LayoutBindingsByResourceType{
			.res_type		  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptor_count = 512,
			.stage_flags	  = VK_SHADER_STAGE_ALL,
			.tag			  = "DS_storage_image",
		},
		LayoutBindingsByResourceType{
			.res_type		  = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			.descriptor_count = 512,
			.stage_flags	  = VK_SHADER_STAGE_ALL,
			.tag			  = "DS_storage_texel_buffer",
		},
	};

	tl::expected<BindlessSystem, VulkanError> bindless_sys{BindlessSystem::create(
		arena,
		BindlessSystem::Kind::Graphics,
		logical_device->device_handle,
		phys_device->descriptor_indexing_properties,
		graphics_bindless_set_layouts,
		graphics_bindless_push_consts
	)};

	if (!bindless_sys) {
		return tl::nullopt;
	}

	xrUniqueVkBuffer staging_buffer{nullptr, VkResourceDeleter_VkBuffer{logical_device->device_handle}};
	const VkBufferCreateInfo staging_create_info{
		.sType				   = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext				   = nullptr,
		.flags				   = 0,
		.size				   = base::megabytes(512),
		.usage				   = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode		   = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices   = nullptr,
	};

	WRAP_VULKAN_FUNC(
		vkCreateBuffer, logical_device->device_handle, &staging_create_info, nullptr, raw_ptr_ptr(staging_buffer)
	);
	if (!staging_buffer) return tl::nullopt;

	VkMemoryRequirements staging_mem_rq{};
	vkGetBufferMemoryRequirements(logical_device->device_handle, raw_ptr(staging_buffer), &staging_mem_rq);

	const VkMemoryAllocateInfo staging_mem_alloc{
		.sType			 = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext			 = nullptr,
		.allocationSize	 = staging_mem_rq.size,
		.memoryTypeIndex = vk_find_allocation_memory_type(
			phys_device->memory_properties.memoryProperties,
			staging_mem_rq.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		),
	};

	xrUniqueVkDeviceMemory staging_mem{nullptr, VkResourceDeleter_VkDeviceMemory{logical_device->device_handle}};
	WRAP_VULKAN_FUNC(
		vkAllocateMemory, logical_device->device_handle, &staging_mem_alloc, nullptr, raw_ptr_ptr(staging_mem)
	);
	if (!staging_mem) return tl::nullopt;

	const VkResult bind_res = WRAP_VULKAN_FUNC(
		vkBindBufferMemory, logical_device->device_handle, raw_ptr(staging_buffer), raw_ptr(staging_mem), 0
	);
	if (bind_res != VK_SUCCESS) {
		return tl::nullopt;
	}

	xrUniqueBufferWithMemory staging{
		logical_device->device_handle, unique_pointer_release(staging_buffer), unique_pointer_release(staging_mem)
	};

	auto mapped_staging_buffer = UniqueMemoryMapping::map_memory(
		logical_device->device_handle, staging.handle<VkDeviceMemory>(), 0, VK_WHOLE_SIZE
	);
	if (!mapped_staging_buffer) return tl::nullopt;

	std::vector<detail::Queue> queues{};
	queues.reserve(3);
	for (const RQueue_t& r_queue : logical_device->queues) {
		queues.emplace_back(
			r_queue.index,
			r_queue.handle,
			xrUniqueVkCommandPool{r_queue.cmd_pool, VkResourceDeleter_VkCommandPool{logical_device->device_handle}}
		);
	}

	return tl::make_optional<VulkanRenderer>(
		PrivateConstructionToken{},

		detail::InstanceState{
			xrUniqueVkInstance{
				instance->instance,
				VkResourceDeleter_VkInstance{NotOwnedVulkanResource{}},
			},
			xrUniqueVkDebugUtilsMessengerEXT{
				instance->debug,
				VkResourceDeleter_VkDebugUtilsMessengerEXT{instance->instance},
			},
		},

		detail::RenderState{
			detail::PhysicalDeviceData{
				.device = phys_device->physical,
				.properties =
					{
						.base				 = phys_device->properties,
						.vk11				 = phys_device->p_vk11,
						.vk12				 = phys_device->p_vk12,
						.vk13				 = phys_device->p_vk13,
						.descriptor_indexing = phys_device->descriptor_indexing_properties,
					},
				.memory_properties = phys_device->memory_properties,
			},

			xrUniqueVkDevice{
				logical_device->device_handle,
				VkResourceDeleter_VkDevice{NotOwnedVulkanResource{}},
			},

			std::move(staging),
			std::move(*mapped_staging_buffer),
			std::move(queues),
			detail::RenderingAttachments{
				.view_mask = 0,
				.attachments =
					{
						phys_device->surface_format.format,
						VK_FORMAT_D32_SFLOAT_S8_UINT,
						VK_FORMAT_D32_SFLOAT_S8_UINT,
					},
			},
		},

		detail::PresentationState{
			0,
			max_frames,
			0,
			0,
			detail::SurfaceState{
				swl::visit(
					VariantVisitor{
						[](PresentToWindowSurface&& win_surface) { return std::move(win_surface.surface); },
						[](PresentToDisplaySurface&& display_surface) {
							return std::move(display_surface.display_surface);
						},
					},
					std::move(*present_to_surface.take())
				),
				phys_device->surface_caps,
				phys_device->surface_format,
				phys_device->present_mode,
				VK_FORMAT_D32_SFLOAT_S8_UINT,
			},
			std::move(*swapchain_state.take()),
			std::move(command_buffers),
		},
		std::move(*bindless_sys)
	);
}

VulkanRenderer::VulkanRenderer(
	VulkanRenderer::PrivateConstructionToken,
	detail::InstanceState instance_state,
	detail::RenderState render_state,
	detail::PresentationState presentation_state,
	BindlessSystem bindless
)
	: _instance_state{std::move(instance_state)},
	  _render_state{std::move(render_state)},
	  _presentation_state{std::move(presentation_state)},
	  _bindless{std::move(bindless)} {
	const char* queue_names[] = {"graphics queue", "transfer queue"};
	const char* pool_names[]  = {"cmd_pool_graphics", "cmd_pool_transfer"};
	for (uint32_t qidx : {static_cast<uint32_t>(QueueType::Graphics), static_cast<uint32_t>(QueueType::Transfer)}) {
		dbg_set_object_name(xray::base::raw_ptr(_render_state.queues[qidx].cmd_pool), pool_names[qidx]);
		dbg_set_object_name(_render_state.queues[qidx].handle, queue_names[qidx]);
	}
}

FrameRenderData VulkanRenderer::begin_rendering(
	const float red, const float green, const float blue, const float depth, const uint32_t stencil
) {
	ZoneScopedN("BeginRendering");

	//
	// wait for previously submitted work to finish
	{
		ZoneScopedN("WaitForFences");
		const VkFence fences[] = {
			raw_ptr(_presentation_state.swapchain_state.sync.fences[_presentation_state.frame_index])
		};
		const VkResult wait_fence_result{WRAP_VULKAN_FUNC(
			vkWaitForFences, raw_ptr(_render_state.dev_logical), 1, fences, true, numeric_limits<uint64_t>::max()
		)};

		if (wait_fence_result != VK_SUCCESS) {
			XR_LOG_CRITICAL("Failed to wait on fence!");
		}

		WRAP_VULKAN_FUNC(vkResetFences, raw_ptr(_render_state.dev_logical), 1, fences);
	}

	{
		ZoneScopedN("AcquireImage");
		const VkResult acquire_image_result{
			WRAP_VULKAN_FUNC(
				vkAcquireNextImageKHR,
				raw_ptr(_render_state.dev_logical),
				raw_ptr(_presentation_state.swapchain_state.swapchain),
				numeric_limits<uint64_t>::max(),
				raw_ptr(_presentation_state.swapchain_state.sync.present_sem[_presentation_state.frame_index]),
				nullptr,
				&_presentation_state.acquired_image
			),
		};

		if (acquire_image_result == VK_SUBOPTIMAL_KHR || acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR ||
			_presentation_state.state_bits & detail::PresentationState::STATE_SWAPCHAIN_SUBOPTIMAL) {
			handle_swapchain_suboptimal_out_of_date(SwapchainReacquireAfterSuboptimal::Always_);
		}
	}

	const uint32_t acquired_image{_presentation_state.acquired_image};
	//
	// reset command buffer
	WRAP_VULKAN_FUNC(vkResetCommandBuffer, _presentation_state.command_buffers[_presentation_state.frame_index], 0);
	const VkCommandBufferBeginInfo cmd_buf_begin_info = {
		.sType			  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext			  = nullptr,
		.flags			  = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr,
	};
	WRAP_VULKAN_FUNC(
		vkBeginCommandBuffer, _presentation_state.command_buffers[_presentation_state.frame_index], &cmd_buf_begin_info
	);

	const VkRenderingAttachmentInfo color_attachment = {
		.sType				= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext				= nullptr,
		.imageView			= raw_ptr(_presentation_state.swapchain_state.swapchain_imageviews[acquired_image]),
		.imageLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.resolveMode		= VK_RESOLVE_MODE_NONE,
		.resolveImageView	= nullptr,
		.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp			= VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue			= VkClearValue{.color = VkClearColorValue{.float32 = {red, green, blue, 1.0f}}},
	};

	const VkRenderingAttachmentInfo depth_attachment = {
		.sType				= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.pNext				= nullptr,
		.imageView			= raw_ptr(_presentation_state.swapchain_state.depth_stencil_image_views[acquired_image]),
		.imageLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.resolveMode		= VK_RESOLVE_MODE_NONE,
		.resolveImageView	= nullptr,
		.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.loadOp				= VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.clearValue = VkClearValue{.depthStencil = VkClearDepthStencilValue{.depth = depth, .stencil = stencil}},
	};

	const VkRect2D render_area{
		.offset = VkOffset2D{.x = 0, .y = 0},
		.extent = _presentation_state.surface_state.caps.currentExtent,
	};

	const VkRenderingInfo rendering_info = VkRenderingInfo{
		.sType				  = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.pNext				  = nullptr,
		.flags				  = 0,
		.renderArea			  = render_area,
		.layerCount			  = 1,
		.viewMask			  = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments	  = &color_attachment,
		.pDepthAttachment	  = &depth_attachment,
		.pStencilAttachment	  = &depth_attachment,
	};

	const VkImageMemoryBarrier2 attachments_to_optimal[] = {
		VkImageMemoryBarrier2{
			.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext				 = nullptr,
			.srcStageMask		 = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask		 = VK_ACCESS_2_NONE,
			.dstStageMask		 = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstAccessMask		 = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.oldLayout			 = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout			 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image				 = _presentation_state.swapchain_state.swapchain_images[acquired_image],
			.subresourceRange =
				VkImageSubresourceRange{
					.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel	= 0,
					.levelCount		= 1,
					.baseArrayLayer = 0,
					.layerCount		= 1,
				},
		},
		VkImageMemoryBarrier2{
			.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext				 = nullptr,
			.srcStageMask		 = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask		 = VK_ACCESS_2_NONE,
			.dstStageMask		 = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
			.dstAccessMask		 = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout			 = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout			 = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = raw_ptr(_presentation_state.swapchain_state.depth_stencil_images[acquired_image].image),
			.subresourceRange =
				VkImageSubresourceRange{
					.aspectMask		= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
					.baseMipLevel	= 0,
					.levelCount		= 1,
					.baseArrayLayer = 0,
					.layerCount		= 1,
				},
		},
	};

	const VkDependencyInfo dependency_info = {
		.sType					  = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext					  = nullptr,
		.dependencyFlags		  = VK_DEPENDENCY_BY_REGION_BIT,
		.memoryBarrierCount		  = 0,
		.pMemoryBarriers		  = nullptr,
		.bufferMemoryBarrierCount = 0,
		.pBufferMemoryBarriers	  = nullptr,
		.imageMemoryBarrierCount  = size(attachments_to_optimal),
		.pImageMemoryBarriers	  = attachments_to_optimal,
	};

	//
	// move rendering attachments from undefined to optimal layout
	WRAP_VULKAN_FUNC(
		vkCmdPipelineBarrier2, _presentation_state.command_buffers[_presentation_state.frame_index], &dependency_info
	);

	//
	// do any pending ownership transfers
	// TODO: move this to scratchpad and remove fn

	if (!_ownership_transfers.empty()) {
		const vector<VkImageMemoryBarrier2> mem_barriers =
			_ownership_transfers % fn::transform([this](const BindlessResourceHandle_Image img) {
				const BindlessResourceEntry_Image& img_data = *bindless_sys().image_entry(img);

				return VkImageMemoryBarrier2{
					.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
					.pNext				 = nullptr,
					.srcStageMask		 = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
					.srcAccessMask		 = VK_ACCESS_2_NONE,
					.dstStageMask		 = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
					.dstAccessMask		 = VK_ACCESS_2_SHADER_READ_BIT,
					.oldLayout			 = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.newLayout			 = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.srcQueueFamilyIndex = _render_state.queues[1].index,
					.dstQueueFamilyIndex = _render_state.queues[0].index,
					.image				 = img_data.handle,
					.subresourceRange =
						VkImageSubresourceRange{
							.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel	= 0,
							.levelCount		= img_data.info.levelCount,
							.baseArrayLayer = 0,
							.layerCount		= img_data.info.layerCount,
						},
				};
			}) %
			fn::to_vector();

		const VkDependencyInfo dep_info{
			.sType					  = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext					  = nullptr,
			.dependencyFlags		  = VK_DEPENDENCY_BY_REGION_BIT,
			.memoryBarrierCount		  = 0,
			.pMemoryBarriers		  = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers	  = nullptr,
			.imageMemoryBarrierCount  = static_cast<uint32_t>(size(mem_barriers)),
			.pImageMemoryBarriers	  = mem_barriers.data(),
		};

		vkCmdPipelineBarrier2(_presentation_state.command_buffers[_presentation_state.frame_index], &dependency_info);
		_ownership_transfers.clear();
	}

	WRAP_VULKAN_FUNC(
		vkCmdBeginRendering, _presentation_state.command_buffers[_presentation_state.frame_index], &rendering_info
	);

	return FrameRenderData{
		.id			= _presentation_state.frame_index,
		.max_frames = _presentation_state.max_frames,
		.cmd_buf	= _presentation_state.command_buffers[_presentation_state.frame_index],
		.fbsize		= _presentation_state.surface_state.caps.currentExtent,
		.fb_f32 =
			{
				.width	= static_cast<float>(_presentation_state.surface_state.caps.currentExtent.width),
				.height = static_cast<float>(_presentation_state.surface_state.caps.currentExtent.height),
			},
	};
}

void VulkanRenderer::end_rendering() {
	ZoneScopedN("EndRendering");
	vkCmdEndRendering(_presentation_state.command_buffers[_presentation_state.frame_index]);

	const uint32_t acquired_swapchain_image = _presentation_state.acquired_image;
	//
	// move rendered image from ATTACHMENT_OPTIMAL to SRC_PRESENT
	const VkImageMemoryBarrier2 color_attachment_optimal_to_src_present = {
		.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext				 = nullptr,
		.srcStageMask		 = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask		 = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask		 = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		.dstAccessMask		 = VK_ACCESS_2_NONE,
		.oldLayout			 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout			 = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.image				 = _presentation_state.swapchain_state.swapchain_images[acquired_swapchain_image],
		.subresourceRange =
			VkImageSubresourceRange{
				.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel	= 0,
				.levelCount		= VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount		= VK_REMAINING_ARRAY_LAYERS,
			},
	};

	const VkDependencyInfo dependency_info = {
		.sType					  = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext					  = nullptr,
		.dependencyFlags		  = VK_DEPENDENCY_BY_REGION_BIT,
		.memoryBarrierCount		  = 0,
		.pMemoryBarriers		  = nullptr,
		.bufferMemoryBarrierCount = 0,
		.pBufferMemoryBarriers	  = nullptr,
		.imageMemoryBarrierCount  = 1,
		.pImageMemoryBarriers	  = &color_attachment_optimal_to_src_present,
	};

	WRAP_VULKAN_FUNC(
		vkCmdPipelineBarrier2, _presentation_state.command_buffers[_presentation_state.frame_index], &dependency_info
	);

	WRAP_VULKAN_FUNC(vkEndCommandBuffer, _presentation_state.command_buffers[_presentation_state.frame_index]);

	//
	// https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
	const VkSemaphoreSubmitInfo semaphore_wait_img_available = {
		.sType		 = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext		 = nullptr,
		.semaphore	 = raw_ptr(_presentation_state.swapchain_state.sync.present_sem[_presentation_state.frame_index]),
		.value		 = 0,
		.stageMask	 = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		.deviceIndex = 0,
	};

	const VkSemaphoreSubmitInfo semaphore_signal_rendering_done = {
		.sType		 = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
		.pNext		 = nullptr,
		.semaphore	 = raw_ptr(_presentation_state.swapchain_state.sync.rendering_sem[acquired_swapchain_image]),
		.value		 = 0,
		.stageMask	 = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		.deviceIndex = 0,
	};

	const VkCommandBufferSubmitInfo cmd_buffer_submit_info = {
		.sType		   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext		   = nullptr,
		.commandBuffer = _presentation_state.command_buffers[_presentation_state.frame_index],
		.deviceMask	   = 0,
	};

	const VkSubmitInfo2 submit_info = {
		.sType					  = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext					  = nullptr,
		.flags					  = 0,
		.waitSemaphoreInfoCount	  = 1,
		.pWaitSemaphoreInfos	  = &semaphore_wait_img_available,
		.commandBufferInfoCount	  = 1,
		.pCommandBufferInfos	  = &cmd_buffer_submit_info,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos	  = &semaphore_signal_rendering_done,
	};

	const VkResult submit_result{
		WRAP_VULKAN_FUNC(
			vkQueueSubmit2,
			_render_state.queues[0].handle,
			1,
			&submit_info,
			raw_ptr(_presentation_state.swapchain_state.sync.fences[_presentation_state.frame_index])
		),
	};

	if (submit_result != VK_SUCCESS) {
		// TODO: handle submit error ??!!
	}

	const VkSwapchainKHR swap_chains[]			= {raw_ptr(_presentation_state.swapchain_state.swapchain)};
	const VkSemaphore present_wait_semaphores[] = {
		raw_ptr(_presentation_state.swapchain_state.sync.rendering_sem[acquired_swapchain_image])
	};

	const VkPresentInfoKHR present_info = {
		.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext				= nullptr,
		.waitSemaphoreCount = static_cast<uint32_t>(size(present_wait_semaphores)),
		.pWaitSemaphores	= present_wait_semaphores,
		.swapchainCount		= static_cast<uint32_t>(size(swap_chains)),
		.pSwapchains		= swap_chains,
		.pImageIndices		= &acquired_swapchain_image,
		.pResults			= nullptr,
	};

	const VkResult present_result{WRAP_VULKAN_FUNC(vkQueuePresentKHR, _render_state.queues[0].handle, &present_info)};
	_presentation_state.frame_index = (_presentation_state.frame_index + 1) % _presentation_state.max_frames;

	if (present_result != VK_SUCCESS) {
		if (present_result == VK_ERROR_OUT_OF_DATE_KHR || present_result == VK_SUBOPTIMAL_KHR) {
			handle_swapchain_suboptimal_out_of_date(SwapchainReacquireAfterSuboptimal::Never_);
		}
	}
}

void VulkanRenderer::handle_swapchain_suboptimal_out_of_date(
	const VulkanRenderer::SwapchainReacquireAfterSuboptimal reacquire
) {
	XR_LOG_INFO("Swapchain is sub-optimal/out-of-date, trying to recreate ...");

	VkSurfaceCapabilitiesKHR surface_caps;
	const VkResult query_result = WRAP_VULKAN_FUNC(
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
		_render_state.dev_physical.device,
		raw_ptr(_presentation_state.surface_state.surface),
		&surface_caps
	);

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
			.device			   = raw_ptr(_render_state.dev_logical),
			.retired_swapchain = raw_ptr(_presentation_state.swapchain_state.swapchain),
			.surface		   = raw_ptr(_presentation_state.surface_state.surface),
			.surface_caps	   = surface_caps,
			.fmt			   = _presentation_state.surface_state.format,
			.present_mode	   = _presentation_state.surface_state.present_mode,
			.mem_props		   = _render_state.dev_physical.memory_properties.memoryProperties,
			.image_count	   = static_cast<uint32_t>(_presentation_state.max_frames),
			.dimensions =
				VkExtent3D{
					.width	= surface_caps.currentExtent.width,
					.height = surface_caps.currentExtent.height,
					.depth	= 1,
				},
			.depth_att_format = _presentation_state.surface_state.depth_stencil_format,
		};

		create_swapchain_state(swapchain_state_creation_info)
			.map_or_else(
				[&](detail::SwapchainState new_swapchain_state) {
					//
					// assign new swapchain state
					_presentation_state.swapchain_state = std::move(new_swapchain_state);
					_presentation_state.state_bits &= ~detail::PresentationState::STATE_SWAPCHAIN_SUBOPTIMAL;
					_presentation_state.surface_state.caps = surface_caps;
					_presentation_state.frame_index		   = 0;

					if (reacquire == VulkanRenderer::SwapchainReacquireAfterSuboptimal::Always_) {
						const uint32_t previous_acquired_image{_presentation_state.acquired_image};

						WRAP_VULKAN_FUNC(
							vkAcquireNextImageKHR,
							raw_ptr(_render_state.dev_logical),
							raw_ptr(_presentation_state.swapchain_state.swapchain),
							numeric_limits<uint64_t>::max(),
							raw_ptr(
								_presentation_state.swapchain_state.sync.present_sem[_presentation_state.frame_index]
							),
							nullptr,
							&_presentation_state.acquired_image
						);

						XR_LOG_INFO(
							"swapchain recreation previous acquired image {}, re-acquired image {}",
							previous_acquired_image,
							_presentation_state.acquired_image
						);

						const VkFence reset_fences[] = {
							raw_ptr(_presentation_state.swapchain_state.sync.fences[_presentation_state.frame_index])
						};
						WRAP_VULKAN_FUNC(vkResetFences, this->device(), 1, reset_fences);
					}
				},
				[]() { XR_LOG_CRITICAL("Could not create new swapchain state"); }
			);
	}
}

void VulkanRenderer::clear_attachments(
	VkCommandBuffer cmd_buf,
	const float red,
	const float green,
	const float blue,
	const float depth,
	const uint32_t stencil
) {
	const VkClearAttachment clear_attachments[] = {
		VkClearAttachment{
			.aspectMask		 = VK_IMAGE_ASPECT_COLOR_BIT,
			.colorAttachment = 0,
			.clearValue		 = VkClearValue{.color = VkClearColorValue{red, green, blue, 1.0f}},
		},
		VkClearAttachment{
			.aspectMask		 = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT,
			.colorAttachment = 0,
			.clearValue		 = VkClearValue{.depthStencil = VkClearDepthStencilValue{depth, stencil}},
		}
	};

	const VkExtent2D surface_extent = _presentation_state.surface_state.caps.currentExtent;
	const VkRect2D clear_area{.offset = {0, 0}, .extent = surface_extent};

	const VkClearRect clear_rects[] = {
		VkClearRect{.rect = clear_area, .baseArrayLayer = 0, .layerCount = 1},
		VkClearRect{.rect = clear_area, .baseArrayLayer = 0, .layerCount = 1},
	};

	vkCmdClearAttachments(
		cmd_buf,
		static_cast<uint32_t>(size(clear_attachments)),
		clear_attachments,
		static_cast<uint32_t>(size(clear_rects)),
		clear_rects
	);
}

void VulkanRenderer::wait_device_idle() noexcept {
	XR_LOG_INFO("Waiting GPU device idle ...");
	WRAP_VULKAN_FUNC(vkDeviceWaitIdle, this->device());
}

uint32_t xray::rendering::VulkanRenderer::find_allocation_memory_type(
	const uint32_t memory_requirements, const VkMemoryPropertyFlags required_flags
) const noexcept {
	return vk_find_allocation_memory_type(
		_render_state.dev_physical.memory_properties.memoryProperties, memory_requirements, required_flags
	);
}

tl::expected<UniqueMemoryMapping, VulkanError> UniqueMemoryMapping::map_memory(
	VkDevice device, VkDeviceMemory memory, const uint64_t offset, const uint64_t size
) noexcept {
	const VkDeviceSize mapping_length = size == 0 ? VK_WHOLE_SIZE : size;
	void* mapped_addr{};
	const VkResult mapping_result =
		WRAP_VULKAN_FUNC(vkMapMemory, device, memory, offset, mapping_length, 0, &mapped_addr);
	XR_VK_CHECK_RESULT(mapping_result);

	return tl::expected<UniqueMemoryMapping, VulkanError>{UniqueMemoryMapping{mapped_addr, memory, device, 0, offset}};
}

UniqueMemoryMapping::~UniqueMemoryMapping() {
	if (_mapped_memory) {
		const VkMappedMemoryRange mapped_memory_range{
			.sType	= VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.pNext	= nullptr,
			.memory = _device_memory,
			.offset = _mapped_offset,
			.size	= _mapped_size,
		};

		WRAP_VULKAN_FUNC(vkFlushMappedMemoryRanges, _device, 1, &mapped_memory_range);
		WRAP_VULKAN_FUNC(vkUnmapMemory, _device, _device_memory);
	}
}

tl::optional<detail::SwapchainState> create_swapchain_state(const SwapchainStateCreationInfo& create_info) {
	//
	// SwapchainKHR object
	xrUniqueVkSwapchainKHR swapchain{
		[&]() {
			const VkSwapchainCreateInfoKHR swapchain_create_info = {
				.sType				   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
				.pNext				   = nullptr,
				.flags				   = 0,
				.surface			   = create_info.surface,
				.minImageCount		   = create_info.image_count,
				.imageFormat		   = create_info.fmt.format,
				.imageColorSpace	   = create_info.fmt.colorSpace,
				.imageExtent		   = create_info.surface_caps.maxImageExtent,
				.imageArrayLayers	   = 1,
				.imageUsage			   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.imageSharingMode	   = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices   = nullptr,
				.preTransform		   = create_info.surface_caps.currentTransform,
				.compositeAlpha		   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
				.presentMode		   = create_info.present_mode,
				.clipped			   = false,
				.oldSwapchain		   = create_info.retired_swapchain,
			};

			VkSwapchainKHR swapchain{};
			WRAP_VULKAN_FUNC(vkCreateSwapchainKHR, create_info.device, &swapchain_create_info, nullptr, &swapchain);
			return swapchain;
		}(),
		VkResourceDeleter_VkSwapchainKHR{create_info.device},
	};

	XR_LOG_INFO(
		"Swapchain created: {:#x}, image count {}",
		reinterpret_cast<uintptr_t>(raw_ptr(swapchain)),
		create_info.image_count
	);

	//
	// acquire images
	vector<VkImage> swapchain_images{[&]() {
		uint32_t swapchain_image_count{};
		WRAP_VULKAN_FUNC(
			vkGetSwapchainImagesKHR, create_info.device, raw_ptr(swapchain), &swapchain_image_count, nullptr
		);

		vector<VkImage> swapchain_images{swapchain_image_count};
		WRAP_VULKAN_FUNC(
			vkGetSwapchainImagesKHR,
			create_info.device,
			raw_ptr(swapchain),
			&swapchain_image_count,
			swapchain_images.data()
		);

		return swapchain_images;
	}()};

	//
	// create image_views
	vector<xrUniqueVkImageView> swapchain_image_views =
		swapchain_images % fn::transform([&](VkImage image) {
			const VkImageViewCreateInfo imageview_create_info = {
				.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext		= nullptr,
				.flags		= 0,
				.image		= image,
				.viewType	= VK_IMAGE_VIEW_TYPE_2D,
				.format		= create_info.fmt.format,
				.components = VkComponentMapping{},
				.subresourceRange =
					VkImageSubresourceRange{
						.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel	= 0,
						.levelCount		= 1,
						.baseArrayLayer = 0,
						.layerCount		= 1,
					},
			};

			VkImageView image_view{};
			WRAP_VULKAN_FUNC(vkCreateImageView, create_info.device, &imageview_create_info, nullptr, &image_view);

			return xrUniqueVkImageView{
				image_view,
				VkResourceDeleter_VkImageView{create_info.device},
			};
		}) %
		fn::where([](const xrUniqueVkImageView& img_view) { return static_cast<bool>(img_view); }) %
		fn::to(vector<xrUniqueVkImageView>{});

	if (swapchain_image_views.size() != create_info.image_count) {
		XR_LOG_ERR_FILE_LINE(
			"Failed to create image views (needed {}, created {})",
			create_info.image_count,
			swapchain_image_views.size()
		);
		return tl::nullopt;
	}

	//
	// depth stencil images + image views
	vector<xrUniqueVkImageView> depth_stencil_image_views;
	vector<UniqueImage> depth_stencil_images;

	for (uint32_t idx = 0; idx < create_info.image_count; ++idx) {
		const VkImageCreateInfo image_create_info = {
			.sType	   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext	   = nullptr,
			.flags	   = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format	   = create_info.depth_att_format,
			.extent =
				VkExtent3D{.width = create_info.dimensions.width, .height = create_info.dimensions.height, .depth = 1},
			.mipLevels			   = 1,
			.arrayLayers		   = 1,
			.samples			   = VK_SAMPLE_COUNT_1_BIT,
			.tiling				   = VK_IMAGE_TILING_OPTIMAL,
			.usage				   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode		   = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices   = nullptr,
			.initialLayout		   = VK_IMAGE_LAYOUT_UNDEFINED,
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
			VkResourceDeleter_VkImage{create_info.device},
		};

		xrUniqueVkDeviceMemory ds_image_memory{
			[&]() {
				VkMemoryRequirements mem_req;
				vkGetImageMemoryRequirements(create_info.device, raw_ptr(ds_image), &mem_req);
				const VkMemoryAllocateInfo mem_alloc_info = {
					.sType			 = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					.pNext			 = nullptr,
					.allocationSize	 = mem_req.size,
					.memoryTypeIndex = vk_find_allocation_memory_type(
						create_info.mem_props, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
					),
				};

				VkDeviceMemory image_memory{};
				WRAP_VULKAN_FUNC(vkAllocateMemory, create_info.device, &mem_alloc_info, nullptr, &image_memory);
				if (image_memory)
					WRAP_VULKAN_FUNC(vkBindImageMemory, create_info.device, raw_ptr(ds_image), image_memory, 0);
				return image_memory;
			}(),
			VkResourceDeleter_VkDeviceMemory{create_info.device},
		};

		const VkImageViewCreateInfo depth_stencil_view_create_info = {
			.sType		= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext		= nullptr,
			.flags		= 0,
			.image		= raw_ptr(ds_image),
			.viewType	= VK_IMAGE_VIEW_TYPE_2D,
			.format		= create_info.depth_att_format,
			.components = VkComponentMapping{},
			.subresourceRange =
				VkImageSubresourceRange{
					.aspectMask		= VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
					.baseMipLevel	= 0,
					.levelCount		= 1,
					.baseArrayLayer = 0,
					.layerCount		= 1,
				},
		};

		xrUniqueVkImageView ds_image_view{
			[&]() {
				VkImageView image_view{};
				WRAP_VULKAN_FUNC(
					vkCreateImageView, create_info.device, &depth_stencil_view_create_info, nullptr, &image_view
				);
				return image_view;
			}(),
			VkResourceDeleter_VkImageView{create_info.device},
		};

		if (ds_image && ds_image_view) {
			depth_stencil_images.emplace_back(std::move(ds_image), std::move(ds_image_memory));
			depth_stencil_image_views.push_back(std::move(ds_image_view));
		}
	}

	if (depth_stencil_image_views.size() != create_info.image_count &&
		depth_stencil_images.size() != create_info.image_count) {
		XR_LOG_ERR(
			"Failed to create all the required {} depth stencil images and image views", create_info.image_count
		);

		return tl::nullopt;
	}

	//
	// sync objects
	const size_t sync_objects_count = swapchain_image_views.size();
	vector<xrUniqueVkFence> fences	= [sync_objects_count, &create_info]() {
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

			 if (!fence) break;

			 fences.emplace_back(fence, VkResourceDeleter_VkFence{create_info.device});
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

			if (!semaphore) break;

			semaphores.emplace_back(semaphore, VkResourceDeleter_VkSemaphore{device});
		}

		return semaphores;
	};

	vector<xrUniqueVkSemaphore> rendering_semaphores{make_semaphores_fn()};
	vector<xrUniqueVkSemaphore> presentation_semaphores{make_semaphores_fn()};

	assert(rendering_semaphores.size() == sync_objects_count && presentation_semaphores.size() == sync_objects_count);

	return tl::make_optional<detail::SwapchainState>(
		std::move(swapchain),
		std::move(swapchain_images),
		std::move(swapchain_image_views),
		std::move(depth_stencil_images),
		std::move(depth_stencil_image_views),
		detail::SyncState{
			std::move(fences),
			std::move(presentation_semaphores),
			std::move(rendering_semaphores),
		}
	);
}

void VulkanRenderer::dbg_set_object_name(const uint64_t object, const VkObjectType object_type, const char* name)
	const noexcept {
	const VkDebugUtilsObjectNameInfoEXT obj_name{
		.sType		  = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext		  = nullptr,
		.objectType	  = object_type,
		.objectHandle = object,
		.pObjectName  = name,
	};
	vkfn::SetDebugUtilsObjectNameEXT(raw_ptr(_render_state.dev_logical), &obj_name);
}

[[nodiscard]] DebugMarkerEndScoped VulkanRenderer::dbg_marker_begin(
	VkCommandBuffer cmd_buf, const char* name, const rgb_color color
) noexcept {
	const VkDebugUtilsLabelEXT debug_label{
		.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext		= nullptr,
		.pLabelName = name,
		.color		= {color.r, color.g, color.b, color.a},
	};

	vkfn::CmdBeginDebugUtilsLabelEXT(cmd_buf, &debug_label);
	return DebugMarkerEndScoped{cmd_buf};
}

void VulkanRenderer::dbg_marker_end(VkCommandBuffer cmd_buf) noexcept { vkfn::CmdEndDebugUtilsLabelEXT(cmd_buf); }

void VulkanRenderer::dbg_marker_insert(VkCommandBuffer cmd_buf, const char* name, const rgb_color color) noexcept {
	const VkDebugUtilsLabelEXT debug_marker{
		.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext		= nullptr,
		.pLabelName = name,
		.color		= {color.r, color.g, color.b, color.a},
	};

	vkfn::CmdInsertDebugUtilsLabelEXT(cmd_buf, &debug_marker);
}

tl::expected<QueuedJob, xray::rendering::VulkanError> xray::rendering::VulkanRenderer::create_job(const QueueType qtype
) noexcept {
	QueueData qdata{queue_data(qtype)};

	const VkCommandBufferAllocateInfo alloc_info = {
		.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext				= nullptr,
		.commandPool		= qdata.cmdpool,
		.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBuffer cmd_buffer{};
	{
		std::unique_lock<xray::base::concurrency::spin_mutex> queue_lock{qdata.cmdpool_lock};
		const VkResult alloc_cmdbuffs_res =
			WRAP_VULKAN_FUNC(vkAllocateCommandBuffers, raw_ptr(_render_state.dev_logical), &alloc_info, &cmd_buffer);
		XR_VK_CHECK_RESULT(alloc_cmdbuffs_res);
	}

	const VkCommandBufferBeginInfo cmd_buf_begin_info{
		.sType			  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext			  = nullptr,
		.flags			  = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr,
	};
	vkBeginCommandBuffer(cmd_buffer, &cmd_buf_begin_info);

	return tl::expected<QueuedJob, VulkanError>{QueuedJob{.buffer = cmd_buffer, .queue_type = qtype}};
}

tl::expected<xray::rendering::QueueSubmitWaitToken, xray::rendering::VulkanError>
xray::rendering::VulkanRenderer::submit_job(QueuedJob queued_job) noexcept {
	vkEndCommandBuffer(queued_job.buffer);

	using namespace xray::base;

	xrUniqueVkFence wait_fence{nullptr, VkResourceDeleter_VkFence{device()}};
	const VkFenceCreateInfo fence_create_info{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};
	const VkResult fence_create_status =
		WRAP_VULKAN_FUNC(vkCreateFence, device(), &fence_create_info, nullptr, raw_ptr_ptr(wait_fence));
	XR_VK_CHECK_RESULT(fence_create_status);

	const VkSubmitInfo submit_info{
		.sType				  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext				  = nullptr,
		.waitSemaphoreCount	  = 0,
		.pWaitSemaphores	  = nullptr,
		.pWaitDstStageMask	  = nullptr,
		.commandBufferCount	  = 1,
		.pCommandBuffers	  = &queued_job.buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores	  = nullptr,
	};

	{
		QueueData qdata{queue_data(queued_job.queue_type)};
		std::unique_lock<xray::base::concurrency::spin_mutex> submit_lock{qdata.cmdpool_lock};
		const VkResult submit_result =
			WRAP_VULKAN_FUNC(vkQueueSubmit, qdata.handle, 1, &submit_info, raw_ptr(wait_fence));
		XR_VK_CHECK_RESULT(submit_result);
	}

	return QueueSubmitWaitToken{this, queued_job.buffer, std::move(wait_fence), queued_job.queue_type};
}

xray::rendering::QueueSubmitWaitToken::~QueueSubmitWaitToken() {
	if (!_waited_on) {
		_r->consume_wait_token(std::move(*this));
	}
}

void xray::rendering::VulkanRenderer::consume_wait_token(QueueSubmitWaitToken wait_token) noexcept {
	const VkFence fences[] = {wait_token.fence()};
	WRAP_VULKAN_FUNC(
		vkWaitForFences,
		device(),
		static_cast<uint32_t>(std::size(fences)),
		fences,
		true,
		std::numeric_limits<uint64_t>::max()
	);
	vkDestroyFence(device(), fences[0], nullptr);
	QueueData qdata = queue_data(wait_token.queue());
	std::unique_lock<xray::base::concurrency::spin_mutex> pool_lock{qdata.cmdpool_lock};

	const VkCommandBuffer cmd_buffs[] = {wait_token.command_buffer()};
	vkFreeCommandBuffers(device(), qdata.cmdpool, static_cast<uint32_t>(std::size(cmd_buffs)), cmd_buffs);
	wait_token._waited_on = true;
}

void xray::rendering::VulkanRenderer::consume_many_wait_tokens(
	xray::base::MemoryArena& arena, std::span<QueueSubmitWaitToken> tokens
) {
	if (tokens.empty()) return;

	ScratchPadArena scratch_pad{&arena};
	const containers::vector<VkFence> wait_fences =
		tokens | lz::map([](QueueSubmitWaitToken& token) {
			token._waited_on = true;
			return token.fence();
		}) |
		lz::to<containers::vector<VkFence>>(MemoryArenaAllocator<VkFence>{arena});

	WRAP_VULKAN_FUNC(
		vkWaitForFences,
		device(),
		static_cast<uint32_t>(wait_fences.size()),
		wait_fences.data(),
		true,
		std::numeric_limits<uint64_t>::max()
	);

	for (VkFence f : wait_fences) {
		vkDestroyFence(device(), f, nullptr);
	}

	const QueueData qdata = queue_data(tokens[0].queue());

	const containers::vector<VkCommandBuffer> cmd_buffers =
		tokens | lz::map([](const QueueSubmitWaitToken& token) { return token.command_buffer(); }) |
		lz::to<containers::vector<VkCommandBuffer>>(MemoryArenaAllocator<VkCommandBuffer>{arena});

	std::unique_lock<xray::base::concurrency::spin_mutex> pool_lock{qdata.cmdpool_lock};
	vkFreeCommandBuffers(device(), qdata.cmdpool, static_cast<uint32_t>(std::size(cmd_buffers)), cmd_buffers.data());
}

uint32_t vk_format_bytes_size(const VkFormat format) {
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

}  // namespace xray::rendering
