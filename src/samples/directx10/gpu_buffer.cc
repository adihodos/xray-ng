#include "gpu_buffer.hpp"
#include "renderer_dx10.hpp"

using namespace xray::base;

xray::rendering::directx10::gpu_buffer::gpu_buffer(renderer* r_ptr,
                                                   const size_t e_size,
                                                   const size_t e_count,
                                                   const uint32_t bind_flags,
                                                   const void* initial_data /* = nullptr */,
                                                   const bool gpu_write /* = false */,
                                                   const bool cpu_acc /* = false */)
{

    D3D10_BUFFER_DESC buff_desc;
    buff_desc.MiscFlags = 0;
    buff_desc.ByteWidth = static_cast<uint32_t>(e_size * e_count);
    D3D10_SUBRESOURCE_DATA sr_data = { initial_data, buff_desc.ByteWidth, 0 };
    const D3D10_SUBRESOURCE_DATA* sr_ptr = nullptr;

    if (gpu_write) {
        buff_desc.Usage = cpu_acc ? D3D10_USAGE_STAGING : D3D10_USAGE_DEFAULT;
    } else {
        buff_desc.Usage = cpu_acc ? D3D10_USAGE_DYNAMIC : D3D10_USAGE_IMMUTABLE;
    }

    buff_desc.BindFlags = bind_flags;
    buff_desc.CPUAccessFlags = 0;

    switch (buff_desc.Usage) {
        case D3D10_USAGE_STAGING:
            buff_desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
            break;

        case D3D10_USAGE_DYNAMIC:
            buff_desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
            break;

        default:
            break;
    }

    if (buff_desc.BindFlags == D3D10_BIND_CONSTANT_BUFFER) {
        assert((buff_desc.ByteWidth % 16 == 0) && "Constant buffer size must be a multiple of 16!");
    }

    if (buff_desc.Usage == D3D10_USAGE_IMMUTABLE || initial_data) {
        sr_ptr = &sr_data;
    }

    r_ptr->dev_ctx()->CreateBuffer(&buff_desc, sr_ptr, raw_ptr_ptr(buff_handle_));
    if (!buff_handle_)
        return;

    e_count_ = e_count;
    e_size_ = e_size;
}

xray::rendering::directx10::gpu_buffer::~gpu_buffer() {}
