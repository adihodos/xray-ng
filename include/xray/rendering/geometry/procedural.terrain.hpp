#pragma once

#include <cstdint>
#include <string>
#include "xray/math/scalar4.hpp"

namespace xray::rendering {

struct TerrainRange
{
    std::string name;
    float height;
    math::vec4f color;
};

struct TerrainParams
{
    uint32_t size{ 512 };
    uint32_t lods{ 8 };
    float scale{ 16.0f };
    float bias{ 8.0f };
    uint32_t octaves{ 6 };
    // meters
    float sea_level{ 0.0f };
    float magnitude;
};

}
