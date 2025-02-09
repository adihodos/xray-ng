#include "system.physics.debug.renderer.hpp"
#include "vulkan.renderer/vulkan.unique.resource.hpp"
#include <type_traits>

#if defined(JPH_DEBUG_RENDERER)

#include <xray/xray.hpp>
#include <xray/base/logger.hpp>
#include <xray/base/scoped_guard.hpp>
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
                                                       PhysDebugRenderResourcesBundle res_yftris,
                                                       PhysDebugRenderResourcesBundle fill_tris)
    : _line_arena{ res_lines.arena_mem }
    , _gpu_lines{ std::move(res_lines.gpu_buffer) }
    , _lines_pp{ std::move(res_lines.pipeline) }
    , _yftris_arena{ res_yftris.arena_mem }
    , _gpu_yftris{ std::move(res_yftris.gpu_buffer) }
    , _yftris_pp{ std::move(res_yftris.pipeline) }
    , _filled_tris_arena{ std::move(fill_tris.arena_mem) }
    , _gpu_filled_tris{ std::move(fill_tris.gpu_buffer) }
    , _filled_tris_pp{ std::move(fill_tris.pipeline) }
{
    DebugRenderer::Initialize();
}

void
PhysicsEngineDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
    XR_LOG_INFO("Draw line");
    std::unique_lock lock{ _lines_lock };
    if (_lines_count + 2 >= MAX_LINES)
        return;

    Line* line = static_cast<Line*>(_line_arena.alloc_align(sizeof(Line), alignof(Line)));
    JPH::Vec3(inFrom).StoreFloat3(&line->from);
    line->from_color = inColor;
    JPH::Vec3(inTo).StoreFloat3(&line->to);
    line->to_color = inColor;
    _lines_count += 2;
}

void
PhysicsEngineDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1,
                                         JPH::RVec3Arg inV2,
                                         JPH::RVec3Arg inV3,
                                         JPH::ColorArg inColor,
                                         ECastShadow inCastShadow)
{
    XR_LOG_INFO("{} is NOT IMPLEMENTED YET!", XRAY_FUNCTION_NAME);
    std::unique_lock lock{ _yftris_lock };
    if (_yftriangles + 3 >= MAX_YF_TRIANGLES)
        return;

    using namespace xray::rendering;
    using namespace xray::math;
    vertex_pc* verts = static_cast<vertex_pc*>(_yftris_arena.alloc_align(3 * sizeof(vertex_pc), alignof(vertex_pc)));
    for (const JPH::RVec3Arg v : { inV1, inV2, inV3 }) {
        verts->position = vec3f{ inV1.GetX(), inV1.GetY(), inV2.GetZ() };
        verts->color = rgb_color{ inColor.r, inColor.g, inColor.b, inColor.a };
        ++verts;
    }
    _yftriangles += 3;
}

void
PhysicsEngineDebugRenderer::DrawText3D(JPH::RVec3Arg inPosition,
                                       const std::string_view& inString,
                                       JPH::ColorArg inColor,
                                       float inHeight)
{
    XR_LOG_INFO("{} is NOT IMPLEMENTED YET!", XRAY_FUNCTION_NAME);
}

PhysicsEngineDebugRenderer::Batch
PhysicsEngineDebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount)
{
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

    //
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
    using namespace JPH;
    using namespace xray::rendering;
    //
    // Figure out which LOD to use
    const LOD* lod = inGeometry->mLODs.data();
    lod = &inGeometry->GetLOD(Vec3(_cam_pos), inWorldSpaceBounds, inLODScaleSq);

    //
    // Draw the batch
    const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());

    auto push_triangle_batch_fn = [batch, &inModelColor, &inModelMatrix](std::mutex& triangle_lock,
                                                                         size_t& triangle_count,
                                                                         xray::base::MemoryArena& triangle_arena) {
        std::unique_lock lock{ triangle_lock };
        const size_t batch_size = batch->mTriangles.size() * 3;
        if (triangle_count + batch_size >= MAX_YF_TRIANGLES) {
            //
            // discard
            return;
        }
        vertex_pc* verts =
            static_cast<vertex_pc*>(triangle_arena.alloc_align(batch_size * sizeof(vertex_pc), alignof(vertex_pc)));

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
            triangle_count += 3;
        }
    };

    if (inDrawMode == EDrawMode::Wireframe) {
        push_triangle_batch_fn(_yftris_lock, _yftriangles, _yftris_arena);
    } else {
        push_triangle_batch_fn(_fill_tris_lock, _filled_tris, _filled_tris_arena);
    }
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
    const size_t block_size = (MAX_LINES + 1) * sizeof(Line) + (MAX_YF_TRIANGLES + 1) * 2 * sizeof(Line);
    XR_LOG_INFO("Allocating {} Kb/{} Mb for lines and triangles", block_size / 1024, block_size / (1024 * 1024));
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

    auto gpu_filled = VulkanBuffer::create(*ctx.renderer,
                                           VulkanBufferCreateInfo{
                                               .name_tag = "[PhysDBG] filled triagles",
                                               .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                               .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                               .bytes = MAX_YF_TRIANGLES * sizeof(Line),
                                               .frames = ctx.renderer->max_inflight_frames(),
                                           });
    if (!gpu_filled)
        return nullptr;

    auto filled_pp = GraphicsPipelineBuilder{ ctx.perm, ctx.temp }
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
                             .poly_mode = VK_POLYGON_MODE_FILL,
                             .cull_mode = VK_CULL_MODE_BACK_BIT,
                             .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                             .line_width = 1.0f,
                         })
                         .create_bindless(*ctx.renderer);

    if (!filled_pp)
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
        },
        PhysDebugRenderResourcesBundle{
            .arena_mem = std::span{ lines_mem + MAX_LINES + 1 + MAX_YF_TRIANGLES + 1, MAX_YF_TRIANGLES },
            .gpu_buffer = std::move(*gpu_filled),
            .pipeline = std::move(*filled_pp),
        });
}

template<typename PrimitiveType>
void
draw_primitives(const RenderEvent& e,
                std::mutex& primitive_lock,
                size_t& primitive_count,
                const xray::rendering::VulkanBuffer& gpu_buffer,
                xray::base::MemoryArena& arena,
                const xray::rendering::GraphicsPipeline& effect)
{
    auto [primitive_align, primitive_size] = []() constexpr -> std::pair<size_t, size_t> {
        if constexpr (std::is_same_v<PrimitiveType, PhysicsEngineDebugRenderer::Line>) {
            return std::pair<size_t, size_t>{ alignof(PhysicsEngineDebugRenderer::Line),
                                              sizeof(PhysicsEngineDebugRenderer::Line) };
        } else if constexpr (std::is_same_v<PrimitiveType, xray::rendering::vertex_pc>) {
            return std::pair<size_t, size_t>{ alignof(xray::rendering::vertex_pc), sizeof(xray::rendering::vertex_pc) };
        } else {
            static_assert(false, "unsupported type");
        }
    }();

    uint32_t draw_count{};
    {
        std::unique_lock lock{ primitive_lock };
        if (primitive_count != 0) {
            gpu_buffer.mmap(e.renderer->device(), e.frame_data->id)
                .map([buf = arena.buf, primitive_count, primitive_align, primitive_size](
                         xray::rendering::UniqueMemoryMapping mapped_buffer) {
                    std::memcpy(mapped_buffer._mapped_memory,
                                reinterpret_cast<const void*>(
                                    xray::base::align_forward(reinterpret_cast<uintptr_t>(buf), primitive_align)),
                                primitive_count * primitive_size);
                });

            draw_count = primitive_count;
            primitive_count = 0;
            arena.free_all();
        }
    }

    if (draw_count != 0) {
        const VkBuffer yftriangle_buffer = gpu_buffer.buffer_handle();
        const VkDeviceSize offsets{};
        vkCmdBindVertexBuffers(e.frame_data->cmd_buf, 0, 1, &yftriangle_buffer, &offsets);
        vkCmdBindPipeline(e.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, effect.handle());
        vkCmdDraw(e.frame_data->cmd_buf, draw_count, 1, 0, 0);
    }
}

void
PhysicsEngineDebugRenderer::draw(const RenderEvent& e)
{
    using namespace xray::rendering;

    const auto _marker =
        e.renderer->dbg_marker_begin(e.frame_data->cmd_buf, "Physics debug draw", rgb_color{ 0.0f, 1.0f, 0.0f });
    const PackedU32PushConstant push_const{ e.frame_data->id };

    draw_primitives<Line>(e, _lines_lock, _lines_count, _gpu_lines, _line_arena, _lines_pp);
    draw_primitives<vertex_pc>(e, _yftris_lock, _yftriangles, _gpu_yftris, _yftris_arena, _yftris_pp);
    draw_primitives<vertex_pc>(e, _fill_tris_lock, _filled_tris, _gpu_filled_tris, _filled_tris_arena, _filled_tris_pp);
}

}

#endif
