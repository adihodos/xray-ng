#pragma once

#include "gpu_shader.hpp"
#include "gpu_shader_traits.hpp"
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

template<>
struct shader_traits<shader_type::fragment>
{
    using handle_type = ID3D10PixelShader*;
    using scoped_shader_type = base::com_ptr<ID3D10PixelShader>;

    static scoped_shader_type create(ID3D10Device* device, ID3D10Blob* blob)
    {
        scoped_shader_type new_shader;
        device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), raw_ptr_ptr(new_shader));

        return new_shader;
    }

    static void bind(ID3D10Device* device, ID3D10PixelShader* shader) noexcept { device->PSSetShader(shader); }

    static void set_constant_buffer(ID3D10Device* device,
                                    const uint32_t start_slot,
                                    const uint32_t num_buffers,
                                    ID3D10Buffer* const* buffers)
    {
        device->PSSetConstantBuffers(start_slot, num_buffers, buffers);
    }
};

using fragment_shader = gpu_shader<shader_type::fragment>;

} // namespace directx10
} // namespace rendering
} // namespace xray
