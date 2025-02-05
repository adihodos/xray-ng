#include "system.physics.hpp"

#include <tracy/Tracy.hpp>

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "xray/base/logger.hpp"

namespace B5 {

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1) {
            case PhysicsSystem::ObjectLayers::NON_MOVING:
                return inObject2 == PhysicsSystem::ObjectLayers::MOVING; // Non moving only collides with moving
            case PhysicsSystem::ObjectLayers::MOVING:
                return true; // Moving collides with everything
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
  public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[PhysicsSystem::ObjectLayers::NON_MOVING] = PhysicsSystem::BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[PhysicsSystem::ObjectLayers::MOVING] = PhysicsSystem::BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override { return PhysicsSystem::BroadPhaseLayers::NUM_LAYERS; }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < PhysicsSystem::ObjectLayers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer) {
            case (JPH::BroadPhaseLayer::Type)PhysicsSystem::BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)PhysicsSystem::BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

  private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[PhysicsSystem::ObjectLayers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1) {
            case PhysicsSystem::ObjectLayers::NON_MOVING:
                return inLayer2 == PhysicsSystem::BroadPhaseLayers::MOVING;
            case PhysicsSystem::ObjectLayers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

PhysicsSystem::~PhysicsSystem() {}

void
PhysicsSystem::test_something()
{
    JPH::RegisterDefaultAllocator();
    xray::base::unique_pointer<JPH::Factory> factory{ new JPH::Factory{} };
    JPH::Factory::sInstance = xray::base::raw_ptr(factory);
    JPH::RegisterTypes();
    JPH::TempAllocatorImpl temp_allocator{ 10 * 1024 * 1024 };
    JPH::JobSystemThreadPool job_system{ JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 4 };
    BPLayerInterfaceImpl broad_phase_layer_interface;
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;
    JPH::PhysicsSystem physics;
    physics.Init(PhysicsSystem::cMaxBodies,
                 PhysicsSystem::cNumBodyMutexes,
                 PhysicsSystem::cMaxBodyPairs,
                 PhysicsSystem::cMaxContactConstraints,
                 broad_phase_layer_interface,
                 object_vs_broadphase_layer_filter,
                 object_vs_object_layer_filter);

    using namespace JPH;
    using namespace JPH::literals;

    // The main way to interact with the bodies in the physics system is through the body interface. There is a locking
    // and a non-locking
    // variant of this. We're going to use the locking version (even though we're not planning to access bodies from
    // multiple threads)
    BodyInterface& body_interface = physics.GetBodyInterface();

    // Next we can create a rigid body to serve as the floor, we make a large box
    // Create the settings for the collision volume (the shape).
    // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
    BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));
    floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as
                                        // such to prevent it from being freed when its reference count goes to 0.

    // Create the shape
    ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
    ShapeRefC floor_shape =
        floor_shape_result
            .Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

    // Create the settings for the body itself. Note that here you can also set other properties like the restitution /
    // friction.
    BodyCreationSettings floor_settings(floor_shape,
                                        RVec3(0.0_r, -1.0_r, 0.0_r),
                                        Quat::sIdentity(),
                                        EMotionType::Static,
                                        B5::PhysicsSystem::ObjectLayers::NON_MOVING);

    // Create the actual rigid body
    Body* floor =
        body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

    // Add it to the world
    body_interface.AddBody(floor->GetID(), EActivation::DontActivate);

    // Now create a dynamic body to bounce on the floor
    // Note that this uses the shorthand version of creating and adding a body to the world
    BodyCreationSettings sphere_settings(new SphereShape(0.5f),
                                         RVec3(0.0_r, 2.0_r, 0.0_r),
                                         Quat::sIdentity(),
                                         EMotionType::Dynamic,
                                         B5::PhysicsSystem::ObjectLayers::MOVING);
    BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);
    // Now you can interact with the dynamic body, in this case we're going to give it a velocity.
    // (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to
    // the physics system)
    body_interface.SetLinearVelocity(sphere_id, Vec3(0.0f, -5.0f, 0.0f));

    // We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
    const float cDeltaTime = 1.0f / 60.0f;

    // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision
    // detection performance (it's pointless here because we only have 2 bodies). You should definitely not call this
    // every frame or when e.g. streaming in a new level section as it is an expensive operation. Instead insert all new
    // objects in batches instead of 1 at a time to keep the broad phase efficient.
    physics.OptimizeBroadPhase();

    // Now we're ready to simulate the body, keep simulating until it goes to sleep
    uint step = 0;
    while (body_interface.IsActive(sphere_id)) {
        // Next step
        ++step;

        // Output current position and velocity of the sphere
        RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
        Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
        XR_LOG_INFO("Step {} Position = ({},{},{}), velocity = ({},{},{})",
                    step,
                    position.GetX(),
                    position.GetY(),
                    position.GetZ(),
                    velocity.GetX(),
                    velocity.GetY(),
                    velocity.GetZ());

        // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep
        // the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
        const int cCollisionSteps = 1;

        // Step the world
        physics.Update(cDeltaTime, cCollisionSteps, &temp_allocator, &job_system);
    }
}

struct PhysicsSystem::PhysicsSystemState
{
    PhysicsSystemState()
        : factory{ []() {
            JPH::RegisterDefaultAllocator();
            xray::base::unique_pointer<JPH::Factory> factory{ new JPH::Factory{} };
            JPH::Factory::sInstance = xray::base::raw_ptr(factory);

            //
            // Register all physics types with the factory and install their collision handlers with the
            // CollisionDispatch class. If you have your own custom shape types you probably need to register their
            // handlers with the CollisionDispatch before calling this function. If you implement your own default
            // material (PhysicsMaterial::sDefault) make sure to initialize it before this function or else this
            // function will create one for you.
            JPH::RegisterTypes();

            return factory;
        }() }
    {
        physics.Init(PhysicsSystem::cMaxBodies,
                     PhysicsSystem::cNumBodyMutexes,
                     PhysicsSystem::cMaxBodyPairs,
                     PhysicsSystem::cMaxContactConstraints,
                     broad_phase_layer_interface,
                     object_vs_broadphase_layer_filter,
                     object_vs_object_layer_filter);
        physics.SetGravity(JPH::Vec3::sZero());
    }

    xray::base::unique_pointer<JPH::Factory> factory;
    // We need a temp allocator for temporary allocations during the physics update. We're
    // pre-allocating 10 MB to avoid having to do allocations during the physics update.
    // If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
    // malloc / free.
    JPH::TempAllocatorImpl temp_allocator{ 10 * 1024 * 1024 };

    // We need a job system that will execute physics jobs on multiple threads. Typically
    // you would implement the JobSystem interface yourself and let Jolt Physics run on top
    // of your own job scheduler. JobSystemThreadPool is an example implementation.
    JPH::JobSystemThreadPool job_system{ JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 4 };

    // Create mapping table from object layer to broadphase layer
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at BroadPhaseLayerInterfaceTable or BroadPhaseLayerInterfaceMask for a simpler interface.
    BPLayerInterfaceImpl broad_phase_layer_interface;

    // Create class that filters object vs broadphase layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at ObjectVsBroadPhaseLayerFilterTable or ObjectVsBroadPhaseLayerFilterMask for a simpler
    // interface.
    ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;

    // Create class that filters object vs object layers
    // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
    // Also have a look at ObjectLayerPairFilterTable or ObjectLayerPairFilterMask for a simpler interface.
    ObjectLayerPairFilterImpl object_vs_object_layer_filter;

    JPH::PhysicsSystem physics;
};

PhysicsSystem::PhysicsSystem(PhysicsSystem::PrivateConstructionToken,
                             xray::base::unique_pointer<PhysicsSystemState> phys_state) noexcept
    : _phys_state{ std::move(phys_state) }
{
}

PhysicsSystem::PhysicsSystem(PhysicsSystem&& rhs) noexcept
    : _phys_state{ std::move(rhs._phys_state) }
{
}

tl::expected<PhysicsSystem, PhysicsSystemError>
PhysicsSystem::create()
{
    return tl::expected<PhysicsSystem, PhysicsSystemError>{
        tl::in_place,
        PhysicsSystem::PrivateConstructionToken{},
        xray::base::make_unique<PhysicsSystemState>(),
    };
}

void
PhysicsSystem::update()
{
    ZoneScopedNC("Physics update", tracy::Color::Red2);
    _phys_state->physics.Update(PhysicsSystem::cDeltaTime, 1, &_phys_state->temp_allocator, &_phys_state->job_system);
}

JPH::PhysicsSystem*
PhysicsSystem::physics() noexcept
{
    return &_phys_state->physics;
}

}
