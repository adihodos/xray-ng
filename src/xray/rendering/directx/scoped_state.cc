#include "xray/rendering/directx/scoped_state.hpp"

xray::rendering::scoped_blend_state::scoped_blend_state(ID3D11DeviceContext* context,
                                                        ID3D11BlendState* new_state,
                                                        const float* blend_factors,
                                                        const uint32_t sample_mask) noexcept
    : _context{ context }
{
    _context->OMGetBlendState(&_saved_state, _blend_factors, &_mask);
    _context->OMSetBlendState(new_state, blend_factors, sample_mask);
}

xray::rendering::scoped_depth_stencil_state::scoped_depth_stencil_state(ID3D11DeviceContext* context,
                                                                        ID3D11DepthStencilState* new_state,
                                                                        const uint32_t stencil_ref) noexcept
    : _context{ context }
{
    _context->OMGetDepthStencilState(&_saved_state, &_saved_stencil_ref);
    _context->OMSetDepthStencilState(new_state, stencil_ref);
}

xray::rendering::scoped_viewport::scoped_viewport(ID3D11DeviceContext* context,
                                                  const D3D11_VIEWPORT* viewports,
                                                  const size_t num_viewports) noexcept
    : _context{ context }
{
    _context->RSGetViewports(&_viewport_count, _saved_viewport);

    if (viewports)
        _context->RSSetViewports(static_cast<uint32_t>(num_viewports), viewports);
}

xray::rendering::scoped_viewport::~scoped_viewport() noexcept
{
    _context->RSSetViewports(_viewport_count, _saved_viewport);
}

xray::rendering::scoped_scissor_rects::scoped_scissor_rects(ID3D11DeviceContext* context,
                                                            const D3D11_RECT* scissor_rects,
                                                            const size_t num_rects) noexcept
    : _context{ context }
{

    _context->RSGetScissorRects(&_scissors_count, _saved_scissor_rects);

    if (scissor_rects)
        _context->RSSetScissorRects(static_cast<uint32_t>(num_rects), scissor_rects);
}

xray::rendering::scoped_scissor_rects::~scoped_scissor_rects() noexcept
{
    _context->RSSetScissorRects(_scissors_count, _saved_scissor_rects);
}
