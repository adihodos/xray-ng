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
#include "xray/base/enum_cast.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_handle.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
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

enum class graphics_pipeline_stage : uint8_t {
  first,
  vertex = first,
  tess_control,
  tess_eval,
  geometry,
  fragment,
  compute,
  last
};

enum class shader_source_type { file, code, binary };

template <graphics_pipeline_stage ps>
struct xray_to_opengl;

template <>
struct xray_to_opengl<graphics_pipeline_stage::vertex> {
  static constexpr GLenum     shader_type       = gl::VERTEX_SHADER;
  static constexpr GLbitfield shader_bit        = gl::VERTEX_SHADER_BIT;
  static constexpr GLenum sub_uniform_interface = gl::VERTEX_SUBROUTINE_UNIFORM;
  static constexpr GLenum sub_interface         = gl::VERTEX_SUBROUTINE;
};

template <>
struct xray_to_opengl<graphics_pipeline_stage::tess_control> {
  static constexpr GLenum     shader_type = gl::TESS_CONTROL_SHADER;
  static constexpr GLbitfield shader_bit  = gl::TESS_CONTROL_SHADER_BIT;
  static constexpr GLenum     sub_uniform_interface =
      gl::TESS_CONTROL_SUBROUTINE_UNIFORM;
  static constexpr GLenum sub_interface = gl::TESS_CONTROL_SUBROUTINE;
};

template <>
struct xray_to_opengl<graphics_pipeline_stage::tess_eval> {
  static constexpr GLenum     shader_type = gl::TESS_EVALUATION_SHADER;
  static constexpr GLbitfield shader_bit  = gl::TESS_EVALUATION_SHADER_BIT;
  static constexpr GLenum     sub_uniform_interface =
      gl::TESS_EVALUATION_SUBROUTINE_UNIFORM;
  static constexpr GLenum sub_interface = gl::TESS_EVALUATION_SUBROUTINE;
};

template <>
struct xray_to_opengl<graphics_pipeline_stage::geometry> {
  static constexpr GLenum     shader_type = gl::GEOMETRY_SHADER;
  static constexpr GLbitfield shader_bit  = gl::GEOMETRY_SHADER_BIT;
  static constexpr GLenum     sub_uniform_interface =
      gl::GEOMETRY_SUBROUTINE_UNIFORM;
  static constexpr GLenum sub_interface = gl::GEOMETRY_SUBROUTINE;
};

template <>
struct xray_to_opengl<graphics_pipeline_stage::fragment> {
  static constexpr GLenum     shader_type = gl::FRAGMENT_SHADER;
  static constexpr GLbitfield shader_bit  = gl::FRAGMENT_SHADER_BIT;
  static constexpr GLenum     sub_uniform_interface =
      gl::FRAGMENT_SUBROUTINE_UNIFORM;
  static constexpr GLenum sub_interface = gl::FRAGMENT_SUBROUTINE;
};

template <>
struct xray_to_opengl<graphics_pipeline_stage::compute> {
  static constexpr GLenum     shader_type = gl::COMPUTE_SHADER;
  static constexpr GLbitfield shader_bit  = gl::COMPUTE_SHADER_BIT;
  static constexpr GLenum     sub_uniform_interface =
      gl::COMPUTE_SUBROUTINE_UNIFORM;
  static constexpr GLenum sub_interface = gl::COMPUTE_SUBROUTINE;
};

struct shader_source_file {
  const char* name;
};

struct shader_source_string {
  const char* cstr{nullptr};
  size_t      len{0};

  shader_source_string() noexcept {}

  shader_source_string(const char* str, const size_t slen) noexcept
      : cstr{str}, len{slen} {}

  explicit shader_source_string(const char* str) noexcept
      : shader_source_string{str, strlen(str)} {}
};

struct shader_source_descriptor {
  shader_source_type src_type;

  union {
    shader_source_file   s_file;
    shader_source_string s_str;
  };

  shader_source_descriptor() {}

  explicit shader_source_descriptor(const shader_source_file& sfile) noexcept;

  explicit shader_source_descriptor(
      const shader_source_string src_str) noexcept;
};

enum pipeline_stage : uint8_t { vertex, geometry, fragment, last };

namespace detail {

/// \brief  Description of a uniform block in a shader program.
struct uniform_block_t {
  uniform_block_t() = default;

  uniform_block_t(const char* name_, const uint32_t offset,
                  const uint32_t size_, const uint32_t index_,
                  const uint32_t bindpoint_, const bool dirty_ = false)
      : name{name_}
      , store_offset{offset}
      , size{size_}
      , index{index_}
      , bindpoint{bindpoint_}
      , dirty{dirty_} {}

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
  uniform_t() = default;

  uniform_t(const char* name_, const uint32_t bytes,
            const uint32_t store_offset, const int32_t parent_idx,
            const uint32_t type_, const uint32_t dimension,
            const uint32_t location_, const uint32_t stride_)
      : name{name_}
      , byte_size{bytes}
      , block_store_offset{store_offset}
      , parent_block_idx{parent_idx}
      , type{type_}
      , array_dim{dimension}
      , location{location_}
      , stride{stride_} {}

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

///   \brief Info about a subroutine defined in a shader.
struct shader_subroutine {
  shader_subroutine() = default;

  shader_subroutine(const char* name, const graphics_pipeline_stage stage,
                    const uint8_t index)
      : ss_name{name}, ss_stage{stage}, ss_index{index} {}

  std::string             ss_name{};
  graphics_pipeline_stage ss_stage{graphics_pipeline_stage::last};
  uint8_t                 ss_index{0xFF};
};

///   \brief Info about an active subroutine uniform of a shader.
struct shader_subroutine_uniform {
  shader_subroutine_uniform() = default;

  shader_subroutine_uniform(const char*                   name,
                            const graphics_pipeline_stage stage,
                            const uint8_t                 location,
                            const uint8_t                 assigned_index)
      : ssu_name{name}
      , ssu_stage{stage}
      , ssu_location{location}
      , ssu_assigned_subroutine_idx{assigned_index} {}

  ///< Uniform name.
  std::string ssu_name{};

  ///< Stage (vertex, geometry, fragment, etc).
  graphics_pipeline_stage ssu_stage{graphics_pipeline_stage::last};

  ///< Assigned location by the shader compiler.
  uint8_t ssu_location{0xFF};

  ///< Index of the assigned subroutine.
  uint8_t ssu_assigned_subroutine_idx{0xFF};
};

///   \brief  Info about subroutine uniforms for a particular stage.
struct pipeline_stage_subroutine_uniform_data {
  pipeline_stage_subroutine_uniform_data() = default;

  pipeline_stage_subroutine_uniform_data(const uint8_t store_offset,
                                         const uint8_t count,
                                         const uint8_t max_locations)
      : datastore_offset{store_offset}
      , uniforms_count{count}
      , max_active_locations{max_locations} {}

  static constexpr auto invalid_offset = uint8_t{0xFF};

  ///< Offset in array with data for all stages. Will be 0xFFFFFFFF
  uint8_t datastore_offset{invalid_offset};

  ///< Number of active subroutine uniforms for this stage.
  uint8_t uniforms_count{0};

  ///< Maximum number of uniform locations assigned by the shader compiler.
  uint8_t max_active_locations{0};
};

struct gpu_program_reflect_data {
  size_t                                  prg_datastore_size;
  std::vector<uniform_block_t>*           prg_ublocks;
  std::vector<uniform_t>*                 prg_uniforms;
  std::vector<shader_subroutine_uniform>* prg_subroutine_uniforms;
  std::vector<shader_subroutine>*         prg_subroutines;
};

struct gpu_program_helpers {
  static bool reflect(const GLuint program,
                      const GLenum api_subroutine_uniform_interface_name,
                      const GLenum api_subroutine_interface_name,
                      gpu_program_reflect_data* refdata);

  static bool collect_uniform_blocks(const GLuint                  program,
                                     std::vector<uniform_block_t>* blks,
                                     size_t* datastore_bytes);

  static bool collect_uniforms(const GLuint                        program,
                               const std::vector<uniform_block_t>& ublocks,
                               std::vector<uniform_t>*             uniforms);

  static bool collect_subroutines_and_uniforms(
      const GLuint program, const GLenum api_subroutine_uniform_interface_name,
      const GLenum                            api_subroutine_interface_name,
      std::vector<shader_subroutine_uniform>* subs_uniforms,
      std::vector<shader_subroutine>*         subs);

  //  static xray::rendering::scoped_shader_handle
  //  create_shader(const xray::rendering::shader_source_descriptor& sd);

  static scoped_shader_handle
  create_shader_from_string(const GLenum                type,
                            const shader_source_string& sstr);

  static void set_uniform(const GLuint program_id, const GLint uniform_location,
                          const uint32_t uniform_type, const void* uniform_data,
                          const size_t item_count) noexcept;
};

class gpu_program_base {
public:
  XRAY_DEFAULT_MOVE(gpu_program_base);

  using handle_type = scoped_program_handle::handle_type;

  handle_type handle() const noexcept { return base::raw_handle(_handle); }
  bool        valid() const noexcept { return _valid; }

  template <typename block_data_type>
  gpu_program_base& set_uniform_block(const char*            block_name,
                                      const block_data_type& data) {
    return set_uniform_block(block_name, &data, sizeof(data));
  }

  gpu_program_base& set_uniform_block(const char*  block_name,
                                      const void*  block_data,
                                      const size_t byte_count);

  template <typename uniform_data_type, size_t size>
  gpu_program_base& set_uniform(const char* uniform_name,
                                const uniform_data_type (&arr_ref)[size]) {
    return set_uniform(uniform_name, &arr_ref[0], size);
  }

  template <typename uniform_data_type>
  gpu_program_base& set_uniform(const char*              uniform_name,
                                const uniform_data_type& data) {
    return set_uniform(uniform_name, &data, 1);
  }

  template <typename uniform_data_type>
  gpu_program_base& set_uniform(const char*              uniform_name,
                                const uniform_data_type* data,
                                const size_t             count);

  gpu_program_base&
  set_subroutine_uniform(const char* uniform_name,
                         const char* subroutine_name) noexcept;

  /// \brief    Sends the values of the uniforms to the GPU.
  void flush_uniforms();

  void swap(gpu_program_base& rhs) noexcept;

protected:
  gpu_program_base() = default;
  explicit gpu_program_base(scoped_program_handle handle, const GLenum stage,
                            const GLenum api_subroutine_uniform_interface_name,
                            const GLenum api_subroutine_interface_name);

  ~gpu_program_base();

protected:
  /// \brief Handle to an OpenGL program object.
  scoped_program_handle _handle{};

  /// \brief List of active uniform blocks in the shader.
  std::vector<detail::uniform_block_t> uniform_blocks_;

  /// \brief List of active uniforms in the shader.
  std::vector<detail::uniform_t> uniforms_;

  /// \brief Storage for uniform block data
  base::unique_pointer<uint8_t[]> ublocks_datastore_;

  ///   \brief  List of all active subroutine uniforms in the program. Grouped
  ///   by index.
  std::vector<detail::shader_subroutine_uniform> subroutine_uniforms_;

  ///   \brief  List with data for all subroutines active in the program.
  std::vector<detail::shader_subroutine> subroutines_;

  /// \brief True if compiled, linked and initialized successfully.
  bool   _valid{false};
  GLenum _stage{gl::INVALID_ENUM};

private:
  XRAY_NO_COPY(gpu_program_base);
};

template <typename uniform_data_type>
gpu_program_base& gpu_program_base::set_uniform(const char* uniform_name,
                                                const uniform_data_type* data,
                                                const size_t count) {
  assert(_valid);

  using namespace std;

  auto u_iter =
      find_if(begin(uniforms_), end(uniforms_),
              [uniform_name](const auto& u) { return u.name == uniform_name; });

  if (u_iter == end(uniforms_)) {
    XR_LOG_ERR("Uniform {} does not exist", uniform_name);
    return *this;
  }

  //
  // Standalone uniform.
  if (u_iter->parent_block_idx == -1) {
    detail::gpu_program_helpers::set_uniform(
        base::raw_handle(_handle), u_iter->location, u_iter->type, data, count);
    return *this;
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

  return *this;
}

} // namespace detail

template <graphics_pipeline_stage stage>
class gpu_program_t : public detail::gpu_program_base {
  /// \name Types and constants
  /// @{
public:
  using base_class = detail::gpu_program_base;
  using gpu_program_base::handle_type;

  static constexpr graphics_pipeline_stage program_stage = stage;

  static constexpr GLenum api_shader_type = xray_to_opengl<stage>::shader_type;

  static constexpr GLbitfield api_shader_bit =
      xray_to_opengl<stage>::shader_bit;

  static constexpr GLbitfield api_subroutine_uniform_interface_name =
      xray_to_opengl<stage>::sub_uniform_interface;

  static constexpr GLbitfield api_subroutine_interface_name =
      xray_to_opengl<stage>::sub_interface;

  /// @}

  /// \name Constructors (copy/move) + assign/move operators
  /// @{

public:
  gpu_program_t() = default;
  //  gpu_program_t(gpu_program_t&&) = default;
  //  gpu_program_t& operator=(gpu_program_t&&) = default;
  explicit gpu_program_t(scoped_program_handle handle);
  XRAY_DEFAULT_MOVE(gpu_program_t);

  /// @}

  explicit operator bool() const noexcept { return valid(); }

private:
  XRAY_NO_COPY(gpu_program_t);
};

template <graphics_pipeline_stage stage>
gpu_program_t<stage>::gpu_program_t(scoped_program_handle handle)
    : detail::gpu_program_base{std::move(handle),
                               xray_to_opengl<stage>::shader_type,
                               api_subroutine_uniform_interface_name,
                               api_subroutine_interface_name} {}

using vertex_program    = gpu_program_t<graphics_pipeline_stage::vertex>;
using tess_eval_program = gpu_program_t<graphics_pipeline_stage::tess_eval>;
using tess_control_program =
    gpu_program_t<graphics_pipeline_stage::tess_control>;
using geometry_program = gpu_program_t<graphics_pipeline_stage::geometry>;
using fragment_program = gpu_program_t<graphics_pipeline_stage::fragment>;
using compute_program  = gpu_program_t<graphics_pipeline_stage::compute>;

GLuint make_gpu_program(const GLuint* shaders_to_attach,
                        const size_t  shaders_count) noexcept;

class gpu_program {
public:
  using handle_type = gpu_program_handle::handle_type;

  gpu_program() noexcept = default;
  explicit gpu_program(scoped_program_handle linked_prg) noexcept;
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

  void set_subroutine_uniform(const graphics_pipeline_stage stage,
                              const char*                   uniform_name,
                              const char* subroutine_name) noexcept;
  /// @}

private:
  bool reflect();

  /// \brief  Description of a uniform block in a shader program.
  struct uniform_block_t {
    uniform_block_t() = default;

    uniform_block_t(const char* bname, const uint32_t offset,
                    const uint32_t bsize, const uint32_t idx,
                    const uint32_t bindpt, const bool dirty_ = false)
        : name{bname}
        , store_offset{offset}
        , size{bsize}
        , index{idx}
        , bindpoint{bindpt}
        , dirty{dirty_} {}

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
    uniform_t() = default;

    uniform_t(const char* name_, const uint32_t size, const uint32_t offset,
              const int32_t parent_idx, const uint32_t type_,
              const uint32_t dimension, const uint32_t loc,
              const uint32_t stride_)
        : name{name_}
        , byte_size{size}
        , block_store_offset{offset}
        , parent_block_idx{parent_idx}
        , type{type_}
        , array_dim{dimension}
        , location{loc}
        , stride{stride_} {}

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
    shader_subroutine() = default;
    shader_subroutine(const char* name_, const graphics_pipeline_stage stage,
                      const uint8_t ss_idx)
        : ss_name{name_}, ss_stage{stage}, ss_index{ss_idx} {}

    std::string             ss_name{};
    graphics_pipeline_stage ss_stage{graphics_pipeline_stage::last};
    uint8_t                 ss_index{0xFF};
  };

  ///   \brief Info about an active subroutine uniform of a shader.
  struct shader_subroutine_uniform {
    shader_subroutine_uniform() = default;

    shader_subroutine_uniform(const char* name, const pipeline_stage stage,
                              const uint8_t location,
                              const uint8_t subroutine_idx)
        : ssu_name{name}
        , ssu_stage{stage}
        , ssu_location{location}
        , ssu_assigned_subroutine_idx{subroutine_idx} {}

    ///< Uniform name.
    std::string ssu_name{};

    ///< Stage (vertex, geometry, fragment, etc).
    pipeline_stage ssu_stage{pipeline_stage::last};

    ///< Assigned location by the shader compiler.
    uint8_t ssu_location{0xFF};

    ///< Index of the assigned subroutine.
    uint8_t ssu_assigned_subroutine_idx{0xFF};
  };

  ///   \brief  Info about subroutine uniforms for a particular stage.
  struct pipeline_stage_subroutine_uniform_data {
    pipeline_stage_subroutine_uniform_data() = default;
    pipeline_stage_subroutine_uniform_data(const uint8_t store_offset,
                                           const uint8_t count,
                                           const uint8_t max_locs)
        : datastore_offset{store_offset}
        , uniforms_count{count}
        , max_active_locations{max_locs} {}

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
    XR_LOG_ERR("Uniform {} does not exist", uniform_name);
    return;
  }

  //
  // Standalone uniform.
  if (u_iter->parent_block_idx == -1) {
    detail::gpu_program_helpers::set_uniform(handle(), u_iter->location,
                                             u_iter->type, data, count);
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

class gpu_program_builder {
public:
  gpu_program_builder() noexcept {}

  gpu_program_builder& add_string(const char* str) {
    return add_string(shader_source_string{str});
  }

  gpu_program_builder& add_string(const shader_source_string ssrc) {
    push_descriptor(shader_source_descriptor{ssrc});
    return *this;
  }

  gpu_program_builder& add_file(const char* sfile) {
    add_file(shader_source_file{sfile});
    return *this;
  }

  gpu_program_builder& add_file(const shader_source_file sfile) {
    push_descriptor(shader_source_descriptor{sfile});
    return *this;
  }

  gpu_program_builder& hint_binary() noexcept {
    _binary = true;
    return *this;
  }

  template <graphics_pipeline_stage stage>
  gpu_program_t<stage>              build() const {
    return gpu_program_t<stage>{
        build_program(xray_to_opengl<stage>::shader_type)};
  }

  explicit operator bool() const noexcept { return _sources_count != 0; }

private:
  static constexpr uint32_t MAX_SLOTS = 16u;

  void push_descriptor(const shader_source_descriptor& sds) noexcept {
    assert(_sources_count < MAX_SLOTS);
    _source_list[_sources_count++] = sds;
  }

  scoped_program_handle build_program(const GLenum stg) const noexcept;

  shader_source_descriptor _source_list[MAX_SLOTS];
  uint8_t                  _sources_count{0};
  bool                     _binary{false};
};

struct gpu_program_pipeline_handle {
  using handle_type = GLuint;

  static bool is_null(const handle_type handle) noexcept { return handle == 0; }

  static void destroy(const handle_type handle) noexcept {
    if (!is_null(handle))
      gl::DeleteProgramPipelines(1, &handle);
  }

  static handle_type null() noexcept { return 0; }
};

using scoped_program_pipeline_handle =
    xray::base::unique_handle<gpu_program_pipeline_handle>;

class gpu_program_pipeline_setup_builder {
public:
  explicit gpu_program_pipeline_setup_builder(const GLuint handle) noexcept;

  gpu_program_pipeline_setup_builder&
  add_vertex_program(vertex_program& vert_prg);

  gpu_program_pipeline_setup_builder&
  add_geometry_program(geometry_program& geom_prg);

  gpu_program_pipeline_setup_builder&
  add_fragment_program(fragment_program& frag_prg);

  gpu_program_pipeline_setup_builder&
  add_tess_control_program(tess_control_program& tess_ctrl_prg);

  gpu_program_pipeline_setup_builder&
  add_tess_eval_program(tess_eval_program& tess_eval_prg);

  void install();

private:
  GLuint _handle;
  detail::gpu_program_base*
      _programs_by_stage[xray::base::enum_helper::to_underlying_type(
          graphics_pipeline_stage::last)];

  XRAY_NO_COPY(gpu_program_pipeline_setup_builder);
};

} // namespace rendering
} // namespace xray
