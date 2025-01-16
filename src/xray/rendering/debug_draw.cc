#include "xray/rendering/debug_draw.hpp"

#include <algorithm>
#include <string_view>

#include <Lz/Lz.hpp>
#include <itlib/small_vector.hpp>
#include <tl/expected.hpp>

#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/vertex_format/vertex_pc.hpp"

#if defined(XRAY_GRAPHICS_API_VULKAN)

#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.pipeline.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"

#endif

#if defined(XRAY_RENDERER_OPENGL)

#include <vector>
#include "xray/rendering/geometry/surface_normal_visualizer.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"

#endif

namespace internal {

constexpr const char* const kVertexShaderCode = R"(
#version 450 core

layout(location = 0) in vec3 VsInPos;
layout(location = 1) in vec4 VsInColor;

layout(binding = 0, row_major) uniform Ubo {
  mat4 WorldViewProj;
};

out gl_PerVertex {
  vec4 gl_Position;
};

out VsOutFsIn {
  layout (location = 0) vec4 color;
} vsOut;

void main() {
  gl_Position = WorldViewProj * vec4(VsInPos, 1.0f);
  vsOut.color = VsInColor;
}

)";

constexpr const char* const kFragmentShaderCode = R"(
#version 450 core

in VsOutFsIn {
  layout (location = 0) vec4 color;
} fsIn;

layout(location = 0) out vec4 FinalFragColor;

void main() {
  FinalFragColor = fsIn.color;
}
)";

}

#if defined(XRAY_GRAPHICS_API_VULKAN)

namespace xray::rendering {

struct DebugDrawSystem::RenderStateVulkan
{
    VulkanBuffer mVertexBuffer;
    GraphicsPipeline mGraphicsPipeline;
    std::vector<vertex_pc*> mVertexMemoryMappings;
    VkDevice mDevice;
    vertex_pc* mBufferPtr{};

    RenderStateVulkan(VulkanBuffer&& vertexBuffer,
                      GraphicsPipeline&& graphicsPipeline,
                      std::vector<vertex_pc*>&& memoryMappings,
                      VkDevice device)
        : mVertexBuffer{ std::move(vertexBuffer) }
        , mGraphicsPipeline{ std::move(graphicsPipeline) }
        , mVertexMemoryMappings{ std::move(memoryMappings) }
        , mDevice{ device }
    {
    }

    ~RenderStateVulkan()
    {
        if (mVertexBuffer.memory_handle())
            vkUnmapMemory(mDevice, mVertexBuffer.memory_handle());
    }

    RenderStateVulkan(RenderStateVulkan&&) noexcept = default;
    static tl::expected<RenderStateVulkan, VulkanError> create(const DebugDrawSystem::InitContext& init);
};

tl::expected<DebugDrawSystem::RenderStateVulkan, VulkanError>
DebugDrawSystem::RenderStateVulkan::create(const DebugDrawSystem::InitContext& init)
{
    auto vertexBuffer = VulkanBuffer::create(*init.renderer,
                                             VulkanBufferCreateInfo{
                                                 .name_tag = "Debug draw vertex buffer",
                                                 .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                 .memory_properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                 .bytes = 8192 * sizeof(vertex_pc),
                                                 .frames = init.renderer->buffering_setup().buffers,
                                             });

    XR_VK_PROPAGATE_ERROR(vertexBuffer);

    constexpr const std::string_view kDebugVertexShader = R"#(
    #version 460 core
    #include "core/bindless.core.glsl"

    layout(location = 0) in vec3 VsInPos;
    layout(location = 1) in vec4 VsInColor;

    out gl_PerVertex {
      vec4 gl_Position;
    };

    out VsOutFsIn {
      layout (location = 0) vec4 color;
    } vsOut;

    void main() {
        const uint frame_idx = (g_GlobalPushConst.data) & 0xFF;
        const FrameGlobalData_t fgd = g_FrameGlobal[frame_idx].data[0];
        gl_Position = fgd.world_view_proj * vec4(VsInPos, 1.0f);
        vsOut.color = VsInColor;
    }
    )#";

    constexpr const std::string_view kDebugFragmentShaderCode = R"#(
    #version 460 core
    #include "core/bindless.core.glsl"

    in VsOutFsIn {
        layout (location = 0) vec4 color;
    } fsIn;

    layout(location = 0) out vec4 FinalFragColor;

    void main() {
        FinalFragColor = fsIn.color;
    }
    )#";

    auto graphicsPipeline = GraphicsPipelineBuilder{}
                                .add_shader(ShaderStage::Vertex,
                                            ShaderBuildOptions{
                                                .code_or_file_path = kDebugVertexShader,
                                            })
                                .add_shader(ShaderStage::Fragment,
                                            ShaderBuildOptions{
                                                .code_or_file_path = kDebugFragmentShaderCode,
                                            })
                                .input_assembly_state(InputAssemblyState{ .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST })
                                .dynamic_state({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
                                .create_bindless(*init.renderer);

    XR_VK_PROPAGATE_ERROR(graphicsPipeline);

    void* ptr_mem{};
    const VkResult mmap_result = WRAP_VULKAN_FUNC(
        vkMapMemory, init.renderer->device(), vertexBuffer->memory_handle(), 0, VK_WHOLE_SIZE, 0, &ptr_mem);
    XR_VK_CHECK_RESULT(mmap_result);

    std::vector<vertex_pc*> vertexMemoryMappings{};
    for (uint32_t idx = 0, count = init.renderer->buffering_setup().buffers; idx < count; ++idx) {
        vertexMemoryMappings.push_back((vertex_pc*)((uint8_t*)ptr_mem + idx * vertexBuffer->aligned_size));
    }

    return tl::expected<RenderStateVulkan, VulkanError>{
        tl::in_place,
        std::move(*vertexBuffer),
        std::move(*graphicsPipeline),
        std::move(vertexMemoryMappings),
        init.renderer->device(),
    };
}

}

#endif

static constexpr const xray::math::vec2f kSinCosTable[] = {
    { 0.0000, 1.0000 },   { 0.1951, 0.9808 },   { 0.3827, 0.9239 },   { 0.5556, 0.8315 },   { 0.7071, 0.7071 },
    { 0.8315, 0.5556 },   { 0.9239, 0.3827 },   { 0.9808, 0.1951 },   { 1.0000, 0.0000 },   { 0.9808, -0.1951 },
    { 0.9239, -0.3827 },  { 0.8315, -0.5556 },  { 0.7071, -0.7071 },  { 0.5556, -0.8315 },  { 0.3827, -0.9239 },
    { 0.1951, -0.9808 },  { 0.0000, -1.0000 },  { -0.1951, -0.9808 }, { -0.3827, -0.9239 }, { -0.5556, -0.8315 },
    { -0.7071, -0.7071 }, { -0.8315, -0.5556 }, { -0.9239, -0.3827 }, { -0.9808, -0.1951 }, { -1.0000, -0.0000 },
    { -0.9808, 0.1951 },  { -0.9239, 0.3827 },  { -0.8315, 0.5556 },  { -0.7071, 0.7071 },  { -0.5556, 0.8315 },
    { -0.3827, 0.9239 },  { -0.1951, 0.9808 },
};

static constexpr const uint32_t kSteps{ 32 };

namespace xray::rendering {

void
DebugDrawSystem::RenderStateDeleter::operator()(DebugDrawSystem::RenderState* r) const noexcept
{
    delete r;
}

DebugDrawSystem::DebugDrawSystem(PrivateConstructionToken,
                                 std::unique_ptr<RenderState, RenderStateDeleter>&& render_state)
    : mRenderState{ std::move(render_state) }
{
}

DebugDrawSystem::~DebugDrawSystem() {}

void
DebugDrawSystem::draw_line(const math::vec3f& from, const math::vec3f& to, const rgb_color& color)
{
    *mRenderState->mBufferPtr++ = vertex_pc{ from, color };
    *mRenderState->mBufferPtr++ = vertex_pc{ to, color };
}

void
DebugDrawSystem::draw_box(const std::span<const math::vec3f> points, const rgb_color& c)
{
    assert(points.size() == 8);
    constexpr const uint8_t indices[] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };
    static_assert(std::size(indices) % 2 == 0);
    for (auto&& primitive : lz::chunks(indices, 2)) {
        const auto pts = primitive.toArray<2>();
        draw_line(points[pts[0]], points[pts[1]], c);
    }
}

void
DebugDrawSystem::draw_frustrum(const math::MatrixWithInvertedMatrixPair4f& mtx, const rgb_color& color)
{
    // NDC coords near + far
    constexpr const math::vec3f planes_points[] = {
        // near plane
        { -1.0f, -1.0f, -1.0f },
        { 1.0f, -1.0f, -1.0f },
        { 1.0f, 1.0f, -1.0f },
        { -1.0f, 1.0f, -1.0f },
        // far plane
        { -1.0f, -1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f },
    };

    // NDC -> view space
    itlib::small_vector<math::vec3f, 8> points{};
    for (const math::vec3f& p : planes_points) {
        const math::vec4f unprojected = math::mul_point(mtx.inverted, math::vec4f{ p });
        if (std::fabs(p.w) < 1.0e-5) {
            continue;
        }
        points.push_back(math::vec3f{ p.x / p.w, p.y / p.w, p.z / p.w });
    }

    if (points.size() == 8)
        draw_box(std::span{ points }, color);
}

void
DebugDrawSystem::draw_oriented_box(const math::vec3f& org,
                                   const math::vec3f& u,
                                   const math::vec3f& v,
                                   const math::vec3f& w,
                                   const float lr,
                                   const float lu,
                                   const float ld,
                                   const rgb_color& c)
{
    const float eu{ lr * 0.5f };
    const float ev{ lu * 0.5f };
    const float ew{ ld * 0.5f };

    const math::vec3f kVertices[] = {
        org + u * eu - v * ev - w * ew, org + u * eu + v * ev - w * ew,
        org - u * eu + v * ev - w * ew, org - u * eu - v * ev - w * ew,

        org + u * eu - v * ev + w * ew, org + u * eu + v * ev + w * ew,
        org - u * eu + v * ev + w * ew, org - u * eu - v * ev + w * ew,
    };

    const uint32_t kIndices[] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

    for (const uint32_t i : kIndices) {
        *mRenderState->mBufferPtr++ = { kVertices[i], c };
    }

    draw_cross(org, u, v, w, std::min({ eu, ev, ew }) * 0.25f, c);
}

void
DebugDrawSystem::draw_axis_aligned_box(const math::vec3f& minpoint, const math::vec3f& maxpoint, const rgb_color& c)
{
    const math::vec3f center = (maxpoint + minpoint) * 0.5f;
    const math::vec3f extent{ maxpoint - minpoint };
    draw_oriented_box(center,
                      math::vec3f::stdc::unit_x,
                      math::vec3f::stdc::unit_y,
                      math::vec3f::stdc::unit_z,
                      std::abs(extent.x),
                      std::abs(extent.y),
                      std::abs(extent.z),
                      c);
}

void
DebugDrawSystem::draw_cross(const math::vec3f& o,
                            const math::vec3f& u,
                            const math::vec3f& v,
                            const math::vec3f& w,
                            const float ext,
                            const rgb_color& color)
{
    const float s{ ext * 0.5f };
    *mRenderState->mBufferPtr++ = { o + u * s, color };
    *mRenderState->mBufferPtr++ = { o - u * s, color };
    *mRenderState->mBufferPtr++ = { o + v * s, color };
    *mRenderState->mBufferPtr++ = { o - v * s, color };
    *mRenderState->mBufferPtr++ = { o + w * s, color };
    *mRenderState->mBufferPtr++ = { o - w * s, color };
}

void
DebugDrawSystem::draw_coord_sys(const math::vec3f& o,
                                const math::vec3f& u,
                                const math::vec3f& v,
                                const math::vec3f& w,
                                const float ext,
                                const rgb_color& cu,
                                const rgb_color& cv,
                                const rgb_color& cw)
{
    draw_arrow(o, o + u * ext, cu);
    draw_arrow(o, o + v * ext, cv);
    draw_arrow(o, o + w * ext, cw);
}

void
DebugDrawSystem::draw_grid(const math::vec3f origin,
                           const math::vec3f x_axis,
                           const math::vec3f z_axis,
                           const size_t x_divs,
                           const size_t z_divs,
                           const float cell_size,
                           const rgb_color color)
{
    for (size_t x = 0; x <= x_divs; ++x) {
        float k = static_cast<float>(x) / static_cast<float>(x_divs);
        // align into [-1, +1]
        k = (k * 2.0f) - 1.0f;
        const math::vec3f scale = k * x_axis + origin;

        draw_line((scale + z_axis) * cell_size, (scale - z_axis) * cell_size, color);
    }

    for (size_t z = 0; z <= z_divs; ++z) {
        float k = static_cast<float>(z) / static_cast<float>(z_divs);
        // align into [-1, +1]
        k = (k * 2.0f) - 1.0f;
        const math::vec3f scale = k * z_axis + origin;

        draw_line((scale + x_axis) * cell_size, (scale - x_axis) * cell_size, color);
    }
}

void
DebugDrawSystem::draw_arrow(const math::vec3f& from, const math::vec3f& to, const rgb_color& c)
{
    const math::vec3f lineDir{ to - from };
    const float arrowLenSq{ math::length_squared(lineDir) };

    if (arrowLenSq < 1.0f) {
        draw_line(from, to, c);
        return;
    }

    draw_line(from, to, c);

    const float arrowLen{ std::sqrt(arrowLenSq) };
    const float adjustedLen = arrowLen * 0.98f;
    const math::vec3f dvec{ lineDir / arrowLen };

    const auto [u, v, w] = math::make_frame_vectors(dvec);
    const float capLen{ (arrowLen - adjustedLen) * 0.5f };
    const math::vec3f kVertices[] = {
        // tip
        to,
        from + u * adjustedLen + v * capLen + w * capLen,
        from + u * adjustedLen - v * capLen + w * capLen,
        from + u * adjustedLen - v * capLen - w * capLen,
        from + u * adjustedLen + v * capLen - w * capLen,
    };

    for (const uint32_t idx : { 1, 2, 2, 3, 3, 4, 4, 1, 0, 1, 0, 2, 0, 3, 0, 4, 2, 4, 1, 3 }) {
        *mRenderState->mBufferPtr++ = { kVertices[idx], c };
    }
}

void
DebugDrawSystem::draw_directional_light(const math::vec3f& dir, const float origin_dist, const rgb_color& c)
{
    auto [u, v, w] = math::make_frame_vectors(dir);
    constexpr const float kBoxSize{ 0.5f };
    const math::vec3f origin = dir * (-origin_dist);
    draw_oriented_box(origin, u, v, w, kBoxSize, kBoxSize, kBoxSize, c);
    draw_arrow(origin, origin + dir * 4.0f, c);
}

void
DebugDrawSystem::draw_plane(const math::vec3f& origin, const math::vec3f& normal, const float scale, const rgb_color& c)
{
    const auto [u, v, w] = math::make_frame_vectors(normal);
    const math::vec3f kVertices[] = {
        origin + v * scale * 0.5f + w * scale * 0.5f,
        origin - v * scale * 0.5f + w * scale * 0.5f,
        origin - v * scale * 0.5f - w * scale * 0.5f,
        origin + v * scale * 0.5f - w * scale * 0.5f,
    };

    for (const uint32_t idx : { 0, 1, 1, 2, 2, 3, 3, 0 }) {
        *mRenderState->mBufferPtr++ = vertex_pc{ kVertices[idx], c };
    }

    draw_arrow(origin, origin + normal * scale * 0.5f, c);
}

void
DebugDrawSystem::draw_circle(const math::vec3f& origin,
                             const math::vec3f& normal,
                             const float radius,
                             const rgb_color& c,
                             const uint32_t draw_options)
{
    using namespace detail;

    const auto [u, v, w] = math::make_frame_vectors(normal);

    for (const uint32_t idx : lz::range(kSteps)) {
        const math::vec3f v0 = origin + v * kSinCosTable[idx].s * radius + w * kSinCosTable[idx].t * radius;
        const math::vec3f v1 =
            origin + v * kSinCosTable[(idx + 1) % kSteps].s * radius + w * kSinCosTable[(idx + 1) % kSteps].t * radius;

        if (draw_options & Draw_CircleSegments)
            draw_line(origin, v0, c);
        draw_line(v0, v1, c);
    }

    if (draw_options & Draw_CircleNormal)
        draw_arrow(origin, origin + normal * radius * 0.5f, c);
}

void
DebugDrawSystem::draw_sphere(const math::vec3f& origin, const float radius, const rgb_color& color)
{
    draw_circle(origin, math::vec3f::stdc::unit_x, radius, color, Draw_NoOptions);
    draw_circle(origin, math::vec3f::stdc::unit_y, radius, color, Draw_NoOptions);
    draw_circle(origin, math::vec3f::stdc::unit_z, radius, color, Draw_NoOptions);
}

void
DebugDrawSystem::draw_cone(const math::vec3f origin,
                           const math::vec3f& normal,
                           const float height,
                           const float base,
                           const float apex,
                           const rgb_color& c)
{
    using namespace detail;

    const auto [u, v, w] = math::make_frame_vectors(normal);
    if (math::is_zero(apex)) {
        for (const uint32_t idx : lz::range(kSteps)) {
            const auto radius = base;
            const math::vec3f v0 = origin + v * kSinCosTable[idx].s * radius + w * kSinCosTable[idx].t * radius;
            const math::vec3f v1 = origin + v * kSinCosTable[(idx + 1) % kSteps].s * radius +
                                   w * kSinCosTable[(idx + 1) % kSteps].t * radius;

            draw_line(origin, v0, c);
            draw_line(v0, v1, c);
            draw_line(origin + normal * height, v0, c);
            draw_line(origin + normal * height, v1, c);
        }
    } else {
        for (const uint32_t idx : lz::range(kSteps)) {
            const auto apex_org = origin + normal * height;
            const auto radius = base;
            const auto apex_r = apex;

            const math::vec3f v0 = origin + v * kSinCosTable[idx].s * radius + w * kSinCosTable[idx].t * radius;
            const math::vec3f v1 = origin + v * kSinCosTable[(idx + 1) % kSteps].s * radius +
                                   w * kSinCosTable[(idx + 1) % kSteps].t * radius;

            draw_line(origin, v0, c);
            draw_line(v0, v1, c);

            // apex
            const math::vec3f apxv0 = apex_org + v * kSinCosTable[idx].s * apex_r + w * kSinCosTable[idx].t * apex_r;
            const math::vec3f apxv1 = apex_org + v * kSinCosTable[(idx + 1) % kSteps].s * apex_r +
                                      w * kSinCosTable[(idx + 1) % kSteps].t * apex_r;

            draw_line(apex_org, apxv0, c);
            draw_line(apxv0, apxv1, c);

            // lines from apex to base
            draw_line(v0, apxv0, c);
        }
    }
}

void
DebugDrawSystem::draw_vector(const math::vec3f& org, const math::vec3f& dir, const float length, const rgb_color& c)
{
    draw_arrow(org, org + dir * length, c);
}

tl::expected<DebugDrawSystem, DebugDrawSystem::ErrorType>
DebugDrawSystem::create(const InitContext& init)
{
#if defined(XRAY_GRAPHICS_API_VULKAN)
    auto render_state = RenderStateVulkan::create(init);
    XR_VK_PROPAGATE_ERROR(render_state);

    return tl::expected<DebugDrawSystem, ErrorType>(
        tl::in_place,
        PrivateConstructionToken{},
        std::unique_ptr<RenderStateVulkan, RenderStateDeleter>(new RenderStateVulkan{ std::move(*render_state) }));
#endif

    // scoped_buffer vertexBuffer{ []() {
    //     GLuint buffHandle{};
    //     gl::CreateBuffers(1, &buffHandle);
    //     gl::NamedBufferStorage(buffHandle, 8192 * sizeof(vertex_pc), nullptr, gl::MAP_WRITE_BIT);
    //
    //     return buffHandle;
    // }() };
    //
    // scoped_vertex_array vertexArray{ [vb = base::raw_handle(vertexBuffer)]() {
    //     GLuint vaoObj{};
    //     gl::CreateVertexArrays(1, &vaoObj);
    //     gl::VertexArrayVertexBuffer(vaoObj, 0, vb, 0, sizeof(vertex_pc));
    //
    //     gl::VertexArrayAttribFormat(vaoObj, 0, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pc, position));
    //     gl::EnableVertexArrayAttrib(vaoObj, 0);
    //     gl::VertexArrayAttribBinding(vaoObj, 0, 0);
    //
    //     gl::VertexArrayAttribFormat(vaoObj, 1, 4, gl::FLOAT, gl::FALSE_, offsetof(vertex_pc, color));
    //     gl::EnableVertexArrayAttrib(vaoObj, 1);
    //     gl::VertexArrayAttribBinding(vaoObj, 1, 0);
    //
    //     return vaoObj;
    // }() };
    //
    // vertex_program vertexShader =
    //     gpu_program_builder{}.add_string(internal::kVertexShaderCode).build<render_stage::e::vertex>();
    // if (!vertexShader)
    //     return tl::nullopt;
    //
    // fragment_program fragmentShader =
    //     gpu_program_builder{}.add_string(internal::kFragmentShaderCode).build<render_stage::e::fragment>();
    // if (!fragmentShader)
    //     return tl::nullopt;
    //
    // program_pipeline graphicsPipeline{ []() {
    //     GLuint pp{};
    //     gl::CreateProgramPipelines(1, &pp);
    //     return pp;
    // }() };
    //
    // graphicsPipeline.use_vertex_program(vertexShader).use_fragment_program(fragmentShader);
    //
    // using std::move;
    // return tl::optional<RenderDebugDraw>{ RenderDebugDraw{ std::move(vertexBuffer),
    //                                                        std::move(vertexArray),
    //                                                        std::move(vertexShader),
    //                                                        std::move(fragmentShader),
    //                                                        std::move(graphicsPipeline) } };
}

void
DebugDrawSystem::new_frame(const uint32_t frame_id) noexcept
{
#if defined(XRAY_GRAPHICS_API_VULKAN)
    mRenderState->mBufferPtr = mRenderState->mVertexMemoryMappings[frame_id];
#endif
}

void
DebugDrawSystem::render(const DebugDrawSystem::RenderContext& rc) noexcept
{
#if defined(XRAY_GRAPHICS_API_VULKAN)
    const uint32_t vertexDrawCount =
        static_cast<uint32_t>(mRenderState->mBufferPtr - mRenderState->mVertexMemoryMappings[rc.frd->id]);
    if (vertexDrawCount == 0) {
        return;
    }

    vkCmdBindPipeline(rc.frd->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderState->mGraphicsPipeline.handle());

    const VkDeviceSize offsets[] = { 0 };
    const VkBuffer buffers[] = { mRenderState->mVertexBuffer.buffer_handle() };
    vkCmdBindVertexBuffers(rc.frd->cmd_buf, 0, 1, buffers, offsets);

    vkCmdPushConstants(rc.frd->cmd_buf,
                       mRenderState->mGraphicsPipeline.layout(),
                       VK_SHADER_STAGE_ALL,
                       0,
                       sizeof(uint32_t),
                       &rc.frd->id);

    vkCmdDraw(rc.frd->cmd_buf, vertexDrawCount, 1, 0, 0);
#endif

    // if (mRenderState.mVertices.empty())
    //     return;
    //
    // scoped_resource_mapping::map(mRenderState.mVertexBuffer,
    //                              gl::MAP_WRITE_BIT | gl::MAP_INVALIDATE_BUFFER_BIT,
    //                              base::container_bytes_size(mRenderState.mVertices))
    //     .map([vtx = &mRenderState.mVertices](scoped_resource_mapping mappedBuffer) {
    //         memcpy(mappedBuffer.memory(), vtx->data(), base::container_bytes_size(*vtx));
    //     });
    //
    // mRenderState.mGraphicsPipeline.use(false);
    // gl::BindVertexArray(base::raw_handle(mRenderState.mVertexArrayObj));
    // mRenderState.mVertexShader.set_uniform_block("Ubo", worldViewProj);
    // mRenderState.mVertexShader.flush_uniforms();
    //
    // gl::DrawArrays(gl::LINES, 0, static_cast<GLsizei>(mRenderState.mVertices.size()));
    // mRenderState.mVertices.clear();
}

}
