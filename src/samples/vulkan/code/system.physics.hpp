#pragma once

#include <cstdint>
#include <tl/expected.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

#include <xray/base/unique_pointer.hpp>

namespace JPH {
    class PhysicsSystem;
}

namespace B5 {

struct PhysicsSystemError
{};

class PhysicsSystem
{
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

  public:
    struct PhysicsSystemState;

    PhysicsSystem(PrivateConstructionToken, xray::base::unique_pointer<PhysicsSystemState> phys_state) noexcept;
    PhysicsSystem(PhysicsSystem&&) noexcept;
    ~PhysicsSystem();

    struct ObjectLayers
    {
        static constexpr const JPH::ObjectLayer NON_MOVING = 0;
        static constexpr const JPH::ObjectLayer MOVING = 1;
        static constexpr const uint32_t NUM_LAYERS = 2;
    };

    struct BroadPhaseLayers
    {
        static constexpr const JPH::BroadPhaseLayer NON_MOVING{ 0 };
        static constexpr const JPH::BroadPhaseLayer MOVING{ 1 };
        static constexpr uint32_t NUM_LAYERS = 2;
    };

    // This is the max amount of rigid bodies that you can add to the physics system.
    static constexpr const uint32_t cMaxBodies = 65536;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access, 0 for the
    // default settings.
    static constexpr const uint32_t cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
    // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this
    // buffer too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is
    // slightly less efficient.
    static constexpr const uint32_t cMaxBodyPairs = 65536;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are
    // detected than this number then these contacts will be ignored and bodies will start interpenetrating / fall
    // through the world.
    static constexpr const uint32_t cMaxContactConstraints = 10240;

    // We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
    static constexpr const float cDeltaTime = 1.0f / 60.0f;

    static tl::expected<PhysicsSystem, PhysicsSystemError> create();

    void update();

    JPH::PhysicsSystem* physics() noexcept;

    static void test_something();

  private:
    xray::base::unique_pointer<PhysicsSystemState> _phys_state;
};
}
