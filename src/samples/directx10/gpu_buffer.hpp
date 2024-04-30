#pragma once

#include "xray/base/windows/com_ptr.hpp"
#include "xray/xray.hpp"
#include <d3d10.h>

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

class renderer;

class gpu_buffer
{
  public:
    using handle_type = ID3D10Buffer*;

  public:
    gpu_buffer() = default;
    gpu_buffer(gpu_buffer&&) = default;
    gpu_buffer& operator=(gpu_buffer&&) = default;

    template<typename T, size_t N>
    gpu_buffer(renderer* r_ptr,
               const uint32_t bind_flags,
               const T (&arr_ref)[N],
               const bool gpu_write = false,
               const bool cpu_acc = false);

    gpu_buffer(renderer* r_ptr,
               const size_t e_size,
               const size_t e_count,
               const uint32_t bind_flags,
               const void* initial_data = nullptr,
               const bool gpu_write = false,
               const bool cpu_acc = false);

    ~gpu_buffer();

    bool valid() const noexcept { return buff_handle_ != nullptr; }
    explicit operator bool() const noexcept { return valid(); }

    handle_type handle() const noexcept { return base::raw_ptr(buff_handle_); }

    uint32_t bytes() const noexcept { return e_count_ * e_size_; }

    uint32_t element_size() const noexcept { return e_size_; }

    uint32_t count() const { return e_count_; }

  private:
    xray::base::com_ptr<ID3D10Buffer> buff_handle_{};
    uint32_t e_count_{};
    uint32_t e_size_{};

  private:
    XRAY_NO_COPY(gpu_buffer);
};

template<typename T, size_t N>
gpu_buffer::gpu_buffer(renderer* r_ptr,
                       const uint32_t bind_flags,
                       const T (&arr_ref)[N],
                       const bool gpu_write,
                       const bool cpu_acc)
    : gpu_buffer{ r_ptr, sizeof(arr_ref[0]), N, bind_flags, arr_ref, gpu_write, cpu_acc }
{
}

inline gpu_buffer::handle_type
raw_handle(const gpu_buffer& buff) noexcept
{
    return buff.handle();
}

} // namespace directx10
} // namespace rendering
} // namespace xray
