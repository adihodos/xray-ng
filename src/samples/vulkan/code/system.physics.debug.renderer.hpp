#pragma once

#if defined(JPH_DEBUG_RENDERER)

#include <cstddef>
#include <string_view>
#include <mutex>
#include <span>
#include <atomic>
#include <cstdint>

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Core/Reference.h>

#include <xray/base/unique_pointer.hpp>
#include <xray/base/memory.arena.hpp>
#include <xray/base/containers/arena.vector.hpp>
#include <xray/math/scalar3.hpp>

#include <xray/rendering/vulkan.renderer/vulkan.buffer.hpp>
#include <xray/rendering/vulkan.renderer/vulkan.error.hpp>
#include <xray/rendering/vulkan.renderer/vulkan.pipeline.hpp>

namespace B5 {

struct InitContext;
struct RenderEvent;

class PhysicsEngineDebugRenderer : public JPH::DebugRenderer
{
  public:
    virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

    virtual void DrawTriangle(JPH::RVec3Arg inV1,
                              JPH::RVec3Arg inV2,
                              JPH::RVec3Arg inV3,
                              JPH::ColorArg inColor,
                              ECastShadow inCastShadow = ECastShadow::Off) override;

    virtual void DrawText3D(JPH::RVec3Arg inPosition,
                            const std::string_view& inString,
                            JPH::ColorArg inColor = JPH::Color::sWhite,
                            float inHeight = 0.5f) override;

    /// Create a batch of triangles that can be drawn efficiently
    virtual Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;

    virtual Batch CreateTriangleBatch(const Vertex* inVertices,
                                      int inVertexCount,
                                      const JPH::uint32* inIndices,
                                      int inIndexCount) override;

    /// Draw some geometry
    /// @param inModelMatrix is the matrix that transforms the geometry to world space.
    /// @param inWorldSpaceBounds is the bounding box of the geometry after transforming it into world space.
    /// @param inLODScaleSq is the squared scale of the model matrix, it is multiplied with the LOD distances in
    /// inGeometry to calculate the real LOD distance (so a number > 1 will force a higher LOD).
    /// @param inModelColor is the color with which to multiply the vertex colors in inGeometry.
    /// @param inGeometry The geometry to draw.
    /// @param inCullMode determines which polygons are culled.
    /// @param inCastShadow determines if this geometry should cast a shadow or not.
    /// @param inDrawMode determines if we draw the geometry solid or in wireframe.
    virtual void DrawGeometry(JPH::RMat44Arg inModelMatrix,
                              const JPH::AABox& inWorldSpaceBounds,
                              float inLODScaleSq,
                              JPH::ColorArg inModelColor,
                              const GeometryRef& inGeometry,
                              ECullMode inCullMode = ECullMode::CullBackFace,
                              ECastShadow inCastShadow = ECastShadow::On,
                              EDrawMode inDrawMode = EDrawMode::Solid) override;

    static xray::base::unique_pointer<PhysicsEngineDebugRenderer> create(const InitContext& ctx);

    void draw(const RenderEvent& e);

  private:
    /// Implementation specific batch object
    class BatchImpl : public JPH::RefTargetVirtual
    {
      public:
        JPH_OVERRIDE_NEW_DELETE

        virtual void AddRef() override { ++mRefCount; }
        virtual void Release() override
        {
            if (--mRefCount == 0)
                delete this;
        }

        JPH::Array<Triangle> mTriangles;

      private:
        std::atomic<uint32_t> mRefCount{};
    };

    struct PhysDebugRenderResourcesBundle
    {
        std::span<std::byte> arena_mem;
        xray::rendering::VulkanBuffer gpu_buffer;
        xray::rendering::GraphicsPipeline pipeline;
    };

    //
    // no need to release the arenas, OS cleans up when the process terminates

    xray::base::MemoryArena _line_arena;
    size_t _lines_count{};
    std::mutex _lines_lock;
    xray::rendering::VulkanBuffer _gpu_lines;
    xray::rendering::GraphicsPipeline _lines_pp;

    std::mutex _yftris_lock;
    xray::base::MemoryArena _yftris_arena;
    xray::rendering::VulkanBuffer _gpu_yftris;
    xray::rendering::GraphicsPipeline _yftris_pp;
    size_t _yftriangles{};

    std::mutex _fill_tris_lock;
    xray::base::MemoryArena _filled_tris_arena;
    xray::rendering::VulkanBuffer _gpu_filled_tris;
    xray::rendering::GraphicsPipeline _filled_tris_pp;
    size_t _filled_tris{};

  public:
    struct Line
    {
        JPH::Float3 from;
        JPH::Color from_color;
        JPH::Float3 to;
        JPH::Color to_color;
    };

    JPH::RVec3 _cam_pos;
    PhysicsEngineDebugRenderer(PhysDebugRenderResourcesBundle res_lines,
                               PhysDebugRenderResourcesBundle res_yftris,
                               PhysDebugRenderResourcesBundle res_filled_tris);
};

}

#endif
