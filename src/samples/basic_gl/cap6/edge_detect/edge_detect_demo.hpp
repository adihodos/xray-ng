//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "xray/xray.hpp"
#include "demo_base.hpp"
#include "material.hpp"
#include "xray/base/base_fwd.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/rendering/colors/rgb_color.hpp"
#include "xray/rendering/mesh.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/scene/point_light.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace app {

struct model_id {
  uint32_t                       hashed_name{};
  xray::rendering::vertex_format format{
      xray::rendering::vertex_format::undefined};

  model_id() noexcept = default;

  model_id(const char* name, const xray::rendering::vertex_format fmt) noexcept
      : hashed_name{FNV::fnv1a(name)}, format{fmt} {}

  model_id(const std::string&                   name,
           const xray::rendering::vertex_format fmt) noexcept
      : model_id{name.c_str(), fmt} {}
};

inline bool operator==(const model_id& lhs, const model_id& rhs) noexcept {
  return lhs.hashed_name == rhs.hashed_name && lhs.format == rhs.format;
}

inline bool operator!=(const model_id& lhs, const model_id& rhs) noexcept {
  return !(lhs == rhs);
}

} // namespace app

namespace std {

template <>
struct hash<app::model_id> {
  using argument_type = app::model_id;
  using result_type   = std::size_t;

  result_type operator()(const argument_type& arg) const {
    return FNV::fnv1a(&arg, sizeof(arg));
  }
};

} // namespace std

namespace app {

struct basic_material {
  uint32_t                      ambient_map{};
  uint32_t                      diffuse_map{};
  uint32_t                      specular_map{};
  uint32_t                      texture_unit{};
  uint32_t                      sampler{};
  float                         spec_pwr{50.0f};
  xray::rendering::gpu_program* draw_prg{};
};

struct graphics_object {
  uint32_t                            hash;
  const basic_material*               mtl;
  const xray::rendering::simple_mesh* mesh;
  xray::math::float4x4                world_mtx;

  void draw(const xray::rendering::draw_context_t& dc) noexcept;
};

class material_cache {
public:
  material_cache() noexcept = default;
  const basic_material* get_material(const char* mtl_name) const;
  void add_material(const char* mtl_name, const basic_material& mtl);

private:
  std::unordered_map<uint32_t, basic_material> _cached_materials;
};

class texture_cache {
public:
  using texture_handle_type = GLuint;

  texture_cache();
  ~texture_cache();

  texture_handle_type get_texture(const char* name) noexcept;

  texture_handle_type
  add_from_file(const char* name, const char* texture_path,
                xray::rendering::texture_load_options load_opts =
                    xray::rendering::texture_load_options::none);

  texture_handle_type pink_texture() const noexcept;
  texture_handle_type black_texture() const noexcept;

  texture_handle_type add_from_color(const char*                       name,
                                     const xray::rendering::rgb_color& color);

  const auto& table() const noexcept { return _cached_textures; }

private:
  static texture_handle_type
  make_color_texture(const xray::rendering::rgb_color& clr,
                     const uint32_t width, const uint32_t height) noexcept;

private:
  std::unordered_map<uint32_t, texture_handle_type> _cached_textures;

private:
  XRAY_NO_COPY(texture_cache);
};

class mesh_cache {
public:
  mesh_cache() noexcept = default;

  const xray::rendering::simple_mesh* get_mesh(const char* name) const noexcept;
  const xray::rendering::simple_mesh*
  add_mesh(const char* name, const xray::rendering::vertex_format fmt);

  const xray::rendering::simple_mesh* get_mesh(const model_id& id) const
      noexcept;

  const xray::rendering::simple_mesh*
  add_mesh(const model_id& id, xray::rendering::simple_mesh mesh);

private:
  std::unordered_map<uint32_t, xray::rendering::simple_mesh> _cached_meshes;
  std::unordered_map<model_id, xray::rendering::simple_mesh> _stored_meshes;

private:
  XRAY_NO_COPY(mesh_cache);
};

enum class shader_type { vertex, tess_control, tess_eval, geometry, fragment };

struct shader_type_path_id_tuple {
  const char* file_name;
  shader_type type;
};

struct program_build_info {
  const char*               id{};
  const char*               rootpath{};
  shader_type_path_id_tuple shaders_by_stage[5];
  uint32_t                  stage_count{};
};

class shader_cache {
public:
  shader_cache() noexcept = default;

  xray::rendering::gpu_program* add_program(const program_build_info& pbi);
  xray::rendering::gpu_program* get_program(const char* id) noexcept;

private:
  std::unordered_map<uint32_t, xray::rendering::gpu_program> _cached_programs;

private:
  XRAY_NO_COPY(shader_cache);
};

class resource_store {
private:
  shader_cache   _shader_store;
  texture_cache  _tex_store;
  mesh_cache     _mesh_store;
  material_cache _mtl_store;

  static void load_texture_materials(const xray::base::config_file& cfg,
                                     app::shader_cache*             sstore,
                                     app::texture_cache*            tex_cache,
                                     app::material_cache*           mtl_cache);

  static void load_color_materials(const xray::base::config_file& cfg,
                                   app::shader_cache*             shader_store,
                                   app::texture_cache*            tex_cache,
                                   app::material_cache*           mtl_cache);

  static void load_graphics_objects(const xray::base::config_file& cfg,
                                    const app::material_cache*     mtl_cache,
                                    app::mesh_cache*               meshes,
                                    std::vector<app::graphics_object>* objects);

  static void load_shaders(const xray::base::config_file& cfg,
                           app::shader_cache*             scache);

public:
  resource_store() = default;
  ~resource_store();

  XRAY_DEFAULT_MOVE(resource_store);

  void load_graphics_objects(const xray::base::config_file&     cfg_file,
                             std::vector<app::graphics_object>* objects);

public:
  shader_cache*   shader_store{&_shader_store};
  texture_cache*  texture_store{&_tex_store};
  mesh_cache*     mesh_store{&_mesh_store};
  material_cache* mat_store{&_mtl_store};

private:
  XRAY_NO_COPY(resource_store);
};

class edge_detect_demo : public demo_base {
public:
  edge_detect_demo();

  ~edge_detect_demo();

  void compose_ui();

  virtual void draw(const xray::rendering::draw_context_t&) override;

  virtual void update(const float delta_ms) override;

  virtual void key_event(const int32_t key_code, const int32_t action,
                         const int32_t mods) override;

  explicit operator bool() const noexcept { return valid(); }

private:
  void init();

  enum { max_lights = 8u };

private:
  struct fbo_data {
    xray::rendering::scoped_framebuffer  fbo_object;
    xray::rendering::scoped_renderbuffer fbo_depth;
    xray::rendering::scoped_texture      fbo_texture;
    xray::rendering::scoped_sampler      fbo_sampler;
  } _fbo;

  xray::rendering::simple_mesh      _object;
  xray::rendering::simple_mesh      _object2;
  xray::rendering::scoped_texture   _obj_material;
  xray::rendering::scoped_texture   _obj_diffuse_map;
  xray::scene::point_light          _lights[edge_detect_demo::max_lights];
  uint32_t                          _lightcount{2};
  float                             _mat_spec_pwr{50.0f};
  std::vector<graphics_object>      _drawables;
  xray::rendering::vertex_program   _vertex_prg;
  xray::rendering::fragment_program _frag_prg;
  xray::rendering::scoped_program_pipeline_handle _prog_pipeline;

private:
  XRAY_NO_COPY(edge_detect_demo);
};

} // namespace app
