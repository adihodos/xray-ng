#pragma once

#include <string_view>
#include <filesystem>
#include <fmt/core.h>
#include <fmt/std.h>
#include <swl/variant.hpp>

#include <xray/base/memory.arena.hpp>
#include <xray/base/containers/arena.string.hpp>
#include <xray/base/containers/arena.vector.hpp>

namespace xray::rendering {

struct ShaderBuildOptions
{
    static constexpr const uint32_t Compile_EnabledOptimizations = 1 << 0;
    static constexpr const uint32_t Compile_GenerateDebugInfo = 1 << 1;
    static constexpr const uint32_t Compile_WarningsToErrors = 1 << 2;
    static constexpr const uint32_t Compile_SuppressWarnings = 1 << 2;
    static constexpr const uint32_t Compile_DumpShaderCode = 1 << 20;

    swl::variant<std::string_view, std::filesystem::path> code_or_file_path;
    std::string_view entry_point{};
    std::span<const std::pair<base::containers::string, base::containers::string>> defines{};
    uint32_t compile_options{ Compile_GenerateDebugInfo };
};

struct ShaderCodeBuilder
{
    ShaderCodeBuilder& add_code(std::string_view code)
    {
        _src_code.append(code);
        return *this;
    }

    ShaderCodeBuilder& add_file(const std::filesystem::path& p)
    {
        fmt::format_to(back_inserter(_src_code), "#include {}", p);
        return *this;
    }

    ShaderCodeBuilder& add_define(std::string_view name, std::string_view value)
    {
        _defines.emplace_back(base::containers::string{ name, *_temp }, base::containers::string{ value, *_temp });
        return *this;
    }

    ShaderCodeBuilder& set_entry_point(std::string_view entrypoint)
    {
        _entrypoint = entrypoint;
        return *this;
    }

    ShaderCodeBuilder& set_compile_flags(const uint32_t flags)
    {
        _compile_options = flags;
        return *this;
    }

    explicit ShaderCodeBuilder(base::MemoryArena* a)
        : _src_code{ *a }
        , _defines{ *a }
        , _entrypoint{ *a }
    {
    }

    xray::rendering::ShaderBuildOptions build_options() const
    {
        return ShaderBuildOptions{
            .code_or_file_path = std::string_view{ _src_code },
            .entry_point = _entrypoint,
            .defines = std::span{ _defines },
            .compile_options = _compile_options,
        };
    }

    xray::base::containers::string _src_code;
    xray::base::containers::vector<std::pair<xray::base::containers::string, xray::base::containers::string>> _defines;
    xray::base::containers::string _entrypoint;
    xray::base::MemoryArena* _temp;
    uint32_t _compile_options{ ShaderBuildOptions::Compile_GenerateDebugInfo };
};

}
