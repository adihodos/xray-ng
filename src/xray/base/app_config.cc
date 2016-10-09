#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/logger.hpp"
#include <platformstl/filesystem/current_directory.hpp>
#include <platformstl/filesystem/filesystem_traits.hpp>
#include <platformstl/filesystem/path.hpp>

using namespace xray::base;

xray::base::app_config* xray::base::app_config::_unique_instance;

xray::base::app_config::app_config(const char* cfg_path /*= nullptr*/) {

  assert((_unique_instance == nullptr) && "Already initialized!");
  _unique_instance = this;

  //
  // set defaults
  paths_.model_path       = "assets/models";
  paths_.texture_path     = "assets/textures";
  paths_.shader_path      = "assets/shaders";
  paths_.fonts_path       = "assets/fonts";
  paths_.camera_cfg_path  = "config/camera";
  paths_.objects_cfg_path = "config/objects";

  const auto config_file_path = cfg_path ? cfg_path : "config/app_config.conf";

  config_file app_conf_file;
  if (!app_conf_file.read_file(config_file_path)) {
    XR_LOG_ERR("Failed to open config file {}", config_file_path);
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
    XR_LOG_ERR("Root directory not defined !");
    return;
  }

  if (root_dir) {
    paths_.root_path = root_dir;
    if (!paths_.root_path.has_sep())
      paths_.root_path.push_sep();

    assert(paths_.root_path.is_absolute());
  }

  struct path_with_conf_entry_t {
    const char*          conf_file_entry_name;
    platformstl::path_a* path;
  };

  //
  //  List of predefined paths we look for in the config file.
  path_with_conf_entry_t paths_to_load[] = {
      {"directories.shaders", &paths_.shader_path},
      {"directories.models", &paths_.model_path},
      {"directories.textures", &paths_.texture_path},
      {"directories.fonts", &paths_.fonts_path},
      {"directories.shader_configs", &paths_.shader_cfg_path},
      {"directories.camera_configs", &paths_.camera_cfg_path},
      {"directories.object_configs", &paths_.objects_cfg_path},
      {"directories.engine_ini", &paths_.engine_ini_file}};

  for (auto& path_load_info : paths_to_load) {
    const char* path_value{nullptr};
    app_conf_file.lookup_value(path_load_info.conf_file_entry_name, path_value);

    platformstl::path_a loaded_path{path_value ? path_value
                                               : path_load_info.path->c_str()};
    if (platformstl::filesystem_traits<char>::is_directory(
            loaded_path.c_str()) &&
        !loaded_path.has_sep()) {
      loaded_path.push_sep();
    }

    //
    //  If path is not absolute, make it so by appending the root first.
    //  Thus, having a root of c:/mydir and a path of "assets/shaders"
    //  will result in c:/mydir/assets/shaders/ .
    if (!loaded_path.is_absolute())
      *path_load_info.path = paths_.root_path;

    path_load_info.path->push(loaded_path);
  }

  XR_LOG_INFO("Dumping configured directories/paths :");
  for (const auto& pi : paths_to_load) {
    XR_LOG_INFO("{} = {}", pi.conf_file_entry_name, pi.path->c_str());
  }
}
