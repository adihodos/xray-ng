#pragma once

#include "xray/xray.hpp"

#include <span>
#include <memory>
#include <tl/expected.hpp>

#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/rendering/colors/rgb_color.hpp"

namespace xray::rendering {

#if defined(XRAY_GRAPHICS_API_VULKAN)
struct FrameRenderData;
class VulkanRenderer;
struct VulkanError;
#endif

class basic_mesh;

/// @brief Debug drawing
class DebugDrawSystem
{
  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() noexcept = default;
    };

  public:
#if defined(XRAY_GRAPHICS_API_VULKAN)
    struct InitContext
    {
        VulkanRenderer* renderer;
    };

    struct RenderContext
    {
        const VulkanRenderer* renderer;
        const FrameRenderData* frd;
    };

    using ErrorType = VulkanError;
#elif defined(XRAY_GRAPHICS_API_OPENGL)
    struct InitInitContext
    {};
    struct RenderContext
    {};
    struct ErrorType
    {};
#else
#endif

    enum
    {
        Draw_NoOptions = 0,
        Draw_CircleNormal = 0b1,
        Draw_CircleSegments = 0b10
    };

    static tl::expected<DebugDrawSystem, ErrorType> create(const InitContext& init);

    /// @brief Draw a colored line between two points.
    void draw_line(const math::vec3f& from, const math::vec3f& to, const rgb_color& color);

    /// @brief Draws a box defined by 8 points
    void draw_box(const std::span<const math::vec3f> points, const rgb_color& c);

    /// @brief Draw oriented box
    void draw_oriented_box(const math::vec3f& org,
                           const math::vec3f& r,
                           const math::vec3f& u,
                           const math::vec3f& d,
                           const float lr,
                           const float lu,
                           const float ld,
                           const rgb_color& c);

    /// @brief Draws axis aligned box
    void draw_axis_aligned_box(const math::vec3f& minpoint, const math::vec3f& maxpoint, const rgb_color& c);

    /// @brief Draw cross
    void draw_cross(const math::vec3f& o,
                    const math::vec3f& u,
                    const math::vec3f& v,
                    const math::vec3f& w,
                    const float ext,
                    const rgb_color& color);

    /// @brief Draws a coordinate system’s axis.
    void draw_coord_sys(const math::vec3f& o,
                        const math::vec3f& u,
                        const math::vec3f& v,
                        const math::vec3f& w,
                        const float ext,
                        const rgb_color& cu,
                        const rgb_color& cv,
                        const rgb_color& cw);

    /// @brief Draws an arrow.
    void draw_arrow(const math::vec3f& from, const math::vec3f& to, const rgb_color& c);

    /// @brief Draws a directional light
    void draw_directional_light(const math::vec3f& dir, const float origin_dist, const rgb_color& c);

    /// @brief Draws a plane
    void draw_plane(const math::vec3f& origin, const math::vec3f& normal, const float scale, const rgb_color& c);

    /// @brief Draws a circle in a plane
    void draw_circle(const math::vec3f& origin,
                     const math::vec3f& normal,
                     const float radius,
                     const rgb_color& c,
                     const uint32_t draw_options = Draw_CircleNormal | Draw_CircleSegments);

    /// @brief Draws a sphere
    void draw_sphere(const math::vec3f& origin, const float radius, const rgb_color& color);

    /// @brief Draws a cone
    void draw_cone(const math::vec3f origin,
                   const math::vec3f& normal,
                   const float height,
                   const float base,
                   const float apex,
                   const rgb_color& c);

    /// @brief Draws a vector
    void draw_vector(const math::vec3f& org, const math::vec3f& dir, const float length, const rgb_color& c);

    /// @brief Draw a grid
    void draw_grid(const math::vec3f origin,
                   const math::vec3f x_axis,
                   const math::vec3f z_axis,
                   const size_t x_divs,
                   const size_t z_divs,
                   const float cell_size,
                   const rgb_color color);

    /// @brief Visualize surface normals for a mesh
    void draw_surface_normals(const basic_mesh& mesh,
                              const math::mat4f& wvp_matrix,
                              const rgb_color& draw_color_start,
                              const rgb_color& draw_color_end,
                              const float line_length = 6.0f)

    {
        // mSurfaceNormalVis.draw(mesh, wvp_matrix, draw_color_start, draw_color_end, line_length);
    }

    /// @brief Draw a frustrum
    void draw_frustrum(const math::MatrixWithInvertedMatrixPair4f& mtx, const rgb_color& color);
    void new_frame(const uint32_t frame_idx) noexcept;
    void render(const RenderContext& rc) noexcept;

#if defined(XRAY_GRAPHICS_API_VULKAN)
  private:
    struct RenderStateVulkan;
    using RenderState = RenderStateVulkan;
    // struct RenderState
    // {
    //     std::vector<vertex_pc> mVertices;
    //     scoped_buffer mVertexBuffer;
    //     scoped_vertex_array mVertexArrayObj;
    //     vertex_program mVertexShader;
    //     fragment_program mFragmentShader;
    //     program_pipeline mGraphicsPipeline;
    // }
#else
  private:
    struct RenderStateOpenGL;
    using RenderState = RenderStateOpenGL;

#endif

  private:
    struct RenderStateDeleter
    {
        void operator()(RenderState* r) const noexcept;
    };
    std::unique_ptr<RenderState, RenderStateDeleter> mRenderState;

  public:
    DebugDrawSystem(PrivateConstructionToken, std::unique_ptr<RenderState, RenderStateDeleter>&& render_state);

    DebugDrawSystem(DebugDrawSystem&&) noexcept = default;
    ~DebugDrawSystem();
};

}
