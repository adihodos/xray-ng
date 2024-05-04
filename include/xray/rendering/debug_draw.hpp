#pragma once

#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/opengl/program_pipeline.hpp"
#include "xray/rendering/opengl/scoped_opengl_setting.hpp"
#include "xray/rendering/vertex_format/vertex_pc.hpp"
#include "xray/xray.hpp"

#include <vector>

#include <tl/optional.hpp>

namespace xray::rendering {

/// @brief Debug drawing
class RenderDebugDraw
{
  public:
    static tl::optional<RenderDebugDraw> create();

    /// @brief Draw a colored line between two points.
    void draw_line(const math::vec3f& from, const math::vec3f& to, const rgb_color& color);

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
    void draw_circle(const math::vec3f& origin, const math::vec3f& normal, const float radius, const rgb_color& c);

    /// @brief Draws a cone
    void draw_cone(const math::vec3f origin,
                   const math::vec3f& normal,
                   const float height,
                   const float base,
                   const float apex,
                   const rgb_color& c);

	/// @brief Draws a vector
	void draw_vector(const math::vec3f& org, const math::vec3f& dir, const float length, const rgb_color& c);

    void render(const math::mat4f& worldViewProj);

  private:
    struct RenderState
    {
        std::vector<vertex_pc> mVertices;
        scoped_buffer mVertexBuffer;
        scoped_vertex_array mVertexArrayObj;
        vertex_program mVertexShader;
        fragment_program mFragmentShader;
        program_pipeline mGraphicsPipeline;
    } mRenderState;

    RenderDebugDraw(scoped_buffer&& vertexBuffer,
                    scoped_vertex_array vertexArray,
                    vertex_program vertexShader,
                    fragment_program fragmentShader,
                    program_pipeline graphicsPipeline)
        : mRenderState{ {},
                        std::move(vertexBuffer),
                        std::move(vertexArray),
                        std::move(vertexShader),
                        std::move(fragmentShader),
                        std::move(graphicsPipeline) }
    {
    }
};

}
