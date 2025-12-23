#include "flight.cam.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

#include "xray/base/logger.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/math_std.hpp"
#include "xray/base/serialization/rfl.libconfig/config.load.hpp"
#include "xray/math/serialization/parser.scalar2.hpp"
#include "xray/math/serialization/parser.scalar3.hpp"

namespace B5 {

using namespace xray::math;

FlightCameraParams
FlightCameraParams::from_file(const std::filesystem::path& file_path)
{
    const rfl::Result<FlightCameraParams> loaded_params = rfl::libconfig::read<FlightCameraParams>(file_path);

    if (!loaded_params) {
        XR_LOG_INFO("Failed to load camera parameters: file: ({}), error: {}. Using defaults.",
                    file_path.generic_string(),
                    "unknow - fix this RFL fiasco"
                    //loaded_params.error()->what()
                    );
    }

    return loaded_params.value_or(FlightCameraParams{});
}

xray::math::MatrixWithInvertedMatrixPair4f
FlightCamera::update(const JPH::Body& target) noexcept
{
    const vec2f target_look_angle = look_input * params.look_angle_limits;
    look_average_angle = xray::math::mix(look_average_angle, target_look_angle, params.look_alpha);

    const JPH::Quat rotation_from_input{ JPH::Quat::sEulerAngles(
        JPH::Vec3{ radians(-look_average_angle.y), radians(look_average_angle.x), 0.0f }) };

    const JPH::Vec3 ship_position = target.GetCenterOfMassTransform().GetTranslation();

    MatrixWithInvertedMatrixPair4f camera_transforms;
    JPH::Mat44 cam_rot;
    JPH::Vec3 cam_pos;

    if (params.fixed_cam && pivot_mode) {
        //
        // does not follow the target's rotations
        cam_rot = JPH::Mat44::sRotation(rotation_from_input);
        cam_pos = ship_position +
                  cam_rot * JPH::Vec3{ params.camera_offset.x, params.camera_offset.y, params.camera_offset.z };
    } else {
        const JPH::Vec3 target_ang_vel = target.GetAngularVelocity();
        const vec3f target_angular_velocity{ target_ang_vel.GetX(), target_ang_vel.GetY(), -target_ang_vel.GetZ() };

        angular_velocity_average =
            xray::math::mix(angular_velocity_average, target_angular_velocity, params.movement_alpha);

        const JPH::Quat rotation_from_angular_vel{ JPH::Quat::sEulerAngles(
            JPH::Vec3{ radians(-angular_velocity_average.x),
                       radians(-angular_velocity_average.y),
                       radians(angular_velocity_average.z) } *
            params.movement_scale) };

        const JPH::Quat ship_orientation = target.GetRotation();

        cam_rot = JPH::Mat44::sRotation(ship_orientation * rotation_from_input * rotation_from_angular_vel);
        cam_pos = ship_position +
                  cam_rot * JPH::Vec3{ params.camera_offset.x, params.camera_offset.y, params.camera_offset.z };
    }

    const JPH::Mat44 world_to_camera = cam_rot.Transposed() * JPH::Mat44::sTranslation(-cam_pos);

    world_to_camera.Transposed().StoreFloat4x4(reinterpret_cast<JPH::Float4*>(camera_transforms.transform.components));
    (JPH::Mat44::sTranslation(cam_pos) * cam_rot)
        .Transposed()
        .StoreFloat4x4(reinterpret_cast<JPH::Float4*>(camera_transforms.inverted.components));

    return camera_transforms;
}

}
