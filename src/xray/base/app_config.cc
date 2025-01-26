#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/logger.hpp"

#include <filesystem>
#include <fmt/std.h>

using namespace xray::base;

xray::base::ConfigSystem* xray::base::ConfigSystem::_unique_instance;

xray::base::ConfigSystem::ConfigSystem(const char* cfg_path /*= nullptr*/)
{
    assert((_unique_instance == nullptr) && "Already initialized!");
    _unique_instance = this;

    //
    // set defaults
    paths_.model_path = "assets/models";
    paths_.texture_path = "assets/textures";
    paths_.shader_path = "assets/shaders";
    paths_.fonts_path = "assets/fonts";
    paths_.config_path = "config";
    paths_.camera_cfg_path = "config/camera";

    //
    // TODO: hardcoded right now
    std::error_code err{};
    const std::filesystem::path cwd{ std::filesystem::current_path(err) };
    if (!err) {
        paths_.config_path = cwd / "config";
    }

    XR_LOG_INFO("CWD = {}, config path {}", cwd.generic_string(), paths_.config_path.generic_string());

    const auto config_file_path = cfg_path ? cfg_path : "config/app_config.conf";

    namespace fs = std::filesystem;
    const fs::path absolute_path{ fs::absolute(config_file_path) };

    assert(fs::exists(absolute_path));
    assert(fs::file_size(absolute_path) > 0);

    config_file app_conf_file;
    if (!app_conf_file.read_file(absolute_path.generic_string().c_str())) {
        XR_LOG_CRITICAL("Could not open configuration file {}", absolute_path.generic_string());
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
        XR_LOG_CRITICAL("Missing {} in config file", ROOT_DIR_ENTRY);
        return;
    }

    if (root_dir) {
        paths_.root_path = root_dir;
        assert(paths_.root_path.is_absolute());
    }

    struct PathWithConfigEntry
    {
        std::string_view conf_file_entry_name;
        std::filesystem::path* path;
    };

    //
    //  List of predefined paths we look for in the config file.
    PathWithConfigEntry paths_to_load[] = {
        { "directories.shaders", &paths_.shader_path },
        { "directories.models", &paths_.model_path },
        { "directories.textures", &paths_.texture_path },
        { "directories.fonts", &paths_.fonts_path },
    };

    for (auto& path_load_info : paths_to_load) {
        const char* path_value{ nullptr };
        app_conf_file.lookup_value(path_load_info.conf_file_entry_name.data(), path_value);

        if (!path_value) {
            auto dotpos = path_load_info.conf_file_entry_name.find('.');
            assert(dotpos != std::string_view::npos);
            const std::string_view item_name = path_load_info.conf_file_entry_name.substr(dotpos + 1);
            const std::filesystem::path item_path = cwd / item_name;

            XR_LOG_INFO(
                "[CFG] resolved by relative to binary {} -> {}", path_load_info.conf_file_entry_name, item_path);
            assert(std::filesystem::exists(item_path));
            assert(std::filesystem::is_directory(item_path));

            *path_load_info.path = item_path;
        } else {
            *path_load_info.path = path_value;

            if (!path_load_info.path->is_absolute())
                *path_load_info.path = paths_.root_path / *path_load_info.path;

            XR_LOG_INFO("[CFG] resolved by config {} -> {}",
                        path_load_info.conf_file_entry_name,
                        path_load_info.path->generic_string());
            assert(std::filesystem::exists(*path_load_info.path));
            assert(std::filesystem::is_directory(*path_load_info.path));
        }
    }
}
