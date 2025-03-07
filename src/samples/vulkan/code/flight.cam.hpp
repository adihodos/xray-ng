#pragma once

#include <filesystem>

#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4.hpp"

namespace JPH {
class Body;
}

namespace B5 {

struct FlightCameraParams
{
    xray::math::vec3f camera_offset{ 0.0f, 1.2f, -3.0f };
    xray::math::vec2f look_angle_limits{ 170.0f, 90.0f };
    // must be in the [0, 1] range
    float look_alpha{ 0.25f };
    // must be in the [0, 1] range
    float movement_scale{ 0.25f };
    // must be in the [0, 1] range
    float movement_alpha{ 0.25f };

    static FlightCameraParams from_file(const std::filesystem::path& file_path);
};

struct FlightCamera
{
    FlightCameraParams params;
    //
    // input along the X axis -> yaw
    // input along the Y axis -> pitch
    xray::math::vec2f look_input{ 0, 0 };
    //
    // x - yaw angle
    // y - pitch angle
    xray::math::vec2f look_average_angle{ 0, 0 };
    xray::math::vec3f angular_velocity_average{ 0, 0, 0 };

    xray::math::MatrixWithInvertedMatrixPair4f update(const JPH::Body& target) noexcept;
};

}
