#include "soubroutines_demo.hpp"
#include "std_assets.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <opengl/opengl.hpp>
#include <vector>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace std;

app::soubroutines_demo::soubroutines_demo() { init(); }

app::soubroutines_demo::~soubroutines_demo() {}

void app::soubroutines_demo::init() {
  {
    geometry_data_t torus_mesh{};
    if (!geometry_factory::load_model(
            &torus_mesh, MODEL_FILE_VIPER,
            mesh_import_options::remove_points_lines)) {
      geometry_factory::torus(0.7f, 0.3f, 30, 30, &torus_mesh);
    }

    vector<vertex_pn> mesh_verts;
    mesh_verts.reserve(torus_mesh.vertex_count);

    std::transform(raw_ptr(torus_mesh.geometry),
                   raw_ptr(torus_mesh.geometry) + torus_mesh.vertex_count,
                   back_inserter(mesh_verts), [](const auto& in_vertex) {
                     return vertex_pn{in_vertex.position, in_vertex.normal};
                   });

    {
      GLuint vbuff{0};
      gl::GenBuffers(1, &vbuff);

      unique_handle_reset(vertex_buffer_, vbuff);
      gl::BindBuffer(gl::ARRAY_BUFFER, raw_handle(vertex_buffer_));
      gl::NamedBufferStorage(raw_handle(vertex_buffer_),
                             mesh_verts.size() * sizeof(mesh_verts[0]),
                             &mesh_verts[0], 0);
    }

    {
      GLuint ibuff{0};
      gl::GenBuffers(1, &ibuff);

      unique_handle_reset(index_buffer_, ibuff);
      gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buffer_));
      gl::NamedBufferStorage(raw_handle(index_buffer_),
                             torus_mesh.index_count * sizeof(uint32_t),
                             raw_ptr(torus_mesh.indices), 0);

      index_count_ = static_cast<uint32_t>(torus_mesh.index_count);
    }
  }

  //
  // vertex layout
  {
    GLuint vao{0};
    gl::GenVertexArrays(1, &vao);
    unique_handle_reset(vertex_layout_, vao);

    gl::BindVertexArray(raw_handle(vertex_layout_));
    gl::VertexArrayVertexBuffer(raw_handle(vertex_layout_), 0,
                                raw_handle(vertex_buffer_), 0,
                                sizeof(vertex_pn));

    gl::EnableVertexArrayAttrib(raw_handle(vertex_layout_), 0);
    gl::EnableVertexArrayAttrib(raw_handle(vertex_layout_), 1);

    gl::VertexArrayAttribFormat(raw_handle(vertex_layout_), 0, 3, gl::FLOAT,
                                gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pn, position));
    gl::VertexArrayAttribFormat(raw_handle(vertex_layout_), 1, 3, gl::FLOAT,
                                gl::FALSE_, XR_U32_OFFSETOF(vertex_pn, normal));
    gl::VertexArrayAttribBinding(raw_handle(vertex_layout_), 0, 0);
    gl::VertexArrayAttribBinding(raw_handle(vertex_layout_), 1, 0);
  }

  {
    constexpr const char* const VS_FILE =
        "shaders/cap3/soubroutines/vert_shader.glsl";
    constexpr const char* const FS_FILE =
        "shaders/cap3/soubroutines/frag_shader.glsl";

    auto vert_shader = make_shader(gl::VERTEX_SHADER, VS_FILE);
    auto frag_shader = make_shader(gl::FRAGMENT_SHADER, FS_FILE);

    const GLuint shader_handles[] = {vert_shader, frag_shader};

    draw_prog_ = gpu_program{shader_handles};
    if (!draw_prog_) {
      XR_LOG_ERR("Failed to create drawing program !");
      return;
    }
  }
}

void app::soubroutines_demo::update(const float) {}

void app::soubroutines_demo::draw(const draw_context_t& draw_ctx) {
  assert(*this);

  gl::BindVertexArray(raw_handle(vertex_layout_));
  gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, raw_handle(index_buffer_));

  {
    struct light_info {
      float3    pos{0.0f, 12.0f, 0.0f};
      float     pad1;
      rgb_color ka{0.1f, 0.1f, 0.1f};
      rgb_color kd{1.0f, 1.0f, 1.0f};
      rgb_color ks{1.0f, 1.0f, 1.0f};
    } li;

    li.pos = mul_point(draw_ctx.view_matrix, li.pos);

    struct material {
      rgb_color ke;
      rgb_color ka;
      rgb_color kd;
      rgb_color ks;
    };

    constexpr material std_materials[] = {
        //
        // polished copper
        {{0.0f, 0.0f, 0.0f},
         {0.2295f, 0.08825f, 0.0275f},
         {0.5508f, 0.2118f, 0.066f},
         {0.580594f, 0.223257f, 0.0695701f, 51.2f}},
        //
        // polished gold
        {{0.0f, 0.0f, 0.0f},
         {0.24725f, 0.2245f, 0.0645f},
         {0.34615f, 0.3143f, 0.0903f},
         {0.797357f, 0.723991f, 0.208006f, 83.2f}}};

    draw_prog_.set_uniform_block("light_info", li);
    draw_prog_.set_uniform_block("material_info", std_materials[mtl_idx_ % 2]);

    struct transforms {
      float4x4 wvp;
      float4x4 mv;
      float4x4 nv;
    } tf;

    tf.wvp = draw_ctx.proj_view_matrix;
    tf.mv  = draw_ctx.view_matrix;
    tf.nv  = draw_ctx.view_matrix;

    draw_prog_.set_uniform_block("transforms", tf);

    static constexpr const char* const SHADING_MODEL_SUBROUTINES[] = {
        "light_model_diffuse", "light_model_ads"};

    draw_prog_.set_subroutine_uniform(
        graphics_pipeline_stage::vertex, "shading_model",
        SHADING_MODEL_SUBROUTINES[use_ads_lighting_]);
  }

  draw_prog_.bind_to_pipeline();
  gl::DrawElements(gl::TRIANGLES, index_count_, gl::UNSIGNED_INT, nullptr);
}

void app::soubroutines_demo::key_event(const int32_t key_code,
                                       const int32_t action,
                                       const int32_t /*mods*/) noexcept {

  if (action != GLFW_PRESS)
    return;

  if (key_code == GLFW_KEY_SPACE) {
    use_ads_lighting_ = !use_ads_lighting_;
    return;
  }

  if (key_code == GLFW_KEY_ENTER) {
    mtl_idx_++;
    return;
  }
}
