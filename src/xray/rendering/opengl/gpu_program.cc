#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/string.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <gsl.h>
#include <numeric>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <span.h>
#include <stlsoft/memory/auto_buffer.hpp>
#include <string>
#include <unordered_map>
#include <utility>

#define CASE_TO_STRING(case_id)                                                \
  case case_id:                                                                \
    return #case_id;                                                           \
    break

const char*
subroutine_uniform_interface_to_string(const uint32_t ifc) noexcept {
  switch (ifc) {
    CASE_TO_STRING(gl::VERTEX_SUBROUTINE_UNIFORM);
    CASE_TO_STRING(gl::FRAGMENT_SUBROUTINE_UNIFORM);
  default:
    break;
  }

  return "unknown/error";
}

const char* pipeline_stage_to_string(
    const xray::rendering::graphics_pipeline_stage ifc) noexcept {
  switch (ifc) {
    CASE_TO_STRING(xray::rendering::graphics_pipeline_stage::vertex);
    CASE_TO_STRING(xray::rendering::graphics_pipeline_stage::geometry);
    CASE_TO_STRING(xray::rendering::graphics_pipeline_stage::fragment);
    CASE_TO_STRING(xray::rendering::graphics_pipeline_stage::tess_control);
    CASE_TO_STRING(xray::rendering::graphics_pipeline_stage::tess_eval);
    CASE_TO_STRING(xray::rendering::graphics_pipeline_stage::compute);
  default:
    break;
  }

  return "unknown/error";
}

GLenum pipeline_stage_to_shader_type(
    const xray::rendering::pipeline_stage ps) noexcept {
  switch (ps) {
  case xray::rendering::pipeline_stage::vertex:
    return gl::VERTEX_SHADER;
    break;

  case xray::rendering::pipeline_stage::geometry:
    return gl::GEOMETRY_SHADER;
    break;

  case xray::rendering::pipeline_stage::fragment:
    return gl::FRAGMENT_SHADER;
    break;

  default:
    break;
  }

  return 0;
}

uint32_t
pipeline_stage_to_gl_enum(const xray::rendering::pipeline_stage ps) noexcept {
  switch (ps) {
  case xray::rendering::pipeline_stage::vertex:
    return gl::VERTEX_SHADER;
    break;

  case xray::rendering::pipeline_stage::fragment:
    return gl::FRAGMENT_SHADER;
    break;

  case xray::rendering::pipeline_stage::geometry:
    return gl::GEOMETRY_SHADER;
    break;

  default:
    break;
  }

  return 0;
}

xray::rendering::graphics_pipeline_stage
gl_stage_to_pipeline_stage(const uint32_t stage) noexcept {
  switch (stage) {
  case gl::VERTEX_SUBROUTINE:
    return xray::rendering::graphics_pipeline_stage::vertex;
    break;

  case gl::FRAGMENT_SUBROUTINE:
    return xray::rendering::graphics_pipeline_stage::fragment;
    break;

  case gl::GEOMETRY_SUBROUTINE:
    return xray::rendering::graphics_pipeline_stage::geometry;
    break;

  default:
    assert(false && "Unmaped stage!");
    break;
  }

  return xray::rendering::graphics_pipeline_stage::last;
}

const char* subroutine_interface_to_string(const uint32_t ifc) noexcept {
  switch (ifc) {
    CASE_TO_STRING(gl::VERTEX_SUBROUTINE);
    CASE_TO_STRING(gl::FRAGMENT_SUBROUTINE);

  default:
    break;
  }

  return "unknown/error";
}

xray::rendering::pipeline_stage
map_subroutine_interface_to_pipeline_stage(const uint32_t gl_id) noexcept {
  switch (gl_id) {
  case gl::VERTEX_SUBROUTINE_UNIFORM:
    return xray::rendering::pipeline_stage::vertex;
    break;

  case gl::FRAGMENT_SUBROUTINE_UNIFORM:
    return xray::rendering::pipeline_stage::fragment;
    break;

  case gl::GEOMETRY_SUBROUTINE_UNIFORM:
    return xray::rendering::pipeline_stage::geometry;
    break;

  default:
    break;
  }

  return xray::rendering::pipeline_stage::last;
}

template <uint32_t uniform_type>
struct uniform_traits;

template <>
struct uniform_traits<gl::UNSIGNED_INT> {
  static constexpr auto size = sizeof(uint32_t);
};

template <>
struct uniform_traits<gl::FLOAT> {
  static constexpr auto size = sizeof(xray::scalar_lowp);
};

template <>
struct uniform_traits<gl::FLOAT_VEC2> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 2;
};

template <>
struct uniform_traits<gl::FLOAT_VEC3> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 3;
};

template <>
struct uniform_traits<gl::FLOAT_VEC4> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 4;
};

template <>
struct uniform_traits<gl::FLOAT_MAT2> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 4;
};

template <>
struct uniform_traits<gl::FLOAT_MAT2x3> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 6;
};

template <>
struct uniform_traits<gl::FLOAT_MAT3x2> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 6;
};

template <>
struct uniform_traits<gl::FLOAT_MAT3> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 9;
};

template <>
struct uniform_traits<gl::FLOAT_MAT4> {
  static constexpr auto size = sizeof(xray::scalar_lowp) * 16;
};

#define PROG_UNIFORM_UIV(u_prg, u_func, u_loc, u_count, u_data)                \
  do {                                                                         \
    u_func(u_prg, u_loc, static_cast<GLsizei>(u_count),                        \
           static_cast<const GLuint*>(u_data));                                \
  } while (0)

#define PROG_UNIFORM_FV(u_prg, u_func, u_loc, u_count, u_data)                 \
  do {                                                                         \
    u_func(u_prg, u_loc, static_cast<GLsizei>(u_count),                        \
           static_cast<const GLfloat*>(u_data));                               \
  } while (0)

#define PROG_UNIFORM_MAT_FV(u_prg, u_func, u_loc, u_count, u_data)             \
  do {                                                                         \
    u_func(u_prg, u_loc, static_cast<GLsizei>(u_count), gl::TRUE_,             \
           static_cast<const GLfloat*>(u_data));                               \
  } while (0)

#define PROG_UNIFORM_IV(u_prg, u_func, u_loc, u_count, u_data)                 \
  do {                                                                         \
    u_func(u_prg, u_loc, static_cast<GLsizei>(u_count),                        \
           static_cast<const GLint*>(u_data));                                 \
  } while (0)

// void xray::rendering::set_uniform_impl(const GLuint   program_id,
//                                       const GLint    u_location,
//                                       const uint32_t uniform_type,
//                                       const void*    u_data,
//                                       const size_t   count) noexcept {
//  switch (uniform_type) {
//  case gl::FLOAT:
//    PROG_UNIFORM_FV(program_id, gl::ProgramUniform1fv, u_location, count,
//                    u_data);
//    break;

//  case gl::UNSIGNED_INT:
//    PROG_UNIFORM_UIV(program_id, gl::ProgramUniform1uiv, u_location, count,
//                     u_data);
//    break;

//  case gl::FLOAT_VEC2:
//    PROG_UNIFORM_FV(program_id, gl::ProgramUniform2fv, u_location, count,
//                    u_data);
//    break;

//  case gl::FLOAT_VEC3:
//    PROG_UNIFORM_FV(program_id, gl::ProgramUniform3fv, u_location, count,
//                    u_data);
//    break;

//  case gl::FLOAT_VEC4:
//    PROG_UNIFORM_FV(program_id, gl::ProgramUniform4fv, u_location, count,
//                    u_data);
//    break;

//  case gl::FLOAT_MAT2:
//    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix2fv, u_location,
//                        count, u_data);
//    break;

//  case gl::FLOAT_MAT2x3:
//    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix2x3fv, u_location,
//                        count, u_data);
//    break;

//  case gl::FLOAT_MAT3x2:
//    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix3x2fv, u_location,
//                        count, u_data);
//    break;

//  case gl::FLOAT_MAT3:
//    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix3fv, u_location,
//                        count, u_data);
//    break;

//  case gl::FLOAT_MAT4:
//    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix4fv, u_location,
//                        count, u_data);
//    break;

//  case gl::INT:
//  case gl::SAMPLER_2D:
//  case gl::SAMPLER_1D:
//  case gl::SAMPLER_CUBE:
//    PROG_UNIFORM_IV(program_id, gl::ProgramUniform1iv, u_location, count,
//                    u_data);
//    break;

//  default:
//    XR_LOG_CRITICAL("Unhandled uniform type {} in set()", uniform_type);
//    assert(false && "Unhandled uniform type in set()");
//    break;
//  }
//}

static uint32_t get_resource_size(const uint32_t res_type) noexcept {
  switch (res_type) {
  case gl::FLOAT:
    return (uint32_t) uniform_traits<gl::FLOAT>::size;
    break;

  case gl::UNSIGNED_INT:
    return (uint32_t) uniform_traits<gl::UNSIGNED_INT>::size;
    break;

  case gl::FLOAT_VEC2:
    return (uint32_t) uniform_traits<gl::FLOAT_VEC2>::size;
    break;

  case gl::FLOAT_VEC3:
    return (uint32_t) uniform_traits<gl::FLOAT_VEC3>::size;
    break;

  case gl::FLOAT_VEC4:
  case gl::FLOAT_MAT2:
    return (uint32_t) uniform_traits<gl::FLOAT_VEC4>::size;
    break;

  case gl::FLOAT_MAT3:
    return (uint32_t) uniform_traits<gl::FLOAT_MAT3>::size;
    break;

  case gl::FLOAT_MAT4:
    return (uint32_t) uniform_traits<gl::FLOAT_MAT4>::size;
    break;

  case gl::FLOAT_MAT2x3:
  case gl::FLOAT_MAT3x2:
    return (uint32_t) uniform_traits<gl::FLOAT_MAT2x3>::size;
    break;

  case gl::INT:
    return 4u;
    break;

  case gl::SAMPLER_2D:
  case gl::SAMPLER_CUBE:
  case gl::SAMPLER_1D:
    return 4u;
    break;

  default:
    XR_LOG_CRITICAL("Unmapped type {} in size info query", res_type);
    assert(false && "Unmapped type in size query, fix it!");
    break;
  }

  return 0;
}

GLuint xray::rendering::make_gpu_program(const GLuint* shaders_to_attach,
                                         const size_t  shaders_count) noexcept {
  scoped_program_handle tmp_handle{gl::CreateProgram()};
  if (!tmp_handle)
    return 0;

  using namespace xray::base;

  //
  // Attach shaders
  for (size_t idx = 0; idx < shaders_count; ++idx) {
    const auto valid_shader = gl::IsShader(shaders_to_attach[idx]);
    if (!valid_shader) {
      XR_LOG_ERR("Invalid shader passed to glAttactShader");
      return 0;
    }

    gl::AttachShader(raw_handle(tmp_handle), shaders_to_attach[idx]);
  }

  gl::LinkProgram(raw_handle(tmp_handle));

  //
  // Check for epic fail.
  {
    GLint link_status{gl::FALSE_};
    gl::GetProgramiv(raw_handle(tmp_handle), gl::LINK_STATUS, &link_status);

    if (link_status == gl::TRUE_)
      return unique_handle_release(tmp_handle);
  }

  //
  //  Epic fail, display error
  {
    GLint log_length{};
    gl::GetProgramiv(raw_handle(tmp_handle), gl::INFO_LOG_LENGTH, &log_length);

    if (log_length > 0) {
      stlsoft::auto_buffer<GLchar, 1024> err_buff{
          static_cast<size_t>(log_length)};

      gl::GetProgramInfoLog(raw_handle(tmp_handle),
                            static_cast<GLsizei>(log_length), nullptr,
                            err_buff.data());

      XR_LOG_CRITICAL("Program link error !");
      XR_LOG_CRITICAL("[{}]", err_buff.data());
    }
  }

  return 0;
}

xray::rendering::gpu_program::gpu_program(const GLuint* shaders_to_attach,
                                          const size_t  shaders_count) noexcept
    : prog_handle_{make_gpu_program(shaders_to_attach, shaders_count)} {
  valid_ = reflect();
}

xray::rendering::gpu_program::gpu_program(
    scoped_program_handle linked_prg) noexcept
    : prog_handle_{std::move(linked_prg)} {
  valid_ = reflect();
}

bool xray::rendering::gpu_program::reflect() {
  assert(prog_handle_ && "Oops");
  assert(uniform_blocks_.empty());
  assert(uniforms_.empty());
  assert(!ublocks_datastore_);

  if (!collect_uniform_blocks())
    return false;

  if (!collect_uniforms())
    return false;

  if (!collect_subroutines_and_uniforms())
    return false;

  return true;
}

void xray::rendering::gpu_program::bind_to_pipeline() {
  assert(valid());

  using namespace std;
  using namespace xray::base;

  //
  // writa data for blocks marked as dirty
  for_each(begin(uniform_blocks_), end(uniform_blocks_), [this](auto& u_blk) {
    if (u_blk.dirty) {

      auto src_ptr = raw_ptr(ublocks_datastore_) + u_blk.store_offset;

      {
        scoped_resource_mapping ubuff_mapping{raw_handle(u_blk.gl_buff),
                                              gl::MAP_WRITE_BIT, u_blk.size};

        if (!ubuff_mapping)
          return;

        memcpy(ubuff_mapping.memory(), src_ptr, u_blk.size);
      }
      u_blk.dirty = false;
    }

    gl::BindBuffer(gl::UNIFORM_BUFFER, raw_handle(u_blk.gl_buff));
    gl::BindBufferBase(gl::UNIFORM_BUFFER, u_blk.bindpoint,
                       raw_handle(u_blk.gl_buff));

  });

  gl::UseProgram(raw_handle(prog_handle_));

  {
    auto stages = gsl::span<pipeline_stage_subroutine_uniform_data>{
        stage_subroutine_ufs_};

    auto uniforms_data_store =
        gsl::span<shader_subroutine_uniform>{subroutine_uniforms_};

    for (uint8_t stage      = static_cast<uint8_t>(pipeline_stage::vertex),
                 max_stages = static_cast<uint8_t>(pipeline_stage::last);
         stage < max_stages; ++stage) {

      const auto& sub_unif_stage_data = stages[stage];

      if (sub_unif_stage_data.datastore_offset ==
          pipeline_stage_subroutine_uniform_data::invalid_offset)
        continue;

      stlsoft::auto_buffer<GLuint> indices_buff{
          sub_unif_stage_data.max_active_locations};

      auto indices =
          gsl::span<GLuint>(indices_buff.data(), indices_buff.size());

      for (size_t idx = sub_unif_stage_data.datastore_offset;
           idx < sub_unif_stage_data.datastore_offset +
                     sub_unif_stage_data.uniforms_count;
           ++idx) {

        const auto& subrout_unif_data = uniforms_data_store[idx];
        indices[subrout_unif_data.ssu_location] =
            subrout_unif_data.ssu_assigned_subroutine_idx;
      }

      gl::UniformSubroutinesuiv(
          pipeline_stage_to_shader_type(static_cast<pipeline_stage>(stage)),
          indices_buff.size(), indices_buff.data());
    }
  }
}

bool xray::rendering::gpu_program::collect_uniform_blocks() {
  assert(prog_handle_);

  using namespace xray::base;
  using namespace std;
  using namespace xray::rendering;

  const auto phandle = raw_handle(prog_handle_);

  //
  // get number of uniform blocks and max length for uniform block name
  GLint num_uniform_blocks{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM_BLOCK, gl::ACTIVE_RESOURCES,
                            &num_uniform_blocks);

  GLint max_name_len{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM_BLOCK, gl::MAX_NAME_LENGTH,
                            &max_name_len);

  //
  // No uniform blocks
  if (num_uniform_blocks == 0)
    return true;

  if (max_name_len <= 0) {
    XR_LOG_CRITICAL("Failed to obtain uniform block name info !");
    return false;
  }

  //
  // colllect info about the uniforms
  stlsoft::auto_buffer<char, 128> txt_buff{static_cast<size_t>(max_name_len)};
  vector<uniform_block_t> ublocks{};

  for (uint32_t blk_idx = 0;
       blk_idx < static_cast<uint32_t>(num_uniform_blocks); ++blk_idx) {

    gl::GetProgramResourceName(phandle, gl::UNIFORM_BLOCK, blk_idx,
                               txt_buff.size(), nullptr, txt_buff.data());

    //
    // List of properties we need for the uniform blocks.
    const GLuint props_to_get[] = {
        gl::BUFFER_BINDING, gl::BUFFER_DATA_SIZE,
    };

    union ublock_prop_data_t {
      struct {
        int32_t ub_bindpoint;
        int32_t ub_size;
      };
      int32_t components[2];

      ublock_prop_data_t() noexcept { ub_bindpoint = ub_size = -1; }

      explicit operator bool() const noexcept {
        return (ub_bindpoint >= 0) && (ub_size >= 1);
      }

    } u_props;

    static_assert(XR_U32_COUNTOF__(props_to_get) ==
                      XR_U32_COUNTOF__(u_props.components),
                  "Mismatch between the count of requested properties and "
                  "their storage block");

    GLsizei props_retrieved{};

    gl::GetProgramResourceiv(phandle, gl::UNIFORM_BLOCK, blk_idx,
                             XR_U32_COUNTOF__(props_to_get), props_to_get,
                             XR_U32_COUNTOF__(u_props.components),
                             &props_retrieved, u_props.components);

    if ((props_retrieved != XR_U32_COUNTOF__(u_props.components)) || !u_props) {
      XR_LOG_ERR("Failed to retrieve all uniform block properties!");
      return false;
    }

    ublocks.push_back({txt_buff.data(), 0,
                       static_cast<uint32_t>(u_props.ub_size), blk_idx,
                       static_cast<uint32_t>(u_props.ub_bindpoint)});
  }

  //
  // compute number of required bytes for blocks and allocate storage.
  {
    uint32_t ublock_store_bytes_req{};
    for (auto itr_c = begin(ublocks), itr_end = end(ublocks); itr_c != itr_end;
         ++itr_c) {
      itr_c->store_offset = ublock_store_bytes_req;
      ublock_store_bytes_req += itr_c->size;
    }

    unique_pointer<uint8_t[]> store{new uint8_t[ublock_store_bytes_req]};

    //
    // create uniform buffers for active blocks
    constexpr auto kBufferCreateFlags = gl::MAP_WRITE_BIT;
    for_each(begin(ublocks), end(ublocks), [kBufferCreateFlags](auto& blk) {
      gl::CreateBuffers(1, raw_handle_ptr(blk.gl_buff));
      gl::NamedBufferStorage(raw_handle(blk.gl_buff), blk.size, nullptr,
                             kBufferCreateFlags);
    });

    //
    // abort if creation of any buffer failed.
    const auto any_fails = any_of(begin(ublocks), end(ublocks),
                                  [](const auto& blk) { return !blk.gl_buff; });

    if (any_fails) {
      return false;
    }

    //
    // Sort uniform blocks by their name
    sort(begin(ublocks), end(ublocks), [](const auto& blk0, const auto& blk1) {
      return blk0.name < blk1.name;
    });

    //
    // assign state here
    ublocks_datastore_ = std::move(store);
    uniform_blocks_    = std::move(ublocks);
  }

  return true;
}

bool xray::rendering::gpu_program::collect_uniforms() {
  using namespace xray::base;
  using namespace std;
  using namespace stlsoft;

  const auto phandle = raw_handle(prog_handle_);

  //
  // get number of standalone uniforms and maximum name length
  GLint num_uniforms{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM, gl::ACTIVE_RESOURCES,
                            &num_uniforms);

  //
  // Nothing to do if no standalone uniforms.
  if (num_uniforms == 0)
    return true;

  GLint u_max_len{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM, gl::MAX_NAME_LENGTH,
                            &u_max_len);

  auto_buffer<char, 128> tmp_buff{static_cast<size_t>(u_max_len)};

  for (uint32_t u_idx = 0; u_idx < static_cast<uint32_t>(num_uniforms);
       ++u_idx) {

    gl::GetProgramResourceName(phandle, gl::UNIFORM, u_idx, tmp_buff.size(),
                               nullptr, tmp_buff.data());

    union uniform_props_t {
      struct {
        int32_t u_blk_idx;
        int32_t u_blk_off;
        int32_t u_type;
        int32_t u_arr_dim;
        int32_t u_loc;
        int32_t u_stride;
      };

      int32_t u_components[6];

      uniform_props_t() noexcept {
        u_blk_idx = u_blk_off = u_type = u_arr_dim = u_loc = -1;
      }
    } u_props;

    const GLuint props_to_get[] = {gl::BLOCK_INDEX, gl::OFFSET,
                                   gl::TYPE,        gl::ARRAY_SIZE,
                                   gl::LOCATION,    gl::MATRIX_STRIDE};

    static_assert(
        XR_U32_COUNTOF__(u_props.u_components) ==
            XR_U32_COUNTOF__(props_to_get),
        "Size mismatch between number of properties to get and to store!");

    GLint props_retrieved{0};

    gl::GetProgramResourceiv(phandle, gl::UNIFORM, u_idx,
                             XR_U32_COUNTOF__(props_to_get), props_to_get,
                             XR_U32_COUNTOF__(u_props.u_components),
                             &props_retrieved, u_props.u_components);

    if (props_retrieved != XR_I32_COUNTOF__(props_to_get)) {
      return false;
    }

    auto new_uniform = uniform_t{
        tmp_buff.data(),
        get_resource_size(u_props.u_type) * u_props.u_arr_dim,
        u_props.u_blk_idx == -1 ? 0 : static_cast<uint32_t>(u_props.u_blk_off),
        u_props.u_blk_idx,
        static_cast<uint32_t>(u_props.u_type),
        static_cast<uint32_t>(u_props.u_arr_dim),
        static_cast<uint32_t>(u_props.u_loc),
        static_cast<uint32_t>(u_props.u_stride)};

    //
    // If this uniform is part of a block,
    // find index of the parent block in our list of blocks.
    if (u_props.u_blk_idx != -1) {
      auto parent_blk_itr = find_if(
          begin(uniform_blocks_), end(uniform_blocks_),
          [&u_props](const auto& ublk) {
            return ublk.index == static_cast<uint32_t>(u_props.u_blk_idx);
          });

      assert(parent_blk_itr != end(uniform_blocks_));

      new_uniform.parent_block_idx =
          std::distance(begin(uniform_blocks_), parent_blk_itr);
    }

    uniforms_.push_back(new_uniform);
  }

  //
  // sort uniforms by name.
  sort(begin(uniforms_), end(uniforms_),
       [](const auto& u0, const auto& u1) { return u0.name < u1.name; });

  return true;
}

bool xray::rendering::gpu_program::collect_subroutines_and_uniforms() {
  assert(prog_handle_);

  using namespace xray::base;
  using namespace std;

  const auto phandle = raw_handle(prog_handle_);

  using namespace std;

  //
  // Collect subroutine uniforms information.
  const uint32_t SUBROUTINE_UNIFORM_INTERFACES_IDS[] = {
      gl::VERTEX_SUBROUTINE_UNIFORM, gl::FRAGMENT_SUBROUTINE_UNIFORM};

  for (const auto sub_uf_ifc : SUBROUTINE_UNIFORM_INTERFACES_IDS) {
    int32_t active_uniforms{0};
    gl::GetProgramInterfaceiv(phandle, sub_uf_ifc, gl::ACTIVE_RESOURCES,
                              &active_uniforms);

    if (!active_uniforms)
      continue;

    int32_t max_name_length{0};
    gl::GetProgramInterfaceiv(phandle, sub_uf_ifc, gl::MAX_NAME_LENGTH,
                              &max_name_length);

    for (int32_t idx = 0; idx < active_uniforms; ++idx) {
      stlsoft::auto_buffer<char> name_buff{
          static_cast<size_t>(max_name_length) + 1};
      GLsizei chars_written{0};
      gl::GetProgramResourceName(phandle, sub_uf_ifc, idx, max_name_length,
                                 &chars_written, name_buff.data());
      name_buff.data()[chars_written] = 0;

      const auto uniform_location =
          gl::GetProgramResourceLocation(phandle, sub_uf_ifc, name_buff.data());

      subroutine_uniforms_.push_back(
          {name_buff.data(),
           map_subroutine_interface_to_pipeline_stage(sub_uf_ifc),
           static_cast<uint8_t>(uniform_location), 0xFF});
    }
  }

  if (subroutine_uniforms_.empty())
    return true;

  //
  // Sort subroutine uniforms by stage and by location.
  sort(begin(subroutine_uniforms_), end(subroutine_uniforms_),
       [](const auto& lhs, const auto& rhs) {

         if (lhs.ssu_stage == rhs.ssu_stage)
           return lhs.ssu_location < rhs.ssu_location;

         return lhs.ssu_stage < rhs.ssu_stage;
       });

  //
  // Fill per stage subroutine uniform data.
  {
    unordered_map<uint8_t, pipeline_stage_subroutine_uniform_data> stages;

    for (size_t idx = 0, uniform_count = subroutine_uniforms_.size();
         idx < uniform_count; ++idx) {

      const auto& uf_data = subroutine_uniforms_[idx];

      auto itr_stage_ufdata =
          stages.find(static_cast<uint8_t>(uf_data.ssu_stage));
      if (itr_stage_ufdata != end(stages)) {
        ++itr_stage_ufdata->second.uniforms_count;
        continue;
      }

      GLint max_locations{0};
      gl::GetProgramStageiv(
          phandle, pipeline_stage_to_gl_enum(uf_data.ssu_stage),
          gl::ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &max_locations);

      stages[static_cast<uint8_t>(uf_data.ssu_stage)] = {
          static_cast<uint8_t>(idx), 1, static_cast<uint8_t>(max_locations)};
    }

    for (const auto& s : stages) {
      stage_subroutine_ufs_[s.first] = s.second;
    }
  }

  //
  // collect subroutines
  const uint32_t SUBROUTINE_INTERFACE_IDS[] = {gl::VERTEX_SUBROUTINE,
                                               gl::FRAGMENT_SUBROUTINE};

  for (const auto sub_ifc : SUBROUTINE_INTERFACE_IDS) {
    int32_t subroutines_count{0};
    gl::GetProgramInterfaceiv(phandle, sub_ifc, gl::ACTIVE_RESOURCES,
                              &subroutines_count);

    if (!subroutines_count)
      continue;

    int32_t max_name_len{0};
    gl::GetProgramInterfaceiv(phandle, sub_ifc, gl::MAX_NAME_LENGTH,
                              &max_name_len);

    for (int32_t idx = 0; idx < subroutines_count; ++idx) {
      int32_t                    name_len{0};
      stlsoft::auto_buffer<char> name_buff{static_cast<size_t>(max_name_len) +
                                           1};

      gl::GetProgramResourceName(phandle, sub_ifc, idx, max_name_len, &name_len,
                                 name_buff.data());
      name_buff.data()[name_len] = 0;

      const auto subroutine_index =
          gl::GetProgramResourceIndex(phandle, sub_ifc, name_buff.data());

      subroutines_.push_back({name_buff.data(),
                              gl_stage_to_pipeline_stage(sub_ifc),
                              static_cast<uint8_t>(subroutine_index)});
    }
  }

  sort(begin(subroutines_), end(subroutines_),
       [](const auto& lhs, const auto& rhs) {
         if (lhs.ss_stage == rhs.ss_stage)
           return lhs.ss_name < rhs.ss_name;

         return lhs.ss_stage < rhs.ss_stage;
       });

  return true;
}

void xray::rendering::gpu_program::set_uniform_block(const char*  block_name,
                                                     const void*  block_data,
                                                     const size_t byte_count) {
  assert(valid());
  assert(block_name != nullptr);
  assert(block_data != nullptr);

  using namespace std;
  using namespace xray::base;

  auto blk_iter =
      find_if(begin(uniform_blocks_), end(uniform_blocks_),
              [block_name](auto& blk) { return blk.name == block_name; });

  if (blk_iter == end(uniform_blocks_)) {
    XR_LOG_ERR("{} error, uniform {} does not exist", __PRETTY_FUNCTION__,
               block_name);
    return;
  }

  assert(byte_count <= blk_iter->size);

  memcpy(raw_ptr(ublocks_datastore_) + blk_iter->store_offset, block_data,
         byte_count);

  blk_iter->dirty = true;
}

void xray::rendering::gpu_program::set_subroutine_uniform(
    const graphics_pipeline_stage stage, const char* uniform_name,
    const char* subroutine_name) noexcept {

  assert(uniform_name != nullptr);
  assert(subroutine_name != nullptr);

  const auto& stage_info = stage_subroutine_ufs_[static_cast<uint8_t>(stage)];

  if (stage_info.datastore_offset ==
      pipeline_stage_subroutine_uniform_data::invalid_offset) {
    XR_LOG_ERR("Attempt to set subroutine {} for subroutine uniform {}, but "
               "stage {} has no subroutine uniforms",
               subroutine_name, uniform_name, pipeline_stage_to_string(stage));

    return;
  }

  using namespace std;
  auto itr_unifrm = find_if(
      begin(subroutine_uniforms_), end(subroutine_uniforms_),
      [uniform_name](const auto& uf) { return uniform_name == uf.ssu_name; });

  if (itr_unifrm == end(subroutine_uniforms_)) {
    XR_LOG_ERR("Subroutine uniform {} does not exist !", uniform_name);
    return;
  }

  auto itr_subroutine = find_if(begin(subroutines_), end(subroutines_),
                                [subroutine_name](const auto& sub) {
                                  return subroutine_name == sub.ss_name;
                                });

  if (itr_subroutine == end(subroutines_)) {
    XR_LOG_ERR("Subroutine {} does not exist !", subroutine_name);
    return;
  }

  assert(stage == itr_subroutine->ss_stage);
  itr_unifrm->ssu_assigned_subroutine_idx = itr_subroutine->ss_index;
}

xray::rendering::shader_source_descriptor::shader_source_descriptor(
    const shader_source_file& sfile) noexcept
    : src_type{shader_source_type::file}, s_file{sfile} {}

xray::rendering::shader_source_descriptor::shader_source_descriptor(
    const shader_source_string src_str) noexcept
    : src_type{shader_source_type::code}, s_str{src_str} {}

xray::rendering::scoped_program_handle
xray::rendering::gpu_program_builder::build_program(const GLenum stg) noexcept {
  using namespace xray::base;
  using namespace stlsoft;
  using namespace platformstl;
  using namespace std;

  array<const char*, MAX_SLOTS> src_pointers;
  src_pointers.fill(nullptr);
  array<memory_mapped_file, MAX_SLOTS> mapped_src_files;

  {
    //
    // retrieve pointer to the strings with the source code of the program.
    // strings defined in the config file are placed in front of the
    // string from the shader file.
    auto src_pts_range =
        gsl::span<const char*>{src_pointers.data(), src_pointers.size()};

    auto src_map_range = gsl::span<memory_mapped_file>{
        mapped_src_files.data(),
        static_cast<ptrdiff_t>(mapped_src_files.size())};
    auto src_dsc_range =
        gsl::span<shader_source_descriptor>{&_source_list[0], _sources_count};

    uint32_t idx{};

    for_each(begin(src_dsc_range), end(src_dsc_range),
             [&idx, &src_pts_range](const shader_source_descriptor& ssd) {
               if (ssd.src_type == shader_source_type::code) {
                 src_pts_range[idx++] = ssd.s_str.cstr;
               }
             });

    for_each(begin(src_dsc_range), end(src_dsc_range),
             [&idx, &src_pts_range,
              &src_map_range](const shader_source_descriptor& ssd) {
               if (ssd.src_type == shader_source_type::file) {
                 try {
                   src_map_range[idx] = memory_mapped_file{ssd.s_file.name};
                   src_pts_range[idx] =
                       static_cast<const char*>(src_map_range[idx].memory());
                   ++idx;
                 } catch (const std::exception&) {
                   XR_LOG_CRITICAL("Cannot open shader file {}",
                                   ssd.s_file.name);
                 }
               }
             });
  }

  auto gpu_prg = scoped_program_handle{
      gl::CreateShaderProgramv(stg, _sources_count, src_pointers.data())};
  if (!gpu_prg) {
    return scoped_program_handle{};
  }

  const auto was_linked = [ph = raw_handle(gpu_prg)]() {
    GLint linked_ok{false};
    gl::GetProgramiv(ph, gl::LINK_STATUS, &linked_ok);
    return linked_ok == gl::TRUE_;
  }
  ();

  if (was_linked) {
    return gpu_prg;
  }

  char  log_buff[1024];
  GLint log_len{0};
  gl::GetProgramInfoLog(raw_handle(gpu_prg), XR_I32_COUNTOF__(log_buff),
                        &log_len, log_buff);
  log_buff[log_len] = 0;

  XR_LOG_CRITICAL("Failed to compile/link program, error : [[{}]]",
                  &log_buff[0]);

  return scoped_program_handle{};
}

xray::rendering::scoped_shader_handle
xray::rendering::detail::gpu_program_helpers::create_shader_from_string(
    const GLenum type, const xray::rendering::shader_source_string& sstr) {
  using namespace xray::base;
  using namespace xray::rendering;

  scoped_shader_handle shader{gl::CreateShader(type)};
  gl::ShaderSource(raw_handle(shader), 1, &sstr.cstr, nullptr);
  gl::CompileShader(raw_handle(shader));
  const auto compilation_succeeded = [handle = raw_handle(shader)]() {
    GLint status{gl::FALSE_};
    gl::GetShaderiv(handle, gl::COMPILE_STATUS, &status);
    return status == gl::TRUE_;
  }
  ();

  if (compilation_succeeded) {
    return shader;
  }

  char  err_buff[1024];
  GLint bsize{0};
  gl::GetShaderInfoLog(raw_handle(shader), XR_I32_COUNTOF__(err_buff), &bsize,
                       err_buff);
  err_buff[bsize] = 0;

  XR_LOG_CRITICAL("Shader compilation error:\n [[{}]]", &err_buff[0]);
  return scoped_shader_handle{};
}

bool xray::rendering::detail::gpu_program_helpers::reflect(
    const GLuint program, const GLenum api_subroutine_uniform_interface_name,
    const GLenum              api_subroutine_interface_name,
    gpu_program_reflect_data* refdata) {

  assert(refdata != nullptr);
  assert(refdata->prg_ublocks && refdata->prg_ublocks->empty());
  assert(refdata->prg_uniforms && refdata->prg_uniforms->empty());
  assert(refdata->prg_subroutines && refdata->prg_subroutines->empty());
  assert(refdata->prg_subroutine_uniforms &&
         refdata->prg_subroutine_uniforms->empty());

  if (!collect_uniform_blocks(program, refdata->prg_ublocks,
                              &refdata->prg_datastore_size)) {
    return false;
  }

  if (!collect_uniforms(program, *refdata->prg_ublocks,
                        refdata->prg_uniforms)) {
    return false;
  }

  if (!collect_subroutines_and_uniforms(
          program, api_subroutine_uniform_interface_name,
          api_subroutine_interface_name, refdata->prg_subroutine_uniforms,
          refdata->prg_subroutines)) {
    return false;
  }

  return true;
}

bool xray::rendering::detail::gpu_program_helpers::collect_uniform_blocks(
    const GLuint program, std::vector<uniform_block_t>* blks,
    size_t* datastore_size) {
  using namespace xray::base;
  using namespace std;
  using namespace xray::rendering;

  const auto phandle = program;

  //
  // get number of uniform blocks and max length for uniform block name
  GLint num_uniform_blocks{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM_BLOCK, gl::ACTIVE_RESOURCES,
                            &num_uniform_blocks);

  GLint max_name_len{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM_BLOCK, gl::MAX_NAME_LENGTH,
                            &max_name_len);

  //
  // No uniform blocks
  if (num_uniform_blocks == 0)
    return true;

  if (max_name_len <= 0) {
    XR_LOG_CRITICAL("Failed to obtain uniform block name info !");
    return false;
  }

  //
  // colllect info about the uniforms
  stlsoft::auto_buffer<char, 128> txt_buff{static_cast<size_t>(max_name_len)};
  vector<uniform_block_t> ublocks{};

  for (uint32_t blk_idx = 0;
       blk_idx < static_cast<uint32_t>(num_uniform_blocks); ++blk_idx) {

    gl::GetProgramResourceName(phandle, gl::UNIFORM_BLOCK, blk_idx,
                               txt_buff.size(), nullptr, txt_buff.data());

    //
    // List of properties we need for the uniform blocks.
    const GLuint props_to_get[] = {
        gl::BUFFER_BINDING, gl::BUFFER_DATA_SIZE,
    };

    union ublock_prop_data_t {
      struct {
        int32_t ub_bindpoint;
        int32_t ub_size;
      };
      int32_t components[2];

      ublock_prop_data_t() noexcept { ub_bindpoint = ub_size = -1; }

      explicit operator bool() const noexcept {
        return (ub_bindpoint >= 0) && (ub_size >= 1);
      }

    } u_props;

    static_assert(XR_U32_COUNTOF__(props_to_get) ==
                      XR_U32_COUNTOF__(u_props.components),
                  "Mismatch between the count of requested properties and "
                  "their storage block");

    GLsizei props_retrieved{};

    gl::GetProgramResourceiv(phandle, gl::UNIFORM_BLOCK, blk_idx,
                             XR_U32_COUNTOF__(props_to_get), props_to_get,
                             XR_U32_COUNTOF__(u_props.components),
                             &props_retrieved, u_props.components);

    if ((props_retrieved != XR_U32_COUNTOF__(u_props.components)) || !u_props) {
      XR_LOG_ERR("Failed to retrieve all uniform block properties!");
      return false;
    }

    ublocks.push_back({txt_buff.data(), 0,
                       static_cast<uint32_t>(u_props.ub_size), blk_idx,
                       static_cast<uint32_t>(u_props.ub_bindpoint)});
  }

  //
  // compute number of required bytes for blocks and allocate storage.
  {
    uint32_t ublock_store_bytes_req{};
    for (auto itr_c = begin(ublocks), itr_end = end(ublocks); itr_c != itr_end;
         ++itr_c) {
      itr_c->store_offset = ublock_store_bytes_req;
      ublock_store_bytes_req += itr_c->size;
    }

    *datastore_size = ublock_store_bytes_req;

    //
    // create uniform buffers for active blocks
    constexpr auto kBufferCreateFlags = gl::MAP_WRITE_BIT;
    for_each(begin(ublocks), end(ublocks), [kBufferCreateFlags](auto& blk) {
      gl::CreateBuffers(1, raw_handle_ptr(blk.gl_buff));
      gl::NamedBufferStorage(raw_handle(blk.gl_buff), blk.size, nullptr,
                             kBufferCreateFlags);
    });

    //
    // abort if creation of any buffer failed.
    const auto any_fails = any_of(begin(ublocks), end(ublocks),
                                  [](const auto& blk) { return !blk.gl_buff; });

    if (any_fails) {
      return false;
    }

    //
    // Sort uniform blocks by their name
    sort(begin(ublocks), end(ublocks), [](const auto& blk0, const auto& blk1) {
      return blk0.name < blk1.name;
    });

    //
    // assign state here
    *blks = std::move(ublocks);
  }

  return true;
}

bool xray::rendering::detail::gpu_program_helpers::collect_uniforms(
    const GLuint program, const std::vector<uniform_block_t>& ublocks,
    std::vector<uniform_t>* uniforms) {
  using namespace xray::base;
  using namespace std;
  using namespace stlsoft;

  const auto phandle = program;

  //
  // get number of standalone uniforms and maximum name length
  GLint num_uniforms{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM, gl::ACTIVE_RESOURCES,
                            &num_uniforms);

  //
  // Nothing to do if no standalone uniforms.
  if (num_uniforms == 0)
    return true;

  GLint u_max_len{};
  gl::GetProgramInterfaceiv(phandle, gl::UNIFORM, gl::MAX_NAME_LENGTH,
                            &u_max_len);

  auto_buffer<char, 128> tmp_buff{static_cast<size_t>(u_max_len)};

  for (uint32_t u_idx = 0; u_idx < static_cast<uint32_t>(num_uniforms);
       ++u_idx) {

    gl::GetProgramResourceName(phandle, gl::UNIFORM, u_idx, tmp_buff.size(),
                               nullptr, tmp_buff.data());

    union uniform_props_t {
      struct {
        int32_t u_blk_idx;
        int32_t u_blk_off;
        int32_t u_type;
        int32_t u_arr_dim;
        int32_t u_loc;
        int32_t u_stride;
      };

      int32_t u_components[6];

      uniform_props_t() noexcept {
        u_blk_idx = u_blk_off = u_type = u_arr_dim = u_loc = -1;
      }
    } u_props;

    const GLuint props_to_get[] = {gl::BLOCK_INDEX, gl::OFFSET,
                                   gl::TYPE,        gl::ARRAY_SIZE,
                                   gl::LOCATION,    gl::MATRIX_STRIDE};

    static_assert(
        XR_U32_COUNTOF__(u_props.u_components) ==
            XR_U32_COUNTOF__(props_to_get),
        "Size mismatch between number of properties to get and to store!");

    GLint props_retrieved{0};

    gl::GetProgramResourceiv(phandle, gl::UNIFORM, u_idx,
                             XR_U32_COUNTOF__(props_to_get), props_to_get,
                             XR_U32_COUNTOF__(u_props.u_components),
                             &props_retrieved, u_props.u_components);

    if (props_retrieved != XR_I32_COUNTOF__(props_to_get)) {
      return false;
    }

    auto new_uniform = uniform_t{
        tmp_buff.data(),
        get_resource_size(u_props.u_type) * u_props.u_arr_dim,
        u_props.u_blk_idx == -1 ? 0 : static_cast<uint32_t>(u_props.u_blk_off),
        u_props.u_blk_idx,
        static_cast<uint32_t>(u_props.u_type),
        static_cast<uint32_t>(u_props.u_arr_dim),
        static_cast<uint32_t>(u_props.u_loc),
        static_cast<uint32_t>(u_props.u_stride)};

    //
    // If this uniform is part of a block,
    // find index of the parent block in our list of blocks.
    if (u_props.u_blk_idx != -1) {
      auto parent_blk_itr =
          find_if(begin(ublocks), end(ublocks), [&u_props](const auto& ublk) {
            return ublk.index == static_cast<uint32_t>(u_props.u_blk_idx);
          });

      assert(parent_blk_itr != end(ublocks));

      new_uniform.parent_block_idx =
          std::distance(begin(ublocks), parent_blk_itr);
    }

    uniforms->push_back(new_uniform);
  }

  //
  // sort uniforms by name.
  sort(begin(*uniforms), end(*uniforms),
       [](const auto& u0, const auto& u1) { return u0.name < u1.name; });

  return true;
}

bool xray::rendering::detail::gpu_program_helpers::
    collect_subroutines_and_uniforms(
        const GLuint program,
        const GLenum api_subroutine_uniform_interface_name,
        const GLenum api_subroutine_interface_name,
        std::vector<shader_subroutine_uniform>* subs_uniforms,
        std::vector<shader_subroutine>*         subs) {

  using namespace xray::base;
  using namespace std;

  const auto phandle = program;

  const int32_t active_uniforms = [phandle,
                                   api_subroutine_uniform_interface_name]() {
    GLint cnt{};
    gl::GetProgramInterfaceiv(phandle, api_subroutine_uniform_interface_name,
                              gl::ACTIVE_RESOURCES, &cnt);

    return cnt;
  }();

  //
  // No subroutines
  if (!active_uniforms)
    return true;

  const int32_t max_namelen_subroutine_uniform =
      [phandle, api_subroutine_uniform_interface_name]() {
        GLint maxlen{};
        gl::GetProgramInterfaceiv(phandle,
                                  api_subroutine_uniform_interface_name,
                                  gl::MAX_NAME_LENGTH, &maxlen);

        return maxlen;
      }();

  assert(max_namelen_subroutine_uniform != 0);

  for (int32_t idx = 0; idx < active_uniforms; ++idx) {
    stlsoft::auto_buffer<char> name_buff{
        static_cast<size_t>(max_namelen_subroutine_uniform) + 1};
    GLsizei chars_written{0};
    gl::GetProgramResourceName(phandle, api_subroutine_uniform_interface_name,
                               idx, max_namelen_subroutine_uniform,
                               &chars_written, name_buff.data());
    name_buff.data()[chars_written] = 0;

    const auto uniform_location = gl::GetProgramResourceLocation(
        phandle, api_subroutine_uniform_interface_name, name_buff.data());

    subs_uniforms->push_back({name_buff.data(), graphics_pipeline_stage::last,
                              static_cast<uint8_t>(uniform_location), 0xFF});
  }

  //
  // Sort subroutine uniforms by stage and by location.
  sort(begin(*subs_uniforms), end(*subs_uniforms),
       [](const auto& lhs, const auto& rhs) {
         return lhs.ssu_location < rhs.ssu_location;
       });

  //
  // collect subroutines
  const int32_t subroutines_count = [phandle, api_subroutine_interface_name]() {
    GLint cnt{};
    gl::GetProgramInterfaceiv(phandle, api_subroutine_interface_name,
                              gl::ACTIVE_RESOURCES, &cnt);

    return cnt;
  }();

  if (!subroutines_count)
    return true;

  const int32_t max_namelen_subroutine = [phandle,
                                          api_subroutine_interface_name]() {
    GLint len{};
    gl::GetProgramInterfaceiv(phandle, api_subroutine_interface_name,
                              gl::MAX_NAME_LENGTH, &len);

    return len;
  }();

  assert(max_namelen_subroutine != 0);

  for (int32_t idx = 0; idx < subroutines_count; ++idx) {
    int32_t                    name_len{0};
    stlsoft::auto_buffer<char> name_buff{
        static_cast<size_t>(max_namelen_subroutine) + 1};

    gl::GetProgramResourceName(phandle, api_subroutine_interface_name, idx,
                               max_namelen_subroutine, &name_len,
                               name_buff.data());
    name_buff.data()[name_len] = 0;

    const auto subroutine_index = gl::GetProgramResourceIndex(
        phandle, api_subroutine_interface_name, name_buff.data());

    subs->push_back({name_buff.data(), graphics_pipeline_stage::last,
                     static_cast<uint8_t>(subroutine_index)});
  }

  sort(begin(*subs), end(*subs), [](const auto& lhs, const auto& rhs) {
    return lhs.ss_name < rhs.ss_name;
  });

  return true;
}

void xray::rendering::detail::gpu_program_helpers::set_uniform(
    const GLuint program_id, const GLint u_location,
    const uint32_t uniform_type, const void* u_data,
    const size_t count) noexcept {

  switch (uniform_type) {
  case gl::FLOAT:
    PROG_UNIFORM_FV(program_id, gl::ProgramUniform1fv, u_location, count,
                    u_data);
    break;

  case gl::UNSIGNED_INT:
    PROG_UNIFORM_UIV(program_id, gl::ProgramUniform1uiv, u_location, count,
                     u_data);
    break;

  case gl::FLOAT_VEC2:
    PROG_UNIFORM_FV(program_id, gl::ProgramUniform2fv, u_location, count,
                    u_data);
    break;

  case gl::FLOAT_VEC3:
    PROG_UNIFORM_FV(program_id, gl::ProgramUniform3fv, u_location, count,
                    u_data);
    break;

  case gl::FLOAT_VEC4:
    PROG_UNIFORM_FV(program_id, gl::ProgramUniform4fv, u_location, count,
                    u_data);
    break;

  case gl::FLOAT_MAT2:
    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix2fv, u_location,
                        count, u_data);
    break;

  case gl::FLOAT_MAT2x3:
    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix2x3fv, u_location,
                        count, u_data);
    break;

  case gl::FLOAT_MAT3x2:
    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix3x2fv, u_location,
                        count, u_data);
    break;

  case gl::FLOAT_MAT3:
    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix3fv, u_location,
                        count, u_data);
    break;

  case gl::FLOAT_MAT4:
    PROG_UNIFORM_MAT_FV(program_id, gl::ProgramUniformMatrix4fv, u_location,
                        count, u_data);
    break;

  case gl::INT:
  case gl::SAMPLER_2D:
  case gl::SAMPLER_1D:
  case gl::SAMPLER_CUBE:
    PROG_UNIFORM_IV(program_id, gl::ProgramUniform1iv, u_location, count,
                    u_data);
    break;

  default:
    XR_LOG_CRITICAL("Unhandled uniform type {} in set()", uniform_type);
    assert(false && "Unhandled uniform type in set()");
    break;
  }
}

xray::rendering::detail::gpu_program_base::gpu_program_base(
    scoped_program_handle handle, const GLenum stage,
    const GLenum api_subroutine_uniform_interface_name,
    const GLenum api_subroutine_interface_name)
    : _handle{std::move(handle)}, _stage{stage} {

  if (!_handle)
    return;

  detail::gpu_program_reflect_data reflection_info{
      0u, &uniform_blocks_, &uniforms_, &subroutine_uniforms_, &subroutines_};

  _valid = detail::gpu_program_helpers::reflect(
      base::raw_handle(_handle), api_subroutine_uniform_interface_name,
      api_subroutine_interface_name, &reflection_info);

  if (!_valid)
    return;

  if (reflection_info.prg_datastore_size) {
    base::unique_pointer_reset(ublocks_datastore_,
                               new uint8_t[reflection_info.prg_datastore_size]);
  }
}

xray::rendering::detail::gpu_program_base::~gpu_program_base() {}

xray::rendering::detail::gpu_program_base&
xray::rendering::detail::gpu_program_base::set_uniform_block(
    const char* block_name, const void* block_data, const size_t byte_count) {

  assert(_valid);
  assert(block_name != nullptr);
  assert(block_data != nullptr);

  using namespace std;
  using namespace xray::base;

  auto blk_iter =
      find_if(begin(uniform_blocks_), end(uniform_blocks_),
              [block_name](auto& blk) { return blk.name == block_name; });

  if (blk_iter == end(uniform_blocks_)) {
    XR_LOG_ERR("{} error, uniform {} does not exist", __PRETTY_FUNCTION__,
               block_name);
    return *this;
  }

  assert(byte_count <= blk_iter->size);
  memcpy(raw_ptr(ublocks_datastore_) + blk_iter->store_offset, block_data,
         byte_count);
  blk_iter->dirty = true;

  return *this;
}

xray::rendering::detail::gpu_program_base&
xray::rendering::detail::gpu_program_base::set_subroutine_uniform(
    const char* uniform_name, const char* subroutine_name) noexcept {

  assert(uniform_name != nullptr);
  assert(subroutine_name != nullptr);

  using namespace std;
  auto itr_unifrm = find_if(
      begin(subroutine_uniforms_), end(subroutine_uniforms_),
      [uniform_name](const auto& uf) { return uniform_name == uf.ssu_name; });

  if (itr_unifrm == end(subroutine_uniforms_)) {
    XR_LOG_ERR("Subroutine uniform {} does not exist !", uniform_name);
    return *this;
  }

  auto itr_subroutine = find_if(begin(subroutines_), end(subroutines_),
                                [subroutine_name](const auto& sub) {
                                  return subroutine_name == sub.ss_name;
                                });

  if (itr_subroutine == end(subroutines_)) {
    XR_LOG_ERR("Subroutine {} does not exist !", subroutine_name);
    return *this;
  }

  itr_unifrm->ssu_assigned_subroutine_idx = itr_subroutine->ss_index;
  return *this;
}

void xray::rendering::detail::gpu_program_base::use() {
  assert(_valid);

  using namespace std;
  using namespace xray::base;

  //
  // writa data for blocks marked as dirty
  for_each(begin(uniform_blocks_), end(uniform_blocks_), [this](auto& u_blk) {
    if (u_blk.dirty) {

      auto src_ptr = raw_ptr(ublocks_datastore_) + u_blk.store_offset;

      {
        scoped_resource_mapping ubuff_mapping{raw_handle(u_blk.gl_buff),
                                              gl::MAP_WRITE_BIT, u_blk.size};

        if (!ubuff_mapping)
          return;

        memcpy(ubuff_mapping.memory(), src_ptr, u_blk.size);
      }
      u_blk.dirty = false;
    }

    gl::BindBuffer(gl::UNIFORM_BUFFER, raw_handle(u_blk.gl_buff));
    gl::BindBufferBase(gl::UNIFORM_BUFFER, u_blk.bindpoint,
                       raw_handle(u_blk.gl_buff));

  });

  if (!subroutine_uniforms_.empty()) {
    stlsoft::auto_buffer<GLuint> indices_buff{subroutine_uniforms_.size()};

    for_each(
        begin(subroutine_uniforms_), end(subroutine_uniforms_),
        [indices = gsl::span<GLuint>(indices_buff.data(), indices_buff.size())](
            const auto& sub_unifrm) {
          indices[sub_unifrm.ssu_location] =
              sub_unifrm.ssu_assigned_subroutine_idx;
        });

    //
    // For subroutine uniforms, program must be set active
    // gl::UseProgram(raw_handle(_handle));
    gl::UniformSubroutinesuiv(_stage, indices_buff.size(), indices_buff.data());
  }
}

void xray::rendering::detail::gpu_program_base::swap(
    gpu_program_base& rhs) noexcept {

  xray::base::swap(_handle, rhs._handle);
  std::swap(uniform_blocks_, rhs.uniform_blocks_);
  std::swap(uniforms_, rhs.uniforms_);
  xray::base::swap(ublocks_datastore_, rhs.ublocks_datastore_);
  std::swap(subroutine_uniforms_, rhs.subroutine_uniforms_);
  std::swap(subroutines_, rhs.subroutines_);
  std::swap(_valid, rhs._valid);
  std::swap(_stage, rhs._stage);
}

xray::rendering::gpu_program_pipeline_setup_builder::
    gpu_program_pipeline_setup_builder(const GLuint pipeline_handle) noexcept
    : _handle{pipeline_handle} {
  memset(_programs_by_stage, 0, sizeof(_programs_by_stage));
}

xray::rendering::gpu_program_pipeline_setup_builder&
xray::rendering::gpu_program_pipeline_setup_builder::add_vertex_program(
    const vertex_program& vert_prg) {
  using namespace xray::base;

  _programs_by_stage[enum_helper::to_underlying_type(
      graphics_pipeline_stage::vertex)] = &vert_prg;
  return *this;
}

xray::rendering::gpu_program_pipeline_setup_builder&
xray::rendering::gpu_program_pipeline_setup_builder::add_geometry_program(
    const geometry_program& geom_prg) {
  using namespace xray::base;

  _programs_by_stage[enum_helper::to_underlying_type(
      graphics_pipeline_stage::geometry)] = &geom_prg;
  return *this;
}

xray::rendering::gpu_program_pipeline_setup_builder&
xray::rendering::gpu_program_pipeline_setup_builder::add_fragment_program(
    const fragment_program& frag_prg) {
  using namespace xray::base;

  _programs_by_stage[enum_helper::to_underlying_type(
      graphics_pipeline_stage::fragment)] = &frag_prg;
  return *this;
}

xray::rendering::gpu_program_pipeline_setup_builder&
xray::rendering::gpu_program_pipeline_setup_builder::add_tess_control_program(
    const tess_control_program& tess_ctrl_prg) {
  using namespace xray::base;

  _programs_by_stage[enum_helper::to_underlying_type(
      graphics_pipeline_stage::fragment)] = &tess_ctrl_prg;
  return *this;
}

xray::rendering::gpu_program_pipeline_setup_builder&
xray::rendering::gpu_program_pipeline_setup_builder::add_tess_eval_program(
    const tess_eval_program& tess_eval_prg) {
  using namespace xray::base;

  _programs_by_stage[enum_helper::to_underlying_type(
      graphics_pipeline_stage::fragment)] = &tess_eval_prg;
  return *this;
}

void xray::rendering::gpu_program_pipeline_setup_builder::install() {
  assert(gl::IsProgramPipeline(_handle));

  using namespace xray::base;

  for (auto i = enum_helper::to_underlying_type(graphics_pipeline_stage::first);
       i < enum_helper::to_underlying_type(graphics_pipeline_stage::last);
       ++i) {

    if (_programs_by_stage[i] == nullptr)
      continue;

    const auto stage_bit = [stage = enum_helper::from_underlying_type<
                                graphics_pipeline_stage>(i)]() {
#define XRAY_TO_OPENGL_GET_STAGE_BIT(stg)                                      \
  case stg:                                                                    \
    return xray_to_opengl<stg>::shader_bit;                                    \
    break

      switch (stage) {
        XRAY_TO_OPENGL_GET_STAGE_BIT(graphics_pipeline_stage::vertex);
        XRAY_TO_OPENGL_GET_STAGE_BIT(graphics_pipeline_stage::tess_eval);
        XRAY_TO_OPENGL_GET_STAGE_BIT(graphics_pipeline_stage::tess_control);
        XRAY_TO_OPENGL_GET_STAGE_BIT(graphics_pipeline_stage::geometry);
        XRAY_TO_OPENGL_GET_STAGE_BIT(graphics_pipeline_stage::fragment);
        XRAY_TO_OPENGL_GET_STAGE_BIT(graphics_pipeline_stage::compute);

      default:
        assert(false && "Unmapped stage bit !");
        break;
      };

      return GLbitfield{gl::INVALID_VALUE};
    }
    ();

    gl::UseProgramStages(_handle, stage_bit, _programs_by_stage[i]->handle());
  }

  gl::UseProgram(0);
  gl::BindProgramPipeline(_handle);
}
