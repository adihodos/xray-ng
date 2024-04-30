#include "rasterizer_state.hpp"

xray::rendering::directx10::rasterizer_state::rasterizer_state() noexcept
{
    FillMode = D3D10_FILL_SOLID;
    CullMode = D3D10_CULL_BACK;
    FrontCounterClockwise = false;
    DepthBias = 0;
    DepthBiasClamp = 0.0f;
    SlopeScaledDepthBias = 0.0f;
    DepthClipEnable = true;
    ScissorEnable = false;
    MultisampleEnable = false;
    AntialiasedLineEnable = false;
}