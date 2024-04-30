#pragma once

#include "gpu_shader_common_core.hpp"
#include "gpu_shader_traits.hpp"
#include "renderer_dx10.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/base/windows/com_ptr.hpp"
#include "xray/xray.hpp"
#include <cassert>
#include <cstdint>
#include <d3d10.h>

namespace xray {
namespace rendering {

#if defined(XRAY_RENDERER_DIRECTX)
inline namespace directx10 {
#else
namespace directx10 {
#endif

template<shader_type stype>
class gpu_shader
{
  public:
    using traits = shader_traits<stype>;
    using blob_handle_type = ID3D10Blob*;
    using shader_handle_type = typename traits::handle_type;

  public:
    gpu_shader() noexcept = default;

    gpu_shader(gpu_shader<stype>&&) = default;
    gpu_shader<stype>& operator=(gpu_shader<stype>&&) = default;

    gpu_shader(renderer* r_ptr, const char* file_name, const uint32_t compile_flags = 0);

    gpu_shader(renderer* r_ptr, const char* src_code, const size_t code_len, const uint32_t compile_flags);

    ~gpu_shader();

  public:
    bool valid() const noexcept { return core_ && handle_; }
    explicit operator bool() const noexcept { return valid(); }

    blob_handle_type bytecode() const noexcept
    {
        assert(valid());
        return core_.bytecode();
    }

    shader_handle_type handle() const noexcept
    {
        assert(valid());
        return base::raw_ptr(handle_);
    }

    template<typename T>
    void set_uniform(const char* name, const T& value) noexcept;

    void set_uniform(const char* name, const void* value, const size_t bytes) noexcept
    {
        assert(valid());
        core_.set_uniform(name, value, bytes);
    }

    template<typename T>
    void set_uniform_block(const char* name, const T& value) noexcept;

    void set_uniform_block(const char* name, const void* value, const size_t bytes) noexcept
    {
        assert(valid());
        core_.set_uniform_block(name, value, bytes);
    }

    void bind(renderer* r_ptr);

  private:
    gpu_shader_common_core core_{};
    typename traits::scoped_shader_type handle_{};

  private:
    XRAY_NO_COPY(gpu_shader<stype>);
};

template<shader_type stype>
xray::rendering::directx10::gpu_shader<stype>::gpu_shader(renderer* r_ptr,
                                                          const char* file_name,
                                                          const uint32_t compile_flags)
    : core_{ stype, r_ptr->dev_ctx(), file_name, compile_flags }
{

    if (core_)
        handle_ = traits::create(r_ptr->dev_ctx(), core_.bytecode());
}

template<shader_type stype>
xray::rendering::directx10::gpu_shader<stype>::gpu_shader(renderer* r_ptr,
                                                          const char* src_code,
                                                          const size_t code_len,
                                                          const uint32_t compile_flags)
    : core_{ stype, r_ptr->dev_ctx(), src_code, code_len, compile_flags }
{

    if (core_)
        handle_ = traits::create(r_ptr->dev_ctx(), core_.bytecode());
}

template<shader_type stype>
xray::rendering::directx10::gpu_shader<stype>::~gpu_shader()
{
}

template<shader_type stype>
template<typename T>
void
xray::rendering::directx10::gpu_shader<stype>::set_uniform(const char* name, const T& value) noexcept
{
    assert(valid());
    core_.set_uniform(name, &value, sizeof(value));
}

template<shader_type stype>
template<typename T>
void
xray::rendering::directx10::gpu_shader<stype>::set_uniform_block(const char* name, const T& value) noexcept
{
    assert(valid());
    core_.set_uniform_block(name, &value, sizeof(value));
}

template<shader_type stype>
void
xray::rendering::directx10::gpu_shader<stype>::bind(renderer* r_ptr)
{
    assert(valid());

    auto device = r_ptr->dev_ctx();
    traits::bind(device, handle());
    core_.bind(device, &traits::set_constant_buffer);
}

} // namespace directx10
} // namespace rendering
} // namespace xray
