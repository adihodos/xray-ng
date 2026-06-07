

#include <third.party/jolt-physics/Jolt/Physics/Constraints/ConeConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/Constraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/ConstraintManager.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/ContactConstraintManager.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/DistanceConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/FixedConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/GearConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/HingeConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/MotorSettings.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/PathConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/PathConstraintPath.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/PathConstraintPathHermite.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/PointConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/PulleyConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/RackAndPinionConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/SixDOFConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/SliderConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/SpringSettings.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/SwingTwistConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Constraints/TwoBodyConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/DeterminismLog.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Hair/Hair.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Hair/HairSettings.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Hair/HairShaders.cpp>
#include <third.party/jolt-physics/Jolt/Physics/IslandBuilder.cpp>
#include <third.party/jolt-physics/Jolt/Physics/LargeIslandSplitter.cpp>
#include <third.party/jolt-physics/Jolt/Physics/PhysicsScene.cpp>
#include <third.party/jolt-physics/Jolt/Physics/PhysicsSystem.cpp>
#include <third.party/jolt-physics/Jolt/Physics/PhysicsUpdateContext.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Ragdoll/Ragdoll.cpp>
#include <third.party/jolt-physics/Jolt/Physics/SoftBody/SoftBodyCreationSettings.cpp>
#include <third.party/jolt-physics/Jolt/Physics/SoftBody/SoftBodyMotionProperties.cpp>
#include <third.party/jolt-physics/Jolt/Physics/SoftBody/SoftBodyShape.cpp>
#include <third.party/jolt-physics/Jolt/Physics/SoftBody/SoftBodySharedSettings.cpp>
#include <third.party/jolt-physics/Jolt/Physics/StateRecorderImpl.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/MotorcycleController.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/TrackedVehicleController.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleAntiRollBar.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleCollisionTester.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleConstraint.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleController.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleDifferential.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleEngine.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleTrack.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/VehicleTransmission.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/Wheel.cpp>
#include <third.party/jolt-physics/Jolt/Physics/Vehicle/WheeledVehicleController.cpp>
#include <third.party/jolt-physics/Jolt/RegisterTypes.cpp>
#include <third.party/jolt-physics/Jolt/Renderer/DebugRenderer.cpp>
#include <third.party/jolt-physics/Jolt/Renderer/DebugRendererPlayback.cpp>
#include <third.party/jolt-physics/Jolt/Renderer/DebugRendererRecorder.cpp>
#include <third.party/jolt-physics/Jolt/Renderer/DebugRendererSimple.cpp>
#include <third.party/jolt-physics/Jolt/Shaders/HairWrapper.cpp>
#include <third.party/jolt-physics/Jolt/Shaders/TestComputeWrapper.cpp>
#include <third.party/jolt-physics/Jolt/Skeleton/SkeletalAnimation.cpp>
#include <third.party/jolt-physics/Jolt/Skeleton/Skeleton.cpp>
#include <third.party/jolt-physics/Jolt/Skeleton/SkeletonMapper.cpp>
#include <third.party/jolt-physics/Jolt/Skeleton/SkeletonPose.cpp>
#include <third.party/jolt-physics/Jolt/TriangleSplitter/TriangleSplitter.cpp>
#include <third.party/jolt-physics/Jolt/TriangleSplitter/TriangleSplitterBinning.cpp>
#include <third.party/jolt-physics/Jolt/TriangleSplitter/TriangleSplitterMean.cpp>

// #include <third.party/jolt-physics/Samples/SamplesApp.cpp>
// #include <third.party/jolt-physics/Samples/Tests/BroadPhase/BroadPhaseCastRayTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/BroadPhase/BroadPhaseInsertionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/BroadPhase/BroadPhaseTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Character/CharacterBaseTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Character/CharacterPlanetTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Character/CharacterSpaceShipTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Character/CharacterTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Character/CharacterVirtualTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/ConeConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/ConstraintPriorityTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/ConstraintSingularityTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/ConstraintVsCOMChangeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/DistanceConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/FixedConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/GearConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/HingeConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/PathConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/PointConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/PoweredHingeConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/PoweredSliderConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/PoweredSwingTwistConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/PulleyConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/RackAndPinionConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/SixDOFConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/SliderConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/SpringTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/SwingTwistConstraintFrictionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Constraints/SwingTwistConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ConvexCollision/CapsuleVsBoxTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ConvexCollision/ClosestPointTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ConvexCollision/ConvexHullShrinkTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ConvexCollision/ConvexHullTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ConvexCollision/EPATest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ConvexCollision/InteractivePairsTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ConvexCollision/RandomRayTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ActivateDuringUpdateTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ActiveEdgesTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/AllowedDOFsTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/BigVsSmallTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/CenterOfMassTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ChangeMotionQualityTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ChangeMotionTypeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ChangeObjectLayerTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ChangeShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ContactListenerTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ContactManifoldTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ConveyorBeltTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/DampingTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/DynamicMeshTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/EnhancedInternalEdgeRemovalTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/FrictionPerTriangleTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/FrictionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/FunnelTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/GravityFactorTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/GyroscopicForceTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/HeavyOnLightTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/HighSpeedTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/IslandTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/KinematicTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/LoadSaveBinaryTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/LoadSaveSceneTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ManifoldReductionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ModifyMassTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/MultithreadedTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/PyramidTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/RestitutionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/SensorTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/ShapeFilterTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/SimCollideBodyVsBodyTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/SimShapeFilterTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/SimpleTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/StackTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/TwoDFunnelTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/General/WallTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Hair/HairCollisionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Hair/HairGravityPreloadTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Hair/HairTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/BigWorldTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/CreateRigTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/KinematicRigTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/LoadRigTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/LoadSaveBinaryRigTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/LoadSaveRigTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/PoweredRigTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/RigPileTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/SkeletonMapperTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Rig/SoftKeyframedRigTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/DynamicScaledShape.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledBoxShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledCapsuleShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledConvexHullShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledCylinderShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledHeightFieldShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledMeshShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledMutableCompoundShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledOffsetCenterOfMassShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledPlaneShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledSphereShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledStaticCompoundShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledTaperedCapsuleShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledTaperedCylinderShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/ScaledShapes/ScaledTriangleShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/BoxShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/CapsuleShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/ConvexHullShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/CylinderShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/DeformedHeightFieldShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/EmptyShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/HeightFieldShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/MeshShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/MeshShapeUserDataTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/MutableCompoundShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/OffsetCenterOfMassShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/PlaneShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/RotatedTranslatedShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/SphereShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/StaticCompoundShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/TaperedCapsuleShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/TaperedCylinderShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Shapes/TriangleShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyBendConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyContactListenerTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyCosseratRodConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyCustomUpdateTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyForceTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyFrictionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyGravityFactorTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyKinematicTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyLRAConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyPressureTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyRestitutionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodySensorTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyShapesTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodySkinnedConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyStressTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyUpdatePositionTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyVertexRadiusTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/SoftBody/SoftBodyVsFastMovingTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Test.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Tools/LoadSnapshotTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Vehicle/MotorcycleTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Vehicle/TankTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Vehicle/VehicleConstraintTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Vehicle/VehicleSixDOFTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Vehicle/VehicleStressTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Vehicle/VehicleTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Water/BoatTest.cpp>
// #include <third.party/jolt-physics/Samples/Tests/Water/WaterShapeTest.cpp>
// #include <third.party/jolt-physics/Samples/Utils/ContactListenerImpl.cpp>
// #include <third.party/jolt-physics/Samples/Utils/RagdollLoader.cpp>
// #include <third.party/jolt-physics/Samples/Utils/ShapeCreator.cpp>
// #include <third.party/jolt-physics/Samples/Utils/SoftBodyCreator.cpp>
