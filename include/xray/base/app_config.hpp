//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "xray/xray.hpp"

#include <cassert>
#include <filesystem>
#include <string_view>

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

///
///  \brief  Simple path management.
class ConfigSystem
{
  public:
    using path_type = std::filesystem::path;

    explicit ConfigSystem(const char* cfg_path);

    const path_type& root_directory() const noexcept { return paths_.root_path; }

    path_type model_path(const char* name) const { return paths_.model_path / name; }

    path_type shader_path(const char* name) const { return paths_.shader_path / name; }

    path_type texture_path(const char* name) const { return paths_.texture_path / name; }

    path_type shader_config_path(const char* name) const { return paths_.shader_cfg_path / name; }

    path_type camera_config_path(const char* name) const { return paths_.camera_cfg_path / name; }

    path_type objects_config_path(const char* name) const { return paths_.objects_cfg_path / name; }

    const path_type font_path(const char* name) const noexcept { return paths_.fonts_path / name; }

    const path_type& engine_config_path() const noexcept { return paths_.engine_ini_file; }

    const path_type& font_root() const noexcept { return paths_.fonts_path; }
    const path_type& model_root() const noexcept { return paths_.model_path; }
    const path_type& shader_root() const noexcept { return paths_.shader_path; }

    static ConfigSystem* instance() noexcept { return _unique_instance; }

  private:
    static ConfigSystem* _unique_instance;

    struct paths_data_t
    {
        std::filesystem::path root_path;
        std::filesystem::path shader_path;
        std::filesystem::path model_path;
        std::filesystem::path texture_path;
        std::filesystem::path shader_cfg_path;
        std::filesystem::path camera_cfg_path;
        std::filesystem::path objects_cfg_path;
        std::filesystem::path fonts_path;
        std::filesystem::path engine_ini_file;
    } paths_;

  private:
    ConfigSystem() = default;
    XRAY_NO_COPY(ConfigSystem);
};

/// @}

} // namespace base
} // namespace xray
