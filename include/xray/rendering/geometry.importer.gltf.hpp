//
// Copyright (c) Adrian Hodos
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

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>
#include <span>

#include <tl/optional.hpp>
#include <tl/expected.hpp>
#include <swl/variant.hpp>

#include "xray/xray.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/rendering/geometry.hpp"

namespace tinygltf {
class Model;
class Node;
}

namespace xray::rendering {

struct MaterialDefinition
{
    std::string name;
    uint32_t base_color_src;
    uint32_t metallic_src;
    uint32_t normal_src;
    float metallic_factor;
    float roughness_factor;
    xray::math::vec4f base_color_factor;
};

struct GeometryImportParseError
{
    std::string err_data;
    std::string warning;
};

using GeometryImportError = swl::variant<GeometryImportParseError, std::error_code>;

class LoadedGeometry
{
  public:
    LoadedGeometry();
    LoadedGeometry(LoadedGeometry&&) = default;
    LoadedGeometry(const LoadedGeometry&) = delete;
    LoadedGeometry& operator=(const LoadedGeometry&) = delete;
    ~LoadedGeometry();

    static tl::expected<LoadedGeometry, GeometryImportError> from_file(const std::filesystem::path& path);
    static tl::expected<LoadedGeometry, GeometryImportError> from_memory(const std::span<const uint8_t> bytes);

    xray::math::vec2ui32 extract_data(void* vertex_buffer, void* index_buffer, const xray::math::vec2ui32 offsets);
    xray::math::vec2ui32 compute_vertex_index_count() const;

  private:
    xray::math::vec2ui32 extract_single_node_data(void* vertex_buffer,
                                                  void* index_buffer,
                                                  const xray::math::vec2ui32 offsets,
                                                  const tinygltf::Node& node,
                                                  const tl::optional<uint32_t> parent);

    xray::math::vec2ui32 compute_node_vertex_index_count(const tinygltf::Node& node) const;

  public:
    std::vector<GeometryNode> nodes{};

  private:
    std::unique_ptr<tinygltf::Model> gltf;
};

}
