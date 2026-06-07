#pragma once

#include <span>
#include <xray/rendering/vertex_format/vertex_format.hpp>
#include <xray/math/scalar2.hpp>
#include <xray/math/scalar3.hpp>
#include <xray/math/scalar4.hpp>

namespace xray::rendering {

struct VertexPBR
{
    math::vec3f pos;
    math::vec3f normal;
    math::vec4f color;
    math::vec4f tangent;
    math::vec2f uv;
    uint32_t pbr_buf_id;

    struct stdc;
};

struct VertexPBR::stdc
{
    static constexpr const VertexInputAttributeDescriptor description[] = {
        { 3, component_type::float_, XR_U32_OFFSETOF(VertexPBR, pos), 0, 0 },
        { 3, component_type::float_, XR_U32_OFFSETOF(VertexPBR, normal), 1, 0 },
        { 4, component_type::float_, XR_U32_OFFSETOF(VertexPBR, color), 2, 0 },
        { 4, component_type::float_, XR_U32_OFFSETOF(VertexPBR, tangent), 3, 0 },
        { 2, component_type::float_, XR_U32_OFFSETOF(VertexPBR, uv), 4, 0 },
        { 1, component_type::unsigned_int, XR_U32_OFFSETOF(VertexPBR, pbr_buf_id), 5, 0 },
    };

    static constexpr const uint32_t size = static_cast<uint32_t>(sizeof(VertexPBR));
    static constexpr const std::span<const VertexInputAttributeDescriptor> descriptor{ description,
                                                                                       std::size(description) };
};

}
