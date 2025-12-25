#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"

#include <filesystem>
#include <fmt/std.h>

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <unistd.h>
#else
#error Not implemented for this OS
#endif

using namespace xray::base;

namespace {

std::filesystem::path get_process_path() {
#if defined(XRAY_OS_IS_LINUX)
	char scratch_buffer[1024];
	const ssize_t bytes_read = readlink("/proc/self/exe", scratch_buffer, std::size(scratch_buffer) - 1);
	if (bytes_read > 0) {
		scratch_buffer[bytes_read] = 0;
		return std::filesystem::path{scratch_buffer};
	}

#endif

	return std::filesystem::current_path();
}

}  // namespace

xray::base::ConfigSystem::ConfigSystem() {
	//
	// init default to exe directory
	FileSys.ExePathAbsolute		 = get_process_path();
	FileSys.RootPathAbsolute	 = FileSys.ExePathAbsolute.parent_path();
	FileSys.GameRootPathAbsolute = FileSys.ExePathAbsolute.parent_path();

	//
	// set defaults
	paths_.model_path	   = FileSys.RootPathAbsolute / "assets/models";
	paths_.texture_path	   = FileSys.RootPathAbsolute / "assets/textures";
	paths_.shader_path	   = FileSys.RootPathAbsolute / "assets/shaders";
	paths_.fonts_path	   = FileSys.RootPathAbsolute / "assets/fonts";
	paths_.config_path	   = FileSys.RootPathAbsolute / "config";
	paths_.camera_cfg_path = FileSys.RootPathAbsolute / "config/camera";

	namespace fs = std::filesystem;

	//
	// if this file exists it specifies overrides for the folders
	const auto cfg_path = FileSys.RootPathAbsolute / "config/app_config.conf";
	assert(fs::exists(cfg_path));
	assert(fs::file_size(cfg_path) > 0);

	config_file app_conf_file;
	if (!app_conf_file.read_file(cfg_path.generic_string().c_str())) {
		return;
	}

	const char* root_dir = nullptr;

	constexpr const char* const ROOT_DIR_ENTRY =
#if defined(XRAY_OS_IS_WINDOWS)
		"directories.root_win";
#else
		"directories.root";
#endif

	if (!app_conf_file.lookup_value(ROOT_DIR_ENTRY, root_dir)) {
		return;
	}

	//
	// root dir must be absolute path
	if (root_dir) {
		const fs::path root_path{root_dir};
		if (!root_path.is_absolute()) {
			return;
		}
		FileSys.GameRootPathAbsolute = root_path;
		paths_.root_path			 = root_path;
	}

	//
	// if path is absolute use as is, if not concat with root dir
	struct PathWithConfigEntry {
		std::string_view conf_file_entry_name;
		std::filesystem::path* path;
	};

	//
	//  List of predefined paths we look for in the config file.
	PathWithConfigEntry paths_to_load[] = {
		{"directories.shaders", &paths_.shader_path},
		{"directories.models", &paths_.model_path},
		{"directories.textures", &paths_.texture_path},
		{"directories.fonts", &paths_.fonts_path},
	};

	for (auto& path_load_info : paths_to_load) {
		const char* path_value{nullptr};
		app_conf_file.lookup_value(path_load_info.conf_file_entry_name.data(), path_value);

		if (!path_value) {
			auto dotpos = path_load_info.conf_file_entry_name.find('.');
			assert(dotpos != std::string_view::npos);
			const std::string_view item_name	  = path_load_info.conf_file_entry_name.substr(dotpos + 1);
			const std::filesystem::path item_path = FileSys.RootPathAbsolute / item_name;

			assert(std::filesystem::exists(item_path));
			assert(std::filesystem::is_directory(item_path));

			*path_load_info.path = item_path;
		} else {
			*path_load_info.path = path_value;

			if (!path_load_info.path->is_absolute()) *path_load_info.path = paths_.root_path / *path_load_info.path;

			assert(std::filesystem::exists(*path_load_info.path));
			assert(std::filesystem::is_directory(*path_load_info.path));
		}
	}
}
