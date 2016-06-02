#pragma once

#include <cstdint>
#include "xray/xray.hpp"
#include "xray/math/scalar2.hpp"

namespace app {

constexpr xray::math::float2 plane_vertices[] = {
    {+0.8f, -0.4f}, {+0.2f, +0.1f}, {+0.0f, +0.8f}, {-0.2f, +0.1f},
    {-0.8f, -0.4f}, {-0.2f, -0.4f}, {-0.1f, -0.5f}, {-0.4f, -0.8f},
    {+0.0f, -0.6f}, {+0.4f, -0.8f}, {+0.1f, -0.5f}, {+0.2f, -0.4f}};

constexpr uint16_t plane_indices[] = {0, 1, 1, 2, 2, 3, 3, 4,  4,  5,  5,  6,
                                      6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 0};

}  // namespace app
