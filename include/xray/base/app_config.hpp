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
#include <platformstl/filesystem/path.hpp>
#include <string>

namespace xray {
namespace base {

/// \addtogroup __GroupXrayBase
/// @{

///
///  \brief  Simple path management.
class app_config {
public:
  explicit app_config(const char* cfg_path);

  const char* root_directory() const noexcept {
    return paths_.root_path.c_str();
  }

  std::string model_path(const char* name) const {
    platformstl::path_a path{paths_.model_path};
    path.push(name);

    return path.c_str();
  }

  std::string shader_path(const char* name) const {
    platformstl::path_a path{paths_.shader_path};
    path.push(name);

    return path.c_str();
  }

  std::string texture_path(const char* name) const {
    platformstl::path_a path{paths_.texture_path};
    path.push(name);

    return path.c_str();
  }

  std::string shader_config_path(const char* name) const {
    platformstl::path_a path{paths_.shader_cfg_path};
    path.push(name);

    return path.c_str();
  }

  std::string camera_config_path(const char* name) const {
    platformstl::path_a path{paths_.camera_cfg_path};
    path.push(name);

    return path.c_str();
  }

  std::string objects_config_path(const char* name) const {
    platformstl::path_a path{paths_.objects_cfg_path};
    path.push(name);

    return path.c_str();
  }

  const char* engine_config_path() const {
    return paths_.engine_ini_file.c_str();
  }

  static app_config* instance() noexcept { return _unique_instance; }

private:
  static app_config* _unique_instance;

  struct paths_data_t {
    platformstl::path_a root_path;
    platformstl::path_a shader_path;
    platformstl::path_a model_path;
    platformstl::path_a texture_path;
    platformstl::path_a shader_cfg_path;
    platformstl::path_a camera_cfg_path;
    platformstl::path_a objects_cfg_path;
    platformstl::path_a engine_ini_file;
  } paths_;

private:
  app_config() = default;

  XRAY_NO_COPY(app_config);
  XRAY_NO_MOVE(app_config);
};

/// @}

} // namespace base
} // namespace xray
