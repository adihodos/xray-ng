#include "xray/rendering/debug_draw.hpp"

#include <algorithm>

#include <Lz/Lz.hpp>
#include <itlib/small_vector.hpp>

#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"

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
  vec4 color;
} vsOut;

void main() {
  gl_Position = WorldViewProj * vec4(VsInPos, 1.0f);
  vsOut.color = VsInColor;
}

)";

constexpr const char* const kFragmentShaderCode = R"(
#version 450 core

in VsOutFsIn {
  vec4 color;
} fsIn;

layout(location = 0) out vec4 FinalFragColor;

void main() {
  FinalFragColor = fsIn.color;
}
)";

}

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
RenderDebugDraw::draw_line(const math::vec3f& from, const math::vec3f& to, const rgb_color& color)
{
    mRenderState.mVertices.emplace_back(from, color);
    mRenderState.mVertices.emplace_back(to, color);
}

void
RenderDebugDraw::draw_box(const std::span<const math::vec3f> points, const rgb_color& c)
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
RenderDebugDraw::draw_frustrum(const math::MatrixWithInvertedMatrixPair4f& mtx, const rgb_color& color)
{
    // NDC coords near + far
    constexpr const math::vec3f planes_points[] = { // near plane
                                                    { -1.0f, -1.0f, -1.0f },
                                                    { 1.0f, -1.0f, -1.0f },
                                                    { 1.0f, 1.0f, -1.0f },
                                                    { -1.0f, 1.0f, -1.0f },
                                                    // far plane
                                                    { -1.0f, -1.0f, 1.0f },
                                                    { 1.0f, -1.0f, 1.0f },
                                                    { 1.0f, 1.0f, 1.0f },
                                                    { -1.0f, 1.0f, 1.0f }
    };

    // NDC -> view space
    const itlib::small_vector<math::vec3f, 8> points =
        lz::chain(planes_points)
            .map([&mtx](const math::vec3f& p) { return math::mul_point(mtx.inverted, math::vec4f{ p }); })
            .filter([](const math::vec4f& p) { return std::fabs(p.w) > 1.0e-4; })
            .map([](const math::vec4f& p) {
                return math::vec3f{ p.x / p.w, p.y / p.w, p.z / p.w };
            })
            .to<itlib::small_vector<math::vec3f, 8>>();

    if (points.size() == 8)
        draw_box(std::span{ points }, color);
}

void
RenderDebugDraw::draw_oriented_box(const math::vec3f& org,
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

    const math::vec3f kVertices[] = { org + u * eu - v * ev - w * ew, org + u * eu + v * ev - w * ew,
                                      org - u * eu + v * ev - w * ew, org - u * eu - v * ev - w * ew,

                                      org + u * eu - v * ev + w * ew, org + u * eu + v * ev + w * ew,
                                      org - u * eu + v * ev + w * ew, org - u * eu - v * ev + w * ew };

    const uint32_t kIndices[] = { 0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7 };

    lz::chain(kIndices)
        .map([kVertices, c](const uint32_t idx) {
            return vertex_pc{ kVertices[idx], c };
        })
        .copyTo(std::back_inserter(mRenderState.mVertices));

    draw_cross(org, u, v, w, std::min({ eu, ev, ew }) * 0.25f, c);
}

void
RenderDebugDraw::draw_axis_aligned_box(const math::vec3f& minpoint, const math::vec3f& maxpoint, const rgb_color& c)
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
RenderDebugDraw::draw_cross(const math::vec3f& o,
                            const math::vec3f& u,
                            const math::vec3f& v,
                            const math::vec3f& w,
                            const float ext,
                            const rgb_color& color)
{
    const float s{ ext * 0.5f };
    mRenderState.mVertices.emplace_back(o + u * s, color);
    mRenderState.mVertices.emplace_back(o - u * s, color);
    mRenderState.mVertices.emplace_back(o + v * s, color);
    mRenderState.mVertices.emplace_back(o - v * s, color);
    mRenderState.mVertices.emplace_back(o + w * s, color);
    mRenderState.mVertices.emplace_back(o - w * s, color);
}

void
RenderDebugDraw::draw_coord_sys(const math::vec3f& o,
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
RenderDebugDraw::draw_grid(const math::vec3f origin,
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
RenderDebugDraw::draw_arrow(const math::vec3f& from, const math::vec3f& to, const rgb_color& c)
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
    const math::vec3f kVertices[] = { // tip
                                      to,
                                      from + u * adjustedLen + v * capLen + w * capLen,
                                      from + u * adjustedLen - v * capLen + w * capLen,
                                      from + u * adjustedLen - v * capLen - w * capLen,
                                      from + u * adjustedLen + v * capLen - w * capLen
    };

    const uint32_t kIndices[] = { 1, 2, 2, 3, 3, 4, 4, 1, 0, 1, 0, 2, 0, 3, 0, 4, 2, 4, 1, 3 };

    lz::chain(kIndices).transformTo(std::back_inserter(mRenderState.mVertices), [kVertices, c](const uint32_t idx) {
        return vertex_pc{ kVertices[idx], c };
    });
}

void
RenderDebugDraw::draw_directional_light(const math::vec3f& dir, const float origin_dist, const rgb_color& c)
{
    auto [u, v, w] = math::make_frame_vectors(dir);
    constexpr const float kBoxSize{ 0.5f };
    const math::vec3f origin = dir * (-origin_dist);
    draw_oriented_box(origin, u, v, w, kBoxSize, kBoxSize, kBoxSize, c);
    draw_arrow(origin, origin + dir * 4.0f, c);
}

void
RenderDebugDraw::draw_plane(const math::vec3f& origin, const math::vec3f& normal, const float scale, const rgb_color& c)
{
    const auto [u, v, w] = math::make_frame_vectors(normal);
    const math::vec3f kVertices[] = {
        origin + v * scale * 0.5f + w * scale * 0.5f,
        origin - v * scale * 0.5f + w * scale * 0.5f,
        origin - v * scale * 0.5f - w * scale * 0.5f,
        origin + v * scale * 0.5f - w * scale * 0.5f,
    };

    constexpr const uint32_t kIndices[] = { 0, 1, 1, 2, 2, 3, 3, 0 };
    lz::chain(kIndices).transformTo(std::back_inserter(mRenderState.mVertices), [kVertices, c](const uint32_t idx) {
        return vertex_pc{ kVertices[idx], c };
    });

    draw_arrow(origin, origin + normal * scale * 0.5f, c);
}

void
RenderDebugDraw::draw_circle(const math::vec3f& origin,
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
RenderDebugDraw::draw_sphere(const math::vec3f& origin, const float radius, const rgb_color& color)
{
    draw_circle(origin, math::vec3f::stdc::unit_x, radius, color, Draw_NoOptions);
    draw_circle(origin, math::vec3f::stdc::unit_y, radius, color, Draw_NoOptions);
    draw_circle(origin, math::vec3f::stdc::unit_z, radius, color, Draw_NoOptions);
}

void
RenderDebugDraw::draw_cone(const math::vec3f origin,
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
RenderDebugDraw::draw_vector(const math::vec3f& org, const math::vec3f& dir, const float length, const rgb_color& c)
{
    draw_arrow(org, org + dir * length, c);
}

tl::optional<RenderDebugDraw>
RenderDebugDraw::create()
{
    scoped_buffer vertexBuffer{ []() {
        GLuint buffHandle{};
        gl::CreateBuffers(1, &buffHandle);
        gl::NamedBufferStorage(buffHandle, 8192 * sizeof(vertex_pc), nullptr, gl::MAP_WRITE_BIT);

        return buffHandle;
    }() };

    scoped_vertex_array vertexArray{ [vb = base::raw_handle(vertexBuffer)]() {
        GLuint vaoObj{};
        gl::CreateVertexArrays(1, &vaoObj);
        gl::VertexArrayVertexBuffer(vaoObj, 0, vb, 0, sizeof(vertex_pc));

        gl::VertexArrayAttribFormat(vaoObj, 0, 3, gl::FLOAT, gl::FALSE_, offsetof(vertex_pc, position));
        gl::EnableVertexArrayAttrib(vaoObj, 0);
        gl::VertexArrayAttribBinding(vaoObj, 0, 0);

        gl::VertexArrayAttribFormat(vaoObj, 1, 4, gl::FLOAT, gl::FALSE_, offsetof(vertex_pc, color));
        gl::EnableVertexArrayAttrib(vaoObj, 1);
        gl::VertexArrayAttribBinding(vaoObj, 1, 0);

        return vaoObj;
    }() };

    vertex_program vertexShader =
        gpu_program_builder{}.add_string(internal::kVertexShaderCode).build<render_stage::e::vertex>();
    if (!vertexShader)
        return tl::nullopt;

    fragment_program fragmentShader =
        gpu_program_builder{}.add_string(internal::kFragmentShaderCode).build<render_stage::e::fragment>();
    if (!fragmentShader)
        return tl::nullopt;

    program_pipeline graphicsPipeline{ []() {
        GLuint pp{};
        gl::CreateProgramPipelines(1, &pp);
        return pp;
    }() };

    graphicsPipeline.use_vertex_program(vertexShader).use_fragment_program(fragmentShader);

    using std::move;
    return tl::optional<RenderDebugDraw>{ RenderDebugDraw{ std::move(vertexBuffer),
                                                           std::move(vertexArray),
                                                           std::move(vertexShader),
                                                           std::move(fragmentShader),
                                                           std::move(graphicsPipeline) } };
}

void
RenderDebugDraw::render(const math::mat4f& worldViewProj)
{
    if (mRenderState.mVertices.empty())
        return;

    scoped_resource_mapping::map(mRenderState.mVertexBuffer,
                                 gl::MAP_WRITE_BIT | gl::MAP_INVALIDATE_BUFFER_BIT,
                                 base::container_bytes_size(mRenderState.mVertices))
        .map([vtx = &mRenderState.mVertices](scoped_resource_mapping mappedBuffer) {
            memcpy(mappedBuffer.memory(), vtx->data(), base::container_bytes_size(*vtx));
        });

    mRenderState.mGraphicsPipeline.use(false);
    gl::BindVertexArray(base::raw_handle(mRenderState.mVertexArrayObj));
    mRenderState.mVertexShader.set_uniform_block("Ubo", worldViewProj);
    mRenderState.mVertexShader.flush_uniforms();

    gl::DrawArrays(gl::LINES, 0, static_cast<GLsizei>(mRenderState.mVertices.size()));
    mRenderState.mVertices.clear();
}

}
