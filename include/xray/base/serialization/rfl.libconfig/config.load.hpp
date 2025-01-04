#pragma once

#include <string>
#include <filesystem>

#include <libconfig.h>
#include <rfl/Processors.hpp>
#include "xray/base/serialization/rfl.libconfig/config.reader.hpp"
#include "xray/base/serialization/rfl.libconfig/config.parser.hpp"

namespace rfl::libconfig {

using InputVarType = typename Reader::InputVarType;

/// Parses an object from a config_setting_t* var.
template<class T, class... Ps>
rfl::Result<T>
read(InputVarType _var)
{
    const auto r = Reader();
    using ProcessorsType = Processors<Ps...>;
    static_assert(!ProcessorsType::no_field_names_, "The NoFieldNames processor is not supported for libconfig");
    return Parser<T, ProcessorsType>::read(r, _var);
}

/// Parses an object from string using reflection.
template<class T, class... Ps>
Result<internal::wrap_in_rfl_array_t<T>>
read(const std::string& _libconfig_str)
{
    config_t cfg;
    const int result = config_read_string(&cfg, _libconfig_str.c_str());
    if (result != CONFIG_TRUE) {
        return rfl::Error(std::string{ config_error_text(&cfg) } + std::to_string(config_error_line(&cfg)));
    }

    return read<T, Ps...>(config_root_setting(&cfg));
}

/// Parses an object from a stringstream.
template<class T, class... Ps>
auto
read(std::istream& _stream)
{
    const auto toml_str = std::string(std::istreambuf_iterator<char>(_stream), std::istreambuf_iterator<char>());
    return read<T, Ps...>(toml_str);
}

/// Parse from file
template<typename T, typename... Ps>
rfl::Result<T>
read(const std::filesystem::path& path)
{
    config_t cfg;
    if (config_read_file(&cfg, path.string().c_str()) != CONFIG_TRUE) {
        return rfl::Error(std::string{ config_error_text(&cfg) } + std::to_string(config_error_line(&cfg)));
    }

    return read<T, Ps...>(config_root_setting(&cfg));
}

} // namespace rfl::toml
