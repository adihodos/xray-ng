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
    uint32_t width{ 512 };
    uint32_t height{ 512 };
    float scale{ 16.0f };
    float bias{ 8.0f };
    uint32_t octaves{ 6 };
    float xmin{-1.0f};
    float xmax{1.0f};
    float zmin{-1.0f};
    float zmax{1.0f};
    // meters
    float sea_level{0.0f};
};

}
