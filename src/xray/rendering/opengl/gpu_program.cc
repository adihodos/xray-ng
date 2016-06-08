#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/string.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/scoped_resource_mapping.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <gsl.h>
#include <numeric>
#include <span.h>
#include <stlsoft/memory/auto_buffer.hpp>
#include <string>
#include <unordered_map>

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

const char*
pipeline_stage_to_string(const xray::rendering::pipeline_stage ifc) noexcept {
  switch (ifc) {
    CASE_TO_STRING(xray::rendering::pipeline_stage::vertex);
    CASE_TO_STRING(xray::rendering::pipeline_stage::geometry);
    CASE_TO_STRING(xray::rendering::pipeline_stage::fragment);
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

xray::rendering::pipeline_stage
gl_stage_to_pipeline_stage(const uint32_t stage) noexcept {
  switch (stage) {
  case gl::VERTEX_SUBROUTINE:
    return xray::rendering::pipeline_stage::vertex;
    break;

  case gl::FRAGMENT_SUBROUTINE:
    return xray::rendering::pipeline_stage::fragment;
    break;

  case gl::GEOMETRY_SUBROUTINE:
    return xray::rendering::pipeline_stage::geometry;
    break;

  default:
    break;
  }

  return xray::rendering::pipeline_stage::last;
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

void xray::rendering::set_uniform_impl(const GLuint   program_id,
                                       const GLint    u_location,
                                       const uint32_t uniform_type,
                                       const void*    u_data,
                                       const size_t   count) noexcept {
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
      OUTPUT_DBG_MSG("Failed to get all uniform block properties !");
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
    const pipeline_stage stage, const char* uniform_name,
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
