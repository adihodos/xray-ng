#include "xray/base/config_settings.hpp"
#include <fmt/format.h>

int32_t xray::base::detail::setting_type_to_library_type(
    const xray::base::config_entry_type st) noexcept {
  switch (st) {
  case config_entry_type::group:
    return CONFIG_TYPE_GROUP;
    break;

  case config_entry_type::array:
    return CONFIG_TYPE_ARRAY;
    break;

  case config_entry_type::string:
    return CONFIG_TYPE_STRING;
    break;

  case config_entry_type::floating_point:
    return CONFIG_TYPE_FLOAT;
    break;

  case config_entry_type::integer:
    return CONFIG_TYPE_INT;
    break;

  case config_entry_type::integer64:
    return CONFIG_TYPE_INT64;
    break;

  default:
    break;
  }

  return CONFIG_TYPE_NONE;
}

xray::base::config_entry_type
xray::base::detail::library_type_to_setting_type(const int32_t lt) noexcept {
  switch (lt) {
  case CONFIG_TYPE_GROUP:
    return config_entry_type::group;
    break;

  case CONFIG_TYPE_INT:
    return config_entry_type::integer;
    break;

  case CONFIG_TYPE_INT64:
    return config_entry_type::integer64;
    break;

  case CONFIG_TYPE_ARRAY:
    return config_entry_type::array;
    break;

  case CONFIG_TYPE_STRING:
    return config_entry_type::string;
    break;

  case CONFIG_TYPE_FLOAT:
    return config_entry_type::floating_point;
    break;

  default:
    break;
  }

  return config_entry_type::none;
}

std::string xray::base::config_file::error() const noexcept {
  const auto err_type = config_error_type(&conf_);
  if (err_type == CONFIG_ERR_NONE)
    return "No error";

  if (err_type == CONFIG_ERR_FILE_IO)
    return "File IO error";

  return fmt::format("Config parse error {}:{}:{}", config_error_text(&conf_),
                     config_error_file(&conf_), config_error_line(&conf_));
}
