#include "xray/rendering/directx/scoped_mapping.hpp"

xray::rendering::scoped_resource_mapping::scoped_resource_mapping(ID3D11DeviceContext* dc,
                                                                  ID3D11Resource* res,
                                                                  const D3D11_MAP map_type,
                                                                  const uint32_t subres_idx /*= 0*/)
    : _devctx{ dc }
    , _resource{ res }
    , _subresource{ subres_idx } /*_valid{false}*/
{
    _devctx->Map(_resource, _subresource, map_type, 0, &_mapping);
}

xray::rendering::scoped_resource_mapping::~scoped_resource_mapping()
{
    if (_mapping.pData) {
        _devctx->Unmap(_resource, _subresource);
    }
}
