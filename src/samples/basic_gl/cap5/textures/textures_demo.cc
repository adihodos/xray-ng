#include "cap5/textures/textures_demo.hpp"
#include "helpers.hpp"
#include "std_assets.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/math/scalar3x3_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include "xray/rendering/opengl/shader_base.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include <algorithm>
#include <imgui/imgui.h>
#include <span.h>
#include <vector>

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace std;

extern xray::base::app_config* xr_app_config;

app::textures_demo::textures_demo() { init(); }

app::textures_demo::~textures_demo() {}

void app::textures_demo::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());

  gl::ClearColor(0.0f, 0.0, 0.0f, 1.0f);
  gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);

  gl::BindVertexArray(raw_handle(_vertex_array));

  struct matrix_pack {
    mat4f world_view;
    mat4f normals_view;
    mat4f world_view_proj;
  } const obj_transforms{dc.view_matrix, dc.view_matrix, dc.proj_view_matrix};

  _draw_prog.set_uniform_block("matrix_pack", obj_transforms);
  _draw_prog.set_uniform("material", 0);
  _draw_prog.bind_to_pipeline();

  gl::BindTextureUnit(0, raw_handle(_texture));
  gl::BindSampler(0, raw_handle(_sampler));

  // gl::CullFace(gl::NONE);
  gl::Disable(gl::CULL_FACE);
  gl::PolygonMode(gl::FRONT_AND_BACK, gl::LINE);

  gl::DrawElements(gl::TRIANGLES, _mesh_index_count, gl::UNSIGNED_INT, nullptr);

  gl::Enable(gl::CULL_FACE);
  gl::PolygonMode(gl::FRONT_AND_BACK, gl::FILL);
}

void app::textures_demo::update(const float /*delta_ms*/) {}

void app::textures_demo::key_event(const int32_t /*key_code*/,
                                   const int32_t /*action*/,
                                   const int32_t /*mods*/) {}

void app::textures_demo::init() {
  {
    const GLuint compiled_shaders[] = {
        make_shader(gl::VERTEX_SHADER, "shaders/cap5/textures/shader.vert"),
        make_shader(gl::FRAGMENT_SHADER, "shaders/cap5/textures/shader.frag")};

    _draw_prog = gpu_program{compiled_shaders};
    if (!_draw_prog) {
      XR_LOG_ERR("Failed to create/link program!");
      return;
    }
  }

  {
    gl::CreateBuffers(1, raw_handle_ptr(_vertex_buffer));
    gl::CreateBuffers(1, raw_handle_ptr(_index_buffer));
    update_shape(0);
  }

  _vertex_array =
      [ vbh = raw_handle(_vertex_buffer), ibh = raw_handle(_index_buffer) ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);

    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ibh);
    gl::VertexArrayVertexBuffer(vao, 0, vbh, 0, sizeof(vertex_pnt));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);
    gl::EnableVertexArrayAttrib(vao, 2);

    gl::VertexArrayAttribFormat(vao, 0, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pnt, position));
    gl::VertexArrayAttribFormat(vao, 1, 3, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pnt, normal));
    gl::VertexArrayAttribFormat(vao, 2, 2, gl::FLOAT, gl::FALSE_,
                                XR_U32_OFFSETOF(vertex_pnt, texcoord));

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);
    gl::VertexArrayAttribBinding(vao, 2, 0);

    return vao;
  }
  ();

  texture_loader tex_ldr{
      c_str_ptr(xr_app_config->texture_path("uv_grids/ash_uvgrid01.jpg"))};

  if (!tex_ldr)
    return;

  _texture = [&tex_ldr]() {
    GLuint texh{};
    gl::CreateTextures(gl::TEXTURE_2D, 1, &texh);
    gl::TextureStorage2D(texh, 1, gl::RGB8, tex_ldr.width(), tex_ldr.height());
    gl::TextureSubImage2D(texh, 0, 0, 0, tex_ldr.width(), tex_ldr.height(),
                          gl::RGB, gl::UNSIGNED_BYTE, tex_ldr.data());

    return texh;
  }();

  _sampler = []() {
    GLuint smph{};
    gl::CreateSamplers(1, &smph);
    gl::SamplerParameteri(smph, gl::TEXTURE_MIN_FILTER, gl::LINEAR);
    gl::SamplerParameteri(smph, gl::TEXTURE_MAG_FILTER, gl::LINEAR);

    return smph;
  }();

  _valid = true;
}

#define PASTE_ENUM(etype, emember) etype::emember

#define MAKE_SHAPE_TUPLE(name)                                                 \
  { #name, app::shape_type::name }

const app::shape_name_id_tuple AVAILABLE_SHAPES[] = {
    MAKE_SHAPE_TUPLE(tetrahedron), MAKE_SHAPE_TUPLE(hexahedron),
    MAKE_SHAPE_TUPLE(octahedron),  MAKE_SHAPE_TUPLE(dodecahedron),
    MAKE_SHAPE_TUPLE(icosahedron), MAKE_SHAPE_TUPLE(cylinder),
    MAKE_SHAPE_TUPLE(sphere)};

void app::textures_demo::compose_ui() {

  struct combo_filler {
    static bool func(void* data, int32_t item_idx, const char** item_name) {
      const auto data_src = static_cast<const shape_name_id_tuple*>(data);
      *item_name          = data_src[item_idx].s_name;
      return true;
    }
  };

  ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  {
    int32_t sel_item_idx{static_cast<int32_t>(_sel_shape_idx)};
    ImGui::Combo("Shapes", &sel_item_idx, &combo_filler::func,
                 (void*) AVAILABLE_SHAPES, XR_I32_COUNTOF__(AVAILABLE_SHAPES),
                 5);

    const auto idxu32 = static_cast<uint32_t>(sel_item_idx);
    assert(idxu32 < XR_U32_COUNTOF__(AVAILABLE_SHAPES));

    if (idxu32 != _sel_shape_idx)
      update_shape(idxu32);

    shape_specific_ui();
  }
  ImGui::End();
}

void app::textures_demo::update_shape(const uint32_t index) {
  assert(index < XR_COUNTOF__(AVAILABLE_SHAPES));

  const auto&     shape_data = AVAILABLE_SHAPES[index];
  geometry_data_t mesh;

  switch (shape_data.s_type) {
  case shape_type::tetrahedron:
    geometry_factory::tetrahedron(&mesh);
    break;

  case shape_type::hexahedron:
    geometry_factory::hexahedron(&mesh);
    break;

  case shape_type::octahedron:
    geometry_factory::octahedron(&mesh);
    break;

  case shape_type::dodecahedron:
    geometry_factory::dodecahedron(&mesh);
    break;

  case shape_type::icosahedron:
    geometry_factory::icosahedron(&mesh);
    break;

  case shape_type::cylinder:
    geometry_factory::cylinder(
        static_cast<uint32_t>(_cyl_params.ring_tess_factor),
        static_cast<uint32_t>(_cyl_params.rings), _cyl_params.height,
        _cyl_params.radius, &mesh);
    break;

  case shape_type::sphere:
    geometry_factory::geosphere(
        _sphere_params.radius,
        static_cast<int32_t>(_sphere_params.subdivisions), &mesh);
    break;

  default:
    assert(false && "Unsupported shape");
    return;
    break;
  }

  vector<vertex_pnt> vertices{};
  vertices.reserve(mesh.vertex_count);

  transform(raw_ptr(mesh.geometry), raw_ptr(mesh.geometry) + mesh.vertex_count,
            back_inserter(vertices), [](const auto& vs_in) {
              return vertex_pnt{vs_in.position, vs_in.normal, vs_in.texcoords};
            });

  const auto size_vertexbuffer = bytes_size(vertices);

  if (size_vertexbuffer <= _bytes_vertexbuffer) {
    //
    // reuse buffer data store
    gl::NamedBufferSubData(raw_handle(_vertex_buffer), 0, size_vertexbuffer,
                           &vertices[0]);
  } else {
    //
    // reallocate buffer data store
    gl::NamedBufferData(raw_handle(_vertex_buffer), size_vertexbuffer,
                        &vertices[0], gl::DYNAMIC_DRAW);
    _bytes_vertexbuffer = size_vertexbuffer;
  }

  const auto size_indexbuffer = mesh.index_count * sizeof(uint32_t);
  if (size_indexbuffer <= _bytes_indexbuffer) {
    //
    // reuse data store
    gl::NamedBufferSubData(raw_handle(_index_buffer), 0, size_indexbuffer,
                           raw_ptr(mesh.indices));
  } else {
    //
    // reallocate data store
    gl::NamedBufferData(raw_handle(_index_buffer), size_indexbuffer,
                        raw_ptr(mesh.indices), gl::DYNAMIC_DRAW);
    _bytes_indexbuffer = size_indexbuffer;
  }

  _mesh_index_count = mesh.index_count;
  _sel_shape_idx    = index;
}

void app::textures_demo::shape_specific_ui() {
  const auto stype = AVAILABLE_SHAPES[_sel_shape_idx].s_type;

  bool update_needed{false};
  if (stype == shape_type::cylinder) {
    ImGui::Separator();

    update_needed |= ImGui::SliderInt("Horizontal tesselation",
                                      &_cyl_params.ring_tess_factor, 3, 128);
    update_needed |=
        ImGui::SliderInt("Vertical tesselation", &_cyl_params.rings, 3, 128);
    update_needed |=
        ImGui::SliderFloat("Height", &_cyl_params.height, 0.2f, 4.0f, "%3.3f");
    update_needed |=
        ImGui::SliderFloat("Radius", &_cyl_params.radius, 0.2f, 5.0f, "%3.3f");
  } else if (stype == shape_type::sphere) {
    ImGui::Separator();

    update_needed |=
        ImGui::SliderInt("Subdivisions", &_sphere_params.subdivisions, 1, 5);
    update_needed |= ImGui::SliderFloat("Radius", &_sphere_params.radius, 0.1f,
                                        5.0f, "%3.3f");
  }

  if (update_needed)
    update_shape(_sel_shape_idx);
}
