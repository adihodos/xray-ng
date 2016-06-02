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
#include "xray/base/array_dimension.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_handle.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/xray_types.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <opengl/opengl.hpp>
#include <string>
#include <vector>

namespace xray {
namespace rendering {

enum pipeline_stage : uint8_t { vertex, geometry, fragment, last };

struct gpu_program_handle {
  using handle_type = GLuint;

  static bool is_null(const handle_type handle) noexcept { return handle == 0; }

  static void destroy(const handle_type handle) noexcept {
    if (!is_null(handle))
      gl::DeleteProgram(handle);
  }

  static handle_type null() noexcept { return 0; }
};

using scoped_program_handle = xray::base::unique_handle<gpu_program_handle>;

GLuint make_gpu_program(const GLuint* shaders_to_attach,
                        const size_t  shaders_count) noexcept;

void set_uniform_impl(const GLuint program_id, const GLint uniform_location,
                      const uint32_t uniform_type, const void* uniform_data,
                      const size_t item_count) noexcept;

class gpu_program {
public:
  using handle_type = gpu_program_handle::handle_type;

  gpu_program() noexcept     = default;
  gpu_program(gpu_program&&) = default;
  gpu_program& operator=(gpu_program&&) = default;

  gpu_program(const GLuint* shaders_to_attach,
              const size_t  shaders_count) noexcept;

  template <size_t shaders_cnt__>
  explicit gpu_program(const GLuint (&arr_ref)[shaders_cnt__]) noexcept
      : gpu_program{&arr_ref[0], XR_COUNTOF__(arr_ref)} {}

  bool valid() const noexcept { return valid_; }

  explicit operator bool() const noexcept { return valid(); }

  handle_type handle() const noexcept { return base::raw_handle(prog_handle_); }

  void bind_to_pipeline();

  /// \name Uniform block functions
  /// @{
public:
  template <typename block_data_type>
  void set_uniform_block(const char* block_name, const block_data_type& data) {
    set_uniform_block(block_name, &data, sizeof(data));
  }

  void set_uniform_block(const char* block_name, const void* block_data,
                         const size_t byte_count);
  /// @}

  /// \name Uniform functions
  /// @{
public:
  template <typename uniform_data_type, size_t size>
  void set_uniform(const char* uniform_name,
                   const uniform_data_type (&arr_ref)[size]) {
    set_uniform(uniform_name, &arr_ref[0], size);
  }

  template <typename uniform_data_type>
  void set_uniform(const char* uniform_name, const uniform_data_type& data) {
    set_uniform(uniform_name, &data, 1);
  }

  template <typename uniform_data_type>
  void set_uniform(const char* uniform_name, const uniform_data_type* data,
                   const size_t count);

  void set_subroutine_uniform(const pipeline_stage stage,
                              const char*          uniform_name,
                              const char*          subroutine_name) noexcept;
  /// @}

private:
  bool reflect();

  /// \brief  Description of a uniform block in a shader program.
  struct uniform_block_t {
    ///< Name of the uniform block as defined in the shader code.
    std::string name{};

    ///< Offset in memory where data for this block is stored.
    uint32_t store_offset{0};

    ///< Size of the block, in bytes.
    uint32_t size{0};

    ///< Index of the block, as assigned by OpenGL.
    uint32_t index{0};

    ///< Binding point.
    uint32_t bindpoint{0};

    ///< True if data is out of sync with the GPU buffer.
    bool dirty{false};

    ///< Handle to the GPU buffer.
    scoped_buffer gl_buff{};
  };

  /// \brief      Stores data for a uniform in a linked shader program.
  struct uniform_t {
    ///< Name of the uniform, as defined in the shader code.
    std::string name{};

    ///< Size of the uniform in bytes.
    uint32_t byte_size{0};

    ///< Offset in uniform block memory. Will be 0 for uniforms that are not
    ///< part of a uniform block.
    uint32_t block_store_offset{0};

    ///< Index of the parent uniform block, in the list of uniform blocks. Will
    ///< be -1 for uniforms that are not part of a block.
    int32_t parent_block_idx{-1};

    ///< Type (float3, mat4, etc).
    uint32_t type{0};

    ///< Number of components.
    uint32_t array_dim{1};

    ///< Location in the linked program. Will be 0xFFFFFFFF for uniforms that
    ///< are part of a uniform block.
    uint32_t location{0};

    uint32_t stride{0};
  };

  bool collect_uniform_blocks();

  bool collect_uniforms();

  ///   \name Subroutine uniforms
  ///   @{

private:
  ///   \brief Info about a subroutine defined in a shader.
  struct shader_subroutine {
    std::string    ss_name;
    pipeline_stage ss_stage{pipeline_stage::last};
    uint8_t        ss_index{0xFF};
  };

  ///   \brief Info about an active subroutine uniform of a shader.
  struct shader_subroutine_uniform {
    ///< Uniform name.
    std::string ssu_name;

    ///< Stage (vertex, geometry, fragment, etc).
    pipeline_stage ssu_stage{pipeline_stage::last};

    ///< Assigned location by the shader compiler.
    uint8_t ssu_location{0xFF};

    ///< Index of the assigned subroutine.
    uint8_t ssu_assigned_subroutine_idx{0xFF};
  };

  ///   \brief  Info about subroutine uniforms for a particular stage.
  struct pipeline_stage_subroutine_uniform_data {
    static constexpr auto invalid_offset = uint8_t{0xFF};

    ///< Offset in array with data for all stages. Will be 0xFFFFFFFF
    uint8_t datastore_offset{invalid_offset};

    ///< Number of active subroutine uniforms for this stage.
    uint8_t uniforms_count{0};

    ///< Maximum number of uniform locations assigned by the shader compiler.
    uint8_t max_active_locations{0};
  };

  bool collect_subroutines_and_uniforms();

  ///   @}

private:
  /// \brief Handle to the linked shader program.
  scoped_program_handle prog_handle_;

  /// \brief List of active uniform blocks in the shader.
  std::vector<uniform_block_t> uniform_blocks_;

  /// \brief List of active uniforms in the shader.
  std::vector<uniform_t> uniforms_;

  /// \brief Storage for uniform block data
  base::unique_pointer<uint8_t[]> ublocks_datastore_;

  ///   \brief  List of all active subroutine uniforms in the program. Grouped
  ///   by
  ///   stage and by index.
  std::vector<shader_subroutine_uniform> subroutine_uniforms_;

  ///   \brief  List with data for all subroutines active in the program.
  std::vector<shader_subroutine> subroutines_;

  ///   \brief  Subroutine info for each stage.
  pipeline_stage_subroutine_uniform_data
      stage_subroutine_ufs_[pipeline_stage::last];

  /// \brief True if compiled, linked and initialized successfully.
  bool valid_{false};

private:
  XRAY_NO_COPY(gpu_program);
};

template <typename uniform_data_type>
void gpu_program::set_uniform(const char*              uniform_name,
                              const uniform_data_type* data,
                              const size_t             count) {
  assert(valid());

  using namespace std;

  auto u_iter =
      find_if(begin(uniforms_), end(uniforms_),
              [uniform_name](const auto& u) { return u.name == uniform_name; });

  if (u_iter == end(uniforms_)) {
    // OUTPUT_DBG_MSG("Uniform %s does not exist!", uniform_name);
    XR_LOG_ERR("Uniform {} does not exist", uniform_name);
    return;
  }

  //
  // Standalone uniform.
  if (u_iter->parent_block_idx == -1) {
    set_uniform_impl(handle(), u_iter->location, u_iter->type, data, count);
    return;
  }

  using namespace xray::base;

  //
  // Uniform is part of a block.
  auto&      par_blk       = uniform_blocks_[u_iter->parent_block_idx];
  const auto mem_offset    = par_blk.store_offset + u_iter->block_store_offset;
  const auto bytes_to_copy = count * sizeof(*data);

  assert(bytes_to_copy == u_iter->byte_size);
  memcpy(raw_ptr(ublocks_datastore_) + mem_offset, data, bytes_to_copy);
  par_blk.dirty = true;
}

} // namespace rendering
} // namespace xray
