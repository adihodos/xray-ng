#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/logger.hpp"

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
    paths_.camera_cfg_path = "config/camera";
    paths_.objects_cfg_path = "config/objects";

    const auto config_file_path = cfg_path ? cfg_path : "config/app_config.conf";

    config_file app_conf_file;
    if (!app_conf_file.read_file(config_file_path)) {
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

    if (root_dir) {
        paths_.root_path = root_dir;
        assert(paths_.root_path.is_absolute());
    }

    struct path_with_conf_entry_t
    {
        const char* conf_file_entry_name;
        std::filesystem::path* path;
    };

    //
    //  List of predefined paths we look for in the config file.
    path_with_conf_entry_t paths_to_load[] = {
        { "directories.shaders", &paths_.shader_path },
        { "directories.models", &paths_.model_path },
        { "directories.textures", &paths_.texture_path },
        { "directories.fonts", &paths_.fonts_path },
    };

    for (auto& path_load_info : paths_to_load) {
        const char* path_value{ nullptr };
        app_conf_file.lookup_value(path_load_info.conf_file_entry_name, path_value);

        if (path_value) {
            *path_load_info.path = path_value;
        }

        if (!path_load_info.path->is_absolute())
            *path_load_info.path = paths_.root_path / *path_load_info.path;

        assert(std::filesystem::exists(*path_load_info.path));
        assert(std::filesystem::is_directory(*path_load_info.path));

        XR_LOG_INFO("{} -> {}", path_load_info.conf_file_entry_name, path_load_info.path->generic_string());
    }
}
