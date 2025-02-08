#include "system.physics.debug.renderer.hpp"

#if defined(JPH_DEBUG_RENDERER)

#include <xray/xray.hpp>
#include <xray/base/logger.hpp>
#include <xray/rendering/shader.code.builder.hpp>
#include <xray/rendering/vulkan.renderer/vulkan.renderer.hpp>
#include <xray/rendering/vulkan.renderer/vulkan.pipeline.hpp>
#include <xray/rendering/vertex_format/vertex_pc.hpp>

#if defined(XRAY_OS_IS_POSIX_FAMILY)
#include <sys/mman.h>
#endif

#include "events.hpp"
#include "push.constant.packer.hpp"

namespace B5 {

const size_t MAX_LINES = 4096;
const size_t MAX_YF_TRIANGLES = 65536;

PhysicsEngineDebugRenderer::PhysicsEngineDebugRenderer(PhysDebugRenderResourcesBundle res_lines,
                                                       PhysDebugRenderResourcesBundle res_yftris)
    : _line_arena{ res_lines.arena_mem }
    , _gpu_lines{ std::move(res_lines.gpu_buffer) }
    , _lines_pp{ std::move(res_lines.pipeline) }
    , _yftris_arena{ res_yftris.arena_mem }
    , _gpu_yftris{ std::move(res_yftris.gpu_buffer) }
    , _yftris_pp{ std::move(res_yftris.pipeline) }

{
    DebugRenderer::Initialize();
}

void
PhysicsEngineDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    std::unique_lock lock{ _lines_lock };
    if (_lines_count + 1 >= MAX_LINES)
        return;

    Line* line = static_cast<Line*>(_line_arena.alloc_align(sizeof(Line), alignof(Line)));
    JPH::Vec3(inFrom).StoreFloat3(&line->from);
    line->from_color = inColor;
    JPH::Vec3(inTo).StoreFloat3(&line->to);
    line->to_color = inColor;
    _lines_count += 1;
}

void
PhysicsEngineDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1,
                                         JPH::RVec3Arg inV2,
                                         JPH::RVec3Arg inV3,
                                         JPH::ColorArg inColor,
                                         ECastShadow inCastShadow)
{
    std::unique_lock lock{ _yftris_lock };
    if (_yftriangles >= MAX_YF_TRIANGLES)
        return;

    using namespace xray::rendering;
    using namespace xray::math;
    vertex_pc* verts = static_cast<vertex_pc*>(_yftris_arena.alloc_align(3 * sizeof(vertex_pc), alignof(vertex_pc)));
    for (const JPH::RVec3Arg v : { inV1, inV2, inV3 }) {
        verts->position = vec3f{ inV1.GetX(), inV1.GetY(), inV2.GetZ() };
        verts->color = rgb_color{ inColor.r, inColor.g, inColor.b, inColor.a };
        ++verts;
    }
    _yftriangles += 1;
}

void
PhysicsEngineDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition,
                                       const std::string_view& inString,
                                       JPH::ColorArg inColor,
                                       float inHeight)
{
    XR_LOG_INFO("{}", XRAY_FUNCTION_NAME);
}

PhysicsEngineDebugRenderer::Batch
PhysicsEngineDebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
{
    XR_LOG_INFO("{} triangles {}", XRAY_FUNCTION_NAME, inTriangleCount);
    BatchImpl* batch = new BatchImpl;
    if (inTriangles == nullptr || inTriangleCount == 0)
        return batch;

    batch->mTriangles.assign(inTriangles, inTriangles + inTriangleCount);
    return batch;
}

PhysicsEngineDebugRenderer::Batch
PhysicsEngineDebugRenderer::CreateTriangleBatch(const Vertex* inVertices,
                                                int inVertexCount,
                                                const JPH::uint32* inIndices,
                                                int inIndexCount)
{
    XR_LOG_INFO("{} vertices {}, indices {}", XRAY_FUNCTION_NAME, inVertexCount, inIndexCount);
    BatchImpl* batch = new BatchImpl;
    if (inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0)
        return batch;

    // Convert indexed triangle list to triangle list
    batch->mTriangles.resize(inIndexCount / 3);
    for (size_t t = 0; t < batch->mTriangles.size(); ++t) {
        Triangle& triangle = batch->mTriangles[t];
        triangle.mV[0] = inVertices[inIndices[t * 3 + 0]];
        triangle.mV[1] = inVertices[inIndices[t * 3 + 1]];
        triangle.mV[2] = inVertices[inIndices[t * 3 + 2]];
    }

    return batch;
}

void
PhysicsEngineDebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix,
                                         const JPH::AABox& inWorldSpaceBounds,
                                         float inLODScaleSq,
                                         JPH::ColorArg inModelColor,
                                         const GeometryRef& inGeometry,
                                         ECullMode inCullMode,
                                         ECastShadow inCastShadow,
                                         EDrawMode inDrawMode)
{
    XR_LOG_INFO("{}", XRAY_FUNCTION_NAME);

    using namespace JPH;
    using namespace xray::rendering;
    // Figure out which LOD to use
    const LOD* lod = inGeometry->mLODs.data();
    lod = &inGeometry->GetLOD(Vec3(_cam_pos), inWorldSpaceBounds, inLODScaleSq);

    // Draw the batch
    const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());

    if (inDrawMode == EDrawMode::Wireframe || inDrawMode == EDrawMode::Solid) {
        std::unique_lock lock{ _yftris_lock };
        const size_t batch_size = batch->mTriangles.size() * 3;
        if (_yftriangles + batch_size >= MAX_YF_TRIANGLES) {
            //
            // discard
            return;
        }
        vertex_pc* verts =
            static_cast<vertex_pc*>(_yftris_arena.alloc_align(batch_size * sizeof(vertex_pc), alignof(vertex_pc)));

        for (const Triangle& triangle : batch->mTriangles) {
            Color color = inModelColor * triangle.mV[0].mColor;

            for (size_t i = 0; i < 3; ++i) {
                const RVec3 v = inModelMatrix * Vec3(triangle.mV[i].mPosition);
                verts->position.x = v.GetX();
                verts->position.y = v.GetY();
                verts->position.z = v.GetZ();

                verts->color.r = color.r;
                verts->color.g = color.g;
                verts->color.b = color.b;
                verts->color.a = color.a;

                ++verts;
            }
            ++_yftriangles;
        }

        return;
    }

    if (inDrawMode == EDrawMode::Solid) {
    }

    // case EDrawMode::Solid:
    // DrawTriangle(v0, v1, v2, color, inCastShadow);
}

xray::base::unique_pointer<PhysicsEngineDebugRenderer>
PhysicsEngineDebugRenderer::create(const InitContext& ctx)
{
    constexpr const std::string_view line_shader_template = R"#(
    #version 460 core
    #include "core/bindless.core.glsl"

    #if defined(__VS__)
    layout (location=0) in vec3 vsInPos;
    layout (location=1) in vec4 vsInColor;
    out 
    #else
    in
    #endif

    VS_OUT_FS_IN {
        layout(location = 0) vec4 color;
    } vsout_fsin;

    #if !defined(__VS__)
    layout (location = 0) out vec4 FragColor;
    #endif

    void main() {
    #if defined(__VS__)
        #include "core/unpack.frame.glsl"
        gl_Position = fgd.world_view_proj * vec4(vsInPos, 1.0);
        vsout_fsin.color = vsInColor;
    #else
        FragColor = vsout_fsin.color;
    #endif
    }
    )#";

    using namespace xray::rendering;
    using namespace xray::base;

    ScratchPadArena scratchpad{ ctx.perm };
    ShaderCodeBuilder vs_code{ ctx.perm };

    const std::pair<containers::string, containers::string> vs_defs{
        containers::string{ "__VS__", *ctx.temp },
        containers::string{ *ctx.temp },
    };

    auto lines_pp = GraphicsPipelineBuilder{ ctx.perm, ctx.temp }
                        .add_shader(ShaderStage::Vertex,
                                    ShaderBuildOptions{
                                        .code_or_file_path = line_shader_template,
                                        .entry_point = "main",
                                        .defines = to_cspan(vs_defs),
                                    })
                        .add_shader(ShaderStage::Fragment,
                                    ShaderBuildOptions{
                                        .code_or_file_path = line_shader_template,
                                        .entry_point = "main",
                                    })
                        .input_assembly_state(InputAssemblyState{ .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST })
                        .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
                        .rasterization_state({
                            .poly_mode = VK_POLYGON_MODE_FILL,
                            .cull_mode = VK_CULL_MODE_BACK_BIT,
                            .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                            .line_width = 1.0f,
                        })
                        .create_bindless(*ctx.renderer);

    if (!lines_pp)
        return nullptr;

    //
    // TODO: portability
    const size_t block_size = (MAX_LINES + 1) * sizeof(Line) + (MAX_YF_TRIANGLES + 1) * sizeof(Line);
    std::byte* lines_mem = static_cast<std::byte*>(
        mmap(nullptr, static_cast<int>(block_size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    if (!lines_mem)
        return nullptr;

    auto gpu_lines = VulkanBuffer::create(*ctx.renderer,
                                          VulkanBufferCreateInfo{
                                              .name_tag = "[PhysDBG] lines",
                                              .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                              .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                              .bytes = MAX_LINES * sizeof(Line),
                                              .frames = ctx.renderer->max_inflight_frames(),
                                          });
    if (!gpu_lines)
        return nullptr;

    auto gpu_yftris = VulkanBuffer::create(*ctx.renderer,
                                           VulkanBufferCreateInfo{
                                               .name_tag = "[PhysDBG] yftris",
                                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                               .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                               .bytes = MAX_YF_TRIANGLES * sizeof(Line),
                                               .frames = ctx.renderer->max_inflight_frames(),
                                           });
    if (!gpu_yftris)
        return nullptr;
    auto yftris_pp = GraphicsPipelineBuilder{ ctx.perm, ctx.temp }
                         .add_shader(ShaderStage::Vertex,
                                     ShaderBuildOptions{
                                         .code_or_file_path = line_shader_template,
                                         .entry_point = "main",
                                         .defines = to_cspan(vs_defs),
                                     })
                         .add_shader(ShaderStage::Fragment,
                                     ShaderBuildOptions{
                                         .code_or_file_path = line_shader_template,
                                         .entry_point = "main",
                                     })
                         .input_assembly_state(InputAssemblyState{ .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST })
                         .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
                         .rasterization_state({
                             .poly_mode = VK_POLYGON_MODE_LINE,
                             .cull_mode = VK_CULL_MODE_BACK_BIT,
                             .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                             .line_width = 1.0f,
                         })
                         .create_bindless(*ctx.renderer);

    if (!yftris_pp)
        return nullptr;

    return xray::base::make_unique<PhysicsEngineDebugRenderer>(
        PhysDebugRenderResourcesBundle{
            .arena_mem = std::span{ lines_mem, MAX_LINES + sizeof(Line) },
            .gpu_buffer = std::move(*gpu_lines),
            .pipeline = std::move(*lines_pp),
        },
        PhysDebugRenderResourcesBundle{
            .arena_mem = std::span{ lines_mem + MAX_LINES + 1, MAX_YF_TRIANGLES },
            .gpu_buffer = std::move(*gpu_yftris),
            .pipeline = std::move(*yftris_pp),
        });
}

void
PhysicsEngineDebugRenderer::draw(const RenderEvent& e)
{
    using namespace xray::rendering;

    e.renderer->dbg_marker_begin(e.frame_data->cmd_buf, "Physics debug draw", rgb_color{ 0.0f, 1.0f, 0.0f });

    const PushConstantPacker push_const{ e.frame_data->id };

    uint32_t lines_count{};
    {
        std::unique_lock lock{ _lines_lock };
        if (_lines_count != 0) {
            _gpu_lines.mmap(e.renderer->device(), e.frame_data->id)
                .map([buf = _line_arena.buf, line_count = _lines_count](UniqueMemoryMapping mapped_buffer) {
                    std::memcpy(mapped_buffer._mapped_memory,
                                (const void*)xray::base::align_forward(reinterpret_cast<uintptr_t>(buf), alignof(Line)),
                                line_count * sizeof(Line));
                });

            lines_count = _lines_count;
            _lines_count = 0;
            _line_arena.free_all();
        }
    }
    if (lines_count) {
        // vkCmdBindPipeline(e.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _lines_pp.handle());
        // vkCmdDraw(e.frame_data->cmd_buf, lines_count * 2, 1, 0, 0);
    }

    uint32_t yftriangles_to_draw{};
    {
        std::unique_lock lock{ _yftris_lock };
        if (_yftriangles != 0) {
            _gpu_yftris.mmap(e.renderer->device(), e.frame_data->id)
                .map([buf = _yftris_arena.buf, tris_count = _yftriangles](UniqueMemoryMapping mapped_buffer) {
                    std::memcpy(mapped_buffer._mapped_memory,
                                reinterpret_cast<const void*>(
                                    xray::base::align_forward(reinterpret_cast<uintptr_t>(buf), alignof(vertex_pc))),
                                tris_count * 3 * sizeof(vertex_pc));
                });

            yftriangles_to_draw = _yftriangles;
            _yftriangles = 0;
            _yftris_arena.free_all();
        }
    }
    XR_LOG_INFO("Triangle count {}", yftriangles_to_draw);
    if (yftriangles_to_draw) {
        const VkBuffer yftriangle_buffer = _gpu_yftris.buffer_handle();
        const VkDeviceSize offsets{};
        vkCmdBindVertexBuffers(e.frame_data->cmd_buf, 0, 1, &yftriangle_buffer, &offsets);
        vkCmdBindPipeline(e.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _yftris_pp.handle());
        vkCmdPushConstants(
            e.frame_data->cmd_buf, _yftris_pp.layout(), VK_SHADER_STAGE_ALL, 0, 4, push_const.as_bytes().data());
        vkCmdDraw(e.frame_data->cmd_buf, yftriangles_to_draw * 3, 1, 0, 0);
    }

    e.renderer->dbg_marker_end(e.frame_data->cmd_buf);
}

}

#endif
