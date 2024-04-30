#include "scoped_resource_mapping.hpp"

xray::rendering::directx10::scoped_buffer_mapping::scoped_buffer_mapping(ID3D10Buffer* buff, const D3D10_MAP type)
    : resource_{ buff }
{
    assert(buff != nullptr);
    resource_->Map(type, 0, &mapping_);
}

xray::rendering::directx10::scoped_buffer_mapping::~scoped_buffer_mapping()
{
    if (valid())
        resource_->Unmap();
}
