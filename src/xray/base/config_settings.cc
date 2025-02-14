#include "xray/base/config_settings.hpp"
#include <libconfig.h>

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
