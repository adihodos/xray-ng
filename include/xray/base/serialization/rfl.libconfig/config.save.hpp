#pragma once

#include <string>
#include <filesystem>

#include <libconfig.h>
#include <rfl/Result.hpp>
#include <rfl/Processors.hpp>
#include <rfl/parsing/Parent.hpp>
#include "xray/base/serialization/rfl.libconfig/config.parser.hpp"

namespace rfl {
namespace libconfig {

template<class... Ps>
Result<Nothing>
save(const std::string& _fname, const auto& _obj)
{
    using ValType = std::remove_cvref_t<decltype(_obj)>;
    using ParentType = parsing::Parent<Writer>;

    config_t cfg;
    config_init(&cfg);

    auto w = Writer{ &cfg };
    using ProcessorsType = Processors<Ps...>;
    Parser<ValType, ProcessorsType>::write(w, _obj, typename ParentType::Root{});

    config_write_file(&cfg, _fname.c_str());
    return Nothing{};
}

template<class... Ps>
Result<Nothing>
save(const std::filesystem::path& _fname, const auto& _obj)
{
    return save(_fname.generic_string(), _obj);
}

} // namespace toml
} // namespace rfl
