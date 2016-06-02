#pragma once

#include "xray/xray.hpp"
#include "gpu_shader_traits.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/base/windows/com_ptr.hpp"
#include <cassert>
#include <cstdint>
#include <d3d10.h>
#include <string>
#include <vector>

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

namespace detail {

struct uniform_t {
  std::string name{};
  uint32_t    parent_idx{0};
  uint32_t    store_offset{0};
  uint32_t    byte_size{0};
};

struct uniform_block_t {
  std::string name{};
  uint32_t    bind_point{0};
  uint32_t    byte_size{0};
  uint32_t    store_offset{0};
  bool        dirty{false};
};

bool shader_collect_uniforms(ID3D10Blob*                   compiled_bytecode,
                             std::vector<uniform_t>&       uniforms,
                             std::vector<uniform_block_t>& uniform_blocks,
                             uint32_t&                     storage_bytes);

} // namespace detail

using scoped_blob_handle = xray::base::com_ptr<ID3D10Blob>;

scoped_blob_handle compile_shader_from_file(const shader_type type,
                                            const char*       file_name,
                                            const uint32_t compile_flags = 0);

scoped_blob_handle compile_shader(const shader_type type, const char* src_code,
                                  const size_t   code_len,
                                  const uint32_t compile_flags = 0);

class gpu_shader_common_core {
public:
  using blob_handle_type = ID3D10Blob*;

public:
  gpu_shader_common_core() noexcept = default;
  XRAY_DEFAULT_MOVE(gpu_shader_common_core);

  gpu_shader_common_core(const shader_type stype, ID3D10Device* dev,
                         const char* file_name, const uint32_t compile_flags);

  gpu_shader_common_core(const shader_type stype, ID3D10Device* dev,
                         const char* src_code, const size_t code_len,
                         const uint32_t compile_flags);

  ~gpu_shader_common_core();

public:
  bool valid() const noexcept { return valid_; }
  explicit operator bool() const noexcept { return valid(); }

  blob_handle_type bytecode() const noexcept {
    assert(valid());
    return base::raw_ptr(bytecode_);
  }

  void set_uniform(const char* name, const void* value,
                   const size_t bytes) noexcept;

  void set_uniform_block(const char* name, const void* value,
                         const size_t bytes) noexcept;

  void bind(ID3D10Device* dev,
            void (*stage_bind_fn)(ID3D10Device*, const uint32_t, const uint32_t,
                                  ID3D10Buffer* const*));

private:
  void collect_uniforms(ID3D10Device* dev);

private:
  scoped_blob_handle                       bytecode_;
  std::vector<detail::uniform_t>           uniforms_;
  std::vector<detail::uniform_block_t>     uniform_blocks_;
  std::vector<base::com_ptr<ID3D10Buffer>> cbuffers_;
  base::unique_pointer<uint8_t[]>          ublocks_storage_;
  bool                                     valid_{false};

private:
  XRAY_NO_COPY(gpu_shader_common_core);
};

} // namespace directx10
} // namespace rendering
} // namespace xray
