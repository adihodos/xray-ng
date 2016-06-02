#include "xray/rendering/mesh.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/objects/sphere_math.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/opengl/scoped_state.hpp"
#include "xray/rendering/vertex_format/vertex_pn.hpp"
#include "xray/rendering/vertex_format/vertex_pnt.hpp"
#include "xray/rendering/vertex_format/vertex_pntt.hpp"
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <span.h>
#include <tbb/tbb.h>
#include <vector>

using namespace std;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

struct malloc_deleter {
  void operator()(void* ptr) const noexcept { free(ptr); }
};

struct vertex_load_option {
  enum { load_normals = 1u, load_texcoord = 1u << 1, load_tangents = 1u << 2 };
};

float3 ai_vector_to_xray_vector(const aiVector3D& input) noexcept {
  return {input.x, input.y, input.z};
}

float2 ai_vector_to_xray_vector(const aiVector2D& input) noexcept {
  return {input.x, input.y};
}

template <typename OutVec, typename InVec>
struct vector_cast_impl;

template <>
struct vector_cast_impl<xray::math::float3, aiVector3D> {
  static xray::math::float3 cast(const aiVector3D& vec) noexcept {
    return {vec.x, vec.y, vec.z};
  }
};

template <>
struct vector_cast_impl<xray::math::float2, aiVector3D> {
  static xray::math::float2 cast(const aiVector3D& vec) noexcept {
    return {vec.x, vec.y};
  }
};

template <typename OutputVectorType, typename InputVectorType>
inline OutputVectorType vector_cast(const InputVectorType& input_vec) noexcept {
  return vector_cast_impl<OutputVectorType, InputVectorType>::cast(input_vec);
}

struct vertex_format_info {
  uint32_t                        components;
  size_t                          element_size;
  const vertex_format_entry_desc* description;
};

template <xray::rendering::vertex_format vfmt>
vertex_format_info                       describe_vertex_format() {
  using fmt_traits = vertex_format_traits<vfmt>;
  return {fmt_traits::components, fmt_traits::bytes_size,
          fmt_traits::description()};
}

auto get_vertex_format_description(const xray::rendering::vertex_format fmt) {
  switch (fmt) {
  case vertex_format::pn:
    return describe_vertex_format<vertex_format::pn>();
    break;

  case vertex_format::pnt:
    return describe_vertex_format<vertex_format::pnt>();
    break;

  case vertex_format::pntt:
    return describe_vertex_format<vertex_format::pntt>();
    break;

  default:
    assert(false && "Unsupported vertex format!");
    break;
  }

  return vertex_format_info{};
}

template <typename In, typename Out>
struct loader_func {
  static Out load(const In* input, const uint32_t idx) noexcept {
    return vector_cast<Out>(input[idx]);
  }

  static Out load_null(const In*, const uint32_t) noexcept {
    return Out::stdc::zero;
  }
};

template <typename InputVertexType, typename OutputVertexType>
static void mesh_load_vertex_data(const InputVertexType* input_vertices,
                                  const uint32_t         num_input_vertices,
                                  const uint32_t         output_stride,
                                  const uint32_t         output_base_vertex,
                                  OutputVertexType*      output_vertices) {
  using loader_func_type = loader_func<InputVertexType, OutputVertexType>;
  auto load_fn =
      input_vertices ? &loader_func_type::load : &loader_func_type::load_null;

  for (uint32_t idx = 0; idx < num_input_vertices; ++idx) {
    auto out_vert = reinterpret_cast<OutputVertexType*>(
        reinterpret_cast<uint8_t*>(output_vertices) +
        (idx + output_base_vertex) * output_stride);

    *out_vert = load_fn(input_vertices, idx);
  }
}

static void mesh_load_pos(float3* where, const uint32_t stride,
                          const aiMesh* mesh, const uint32_t base_vertex) {
  mesh_load_vertex_data(mesh->mVertices, mesh->mNumVertices, stride,
                        base_vertex, where);
}

static void mesh_load_normal(float3* where, const uint32_t stride,
                             const aiMesh* mesh, const uint32_t base_vertex) {
  mesh_load_vertex_data(mesh->mNormals, mesh->mNumVertices, stride, base_vertex,
                        where);
}

static void mesh_load_texcoord(float2* where, const uint32_t stride,
                               const aiMesh* mesh, const uint32_t base_vertex) {
  mesh_load_vertex_data(mesh->mTextureCoords[0], mesh->mNumVertices, stride,
                        base_vertex, where);
}

static void mesh_load_tangent(float3* where, const uint32_t stride,
                              const aiMesh* mesh, const uint32_t base_vertex) {
  mesh_load_vertex_data(mesh->mTangents, mesh->mNumVertices, stride,
                        base_vertex, where);
}

static void mesh_load_vertex_pn(void* output, const aiMesh* mesh,
                                const uint32_t base_vertex) {
  auto           dst    = static_cast<vertex_pn*>(output);
  constexpr auto stride = static_cast<uint32_t>(sizeof(vertex_pn));
  mesh_load_pos(&dst->position, stride, mesh, base_vertex);
  mesh_load_normal(&dst->normal, stride, mesh, base_vertex);
}

static void mesh_load_vertex_pnt(void* where, const aiMesh* mesh,
                                 const uint32_t base_vertex) {
  auto           dst    = static_cast<vertex_pnt*>(where);
  constexpr auto stride = static_cast<uint32_t>(sizeof(vertex_pnt));

  mesh_load_pos(&dst->position, stride, mesh, base_vertex);
  mesh_load_normal(&dst->normal, stride, mesh, base_vertex);
  mesh_load_texcoord(&dst->texcoord, stride, mesh, base_vertex);
}

void mesh_load_vertex_pntt(void* where, const aiMesh* mesh,
                           const uint32_t base_vertex) {
  auto           dst    = static_cast<vertex_pntt*>(where);
  constexpr auto stride = static_cast<uint32_t>(sizeof(vertex_pntt));

  mesh_load_pos(&dst->position, stride, mesh, base_vertex);
  mesh_load_normal(&dst->normal, stride, mesh, base_vertex);
  mesh_load_texcoord(&dst->texcoords, stride, mesh, base_vertex);
  mesh_load_tangent(&dst->tangent, stride, mesh, base_vertex);
}

template <typename IndexType>
void mesh_load_face_indices(void* output, const uint32_t output_offset,
                            const uint32_t input_offset, const aiFace* face) {
  auto dst = static_cast<IndexType*>(output) + output_offset;
  for (uint32_t i = 0; i < face->mNumIndices; ++i) {
    dst[i] = static_cast<IndexType>(face->mIndices[i] + input_offset);
  }
}

bool xray::rendering::simple_mesh::load_model_impl(
    const char* model_data, const size_t data_size,
    const uint32_t mesh_process_opts, const uint32_t mesh_import_opts) {

  struct ai_propstore_deleter {
    void operator()(aiPropertyStore* prop_store) const noexcept {
      if (prop_store)
        aiReleasePropertyStore(prop_store);
    }
  };

  unique_pointer<aiPropertyStore, ai_propstore_deleter> import_props{
      aiCreatePropertyStore()};

  if (!import_props) {
    XR_LOG_ERR("Failed to create Assimp property store object!");
    return false;
  }

  {
    aiSetImportPropertyInteger(raw_ptr(import_props),
                               AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);

    if (mesh_import_opts & mesh_load_option::remove_points_lines) {
      aiSetImportPropertyInteger(raw_ptr(import_props), AI_CONFIG_PP_SBP_REMOVE,
                                 aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    }
  }

  auto imported_scene = aiImportFileFromMemoryWithProperties(
      model_data, data_size, mesh_process_opts, nullptr, raw_ptr(import_props));

  if (!imported_scene) {
    XR_LOG_ERR("Assimp import error : failed to load scene from model file !");
    return false;
  }

  size_t num_vertices = 0;
  size_t num_indices  = 0;
  size_t num_faces    = 0;

  //
  //  Count vertices and indices.
  for (uint32_t mesh_index = 0; mesh_index < imported_scene->mNumMeshes;
       ++mesh_index) {

    const aiMesh* curr_mesh = imported_scene->mMeshes[mesh_index];
    num_vertices += curr_mesh->mNumVertices;
    num_faces += curr_mesh->mNumFaces;

    for (uint32_t face_index = 0; face_index < curr_mesh->mNumFaces;
         ++face_index) {
      const aiFace* curr_face = &curr_mesh->mFaces[face_index];
      num_indices += curr_face->mNumIndices;
    }
  }

  _indexcount = num_indices;

  auto buffer_alloc_fn = [ num_vertices, fmt = _vertexformat ]()->void* {
    switch (fmt) {
    case vertex_format::pn:
      return malloc(num_vertices * sizeof(vertex_pn));
      break;

    case vertex_format::pnt:
      return malloc(num_vertices * sizeof(vertex_pnt));
      break;

    case vertex_format::pntt:
      return malloc(num_vertices * sizeof(vertex_pntt));
      break;

    default:
      assert(false && "Unsupported vertex format!");
      break;
    };

    return nullptr;
  };

  unique_pointer<void, malloc_deleter> imported_geometry{buffer_alloc_fn()};
  if (num_indices > 65535)
    _indexformat = index_format::u32;

  constexpr size_t INDEX_ELEMENT_SIZE[] = {sizeof(uint16_t), sizeof(uint32_t)};
  const auto       index_buffer_bytes_size =
      num_indices * INDEX_ELEMENT_SIZE[_indexformat == index_format::u32];

  unique_pointer<void, malloc_deleter> indices_buff{
      malloc(index_buffer_bytes_size)};

  uint32_t base_vertex{};
  uint32_t input_base_index{};
  uint32_t output_base_index{};

  auto load_index_func = _indexformat == index_format::u32
                             ? &mesh_load_face_indices<uint32_t>
                             : &mesh_load_face_indices<uint16_t>;

  for (uint32_t mesh_index = 0; mesh_index < imported_scene->mNumMeshes;
       ++mesh_index) {
    const aiMesh* curr_mesh = imported_scene->mMeshes[mesh_index];

    if (!curr_mesh->mVertices)
      continue;

    switch (_vertexformat) {
    case vertex_format::pn:
      mesh_load_vertex_pn(raw_ptr(imported_geometry), curr_mesh, base_vertex);
      break;

    case vertex_format::pnt:
      mesh_load_vertex_pnt(raw_ptr(imported_geometry), curr_mesh, base_vertex);
      break;

    case vertex_format::pntt:
      mesh_load_vertex_pntt(raw_ptr(imported_geometry), curr_mesh, base_vertex);
      break;

    default:
      assert(false && "Unsupported vertex format!");
      break;
    }

    //
    //  Collect indices defining primitives.
    for (uint32_t face_index = 0; face_index < curr_mesh->mNumFaces;
         ++face_index) {
      const aiFace* curr_face = &curr_mesh->mFaces[face_index];

      for (uint32_t i = 0; i < curr_face->mNumIndices; ++i) {
        load_index_func(raw_ptr(indices_buff), output_base_index,
                        input_base_index, curr_face);
      }

      output_base_index += curr_face->mNumIndices;
    }

    input_base_index += curr_mesh->mNumVertices;
    base_vertex += curr_mesh->mNumVertices;
  }

  const auto fmt_desc = [fmt = _vertexformat]() {
    switch (fmt) {
    case vertex_format::pn:
      return describe_vertex_format<vertex_format::pn>();
      break;

    case vertex_format::pnt:
      return describe_vertex_format<vertex_format::pnt>();
      break;

    case vertex_format::pntt:
      return describe_vertex_format<vertex_format::pntt>();
      break;

    default:
      assert(false && "Unsupported vertex format!");
      break;
    }

    return vertex_format_info{};
  }
  ();

  _vertexbuffer =
      [ buffdata = raw_ptr(imported_geometry), num_vertices, &fmt_desc ]() {
    GLuint vbuff{};
    gl::CreateBuffers(1, &vbuff);
    gl::NamedBufferStorage(vbuff, num_vertices * fmt_desc.element_size,
                           buffdata, 0);

    return vbuff;
  }
  ();

  _indexbuffer =
      [ buffdata = raw_ptr(indices_buff), index_buffer_bytes_size ]() {
    GLuint ibuff{};
    gl::CreateBuffers(1, &ibuff);
    gl::NamedBufferStorage(ibuff, index_buffer_bytes_size, buffdata, 0);

    return ibuff;
  }
  ();

  create_vertexarray();

  _boundingbox = [
    geom = raw_ptr(imported_geometry), num_vertices,
    fmt  = get_vertex_format_description(_vertexformat)
  ]() {
    return bounding_box3_axis_aligned(static_cast<const float3*>(geom),
                                      num_vertices, fmt.element_size);
  }
  ();

  return true;
}

xray::rendering::simple_mesh::simple_mesh(const vertex_format fmt,
                                          const char*         mesh_file,
                                          const uint32_t      load_options)
    : _vertexformat{fmt} {

  constexpr uint32_t default_processing_opts =
      aiProcess_CalcTangentSpace |         // calculate tangents and
      aiProcess_JoinIdenticalVertices |    // join identical vertices/
      aiProcess_ValidateDataStructure |    // perform a full validation of the
      aiProcess_ImproveCacheLocality |     // improve the cache locality of
      aiProcess_RemoveRedundantMaterials | // remove redundant materials
      aiProcess_FindDegenerates |   // remove degenerated polygons from the
      aiProcess_FindInvalidData |   // detect invalid model data, such as
      aiProcess_GenUVCoords |       // convert spherical, cylindrical, box and
      aiProcess_TransformUVCoords | // preprocess UV transformations
      aiProcess_FindInstances |     // search for instanced meshes and remove
      aiProcess_LimitBoneWeights |  // limit bone weights to 4 per vertex
      aiProcess_OptimizeMeshes |    // join small meshes, if possible;
      aiProcess_SplitByBoneCount |  // split meshes with too many bones.
      0;

  constexpr auto post_processing_opts =
      default_processing_opts | aiProcess_GenSmoothNormals | // generate smooth
      aiProcess_SplitLargeMeshes |                           // split large,
      aiProcess_Triangulate | // triangulate polygons with
      aiProcess_SortByPType | // make 'clean' meshes which consist of a
      0;

  const auto all_processing_opts =
      post_processing_opts |
      (load_options & mesh_load_option::convert_left_handed
           ? aiProcess_ConvertToLeftHanded
           : 0);

  try {
    platformstl::memory_mapped_file mesh_mmfile{mesh_file};
    _valid =
        load_model_impl(static_cast<const char*>(mesh_mmfile.memory()),
                        mesh_mmfile.size(), all_processing_opts, load_options);
  } catch (const std::exception&) {
    XR_LOG_ERR("Failed to import model file {}", mesh_file);
  }
}

void xray::rendering::simple_mesh::draw() {
  assert(valid());

  scoped_vertex_array_binding vao_binding{raw_handle(_vertexarray)};
  XR_UNUSED_ARG(vao_binding);

  const GLuint element_type[] = {gl::UNSIGNED_SHORT, gl::UNSIGNED_INT};
  gl::DrawElements(gl::TRIANGLES, _indexcount,
                   element_type[_indexformat == index_format::u32], nullptr);
}

template <typename OutputFormatType, typename InputFormatType>
struct format_cast_impl;

template <>
struct format_cast_impl<xray::rendering::vertex_pn,
                        xray::rendering::vertex_pntt> {
  static xray::rendering::vertex_pn
  cast(const xray::rendering::vertex_pntt& vs_in) {
    return {vs_in.position, vs_in.normal};
  }
};

template <>
struct format_cast_impl<xray::rendering::vertex_pnt,
                        xray::rendering::vertex_pntt> {
  static xray::rendering::vertex_pnt
  cast(const xray::rendering::vertex_pntt& vs_in) {
    return {vs_in.position, vs_in.normal, vs_in.texcoords};
  }
};

template <typename OutputFormatType, typename InputFormatType>
OutputFormatType format_cast(const InputFormatType& vs_in) {
  return format_cast_impl<OutputFormatType, InputFormatType>::cast(vs_in);
}

template <typename OutputFormatType>
void copy_geometry(void* dest, const geometry_data_t& geometry) {
  const auto input_span = gsl::span<const vertex_pntt>{
      raw_ptr(geometry.geometry),
      static_cast<ptrdiff_t>(geometry.vertex_count)};

  auto out = static_cast<OutputFormatType*>(dest);
  for (const auto& vs_in : input_span)
    *out++ = format_cast<OutputFormatType>(vs_in);
}

xray::rendering::simple_mesh::simple_mesh(const vertex_format    fmt,
                                          const geometry_data_t& geometry)
    : _vertexformat{fmt}
    , _indexformat{index_format::u32}
    , _indexcount{static_cast<uint32_t>(geometry.index_count)} {

  const auto fmt_desc = get_vertex_format_description(_vertexformat);
  unique_pointer<void, malloc_deleter> vbuff_data;
  void*      buffer_init_data{nullptr};
  GLsizeiptr buffer_bytes{};

  if (fmt != vertex_format::pntt) {
    buffer_bytes =
        static_cast<GLsizeiptr>(geometry.vertex_count * fmt_desc.element_size);
    vbuff_data = unique_pointer<void, malloc_deleter>(malloc(buffer_bytes));

    buffer_init_data = raw_ptr(vbuff_data);

    switch (fmt) {
    case vertex_format::pn:
      copy_geometry<vertex_pn>(buffer_init_data, geometry);
      break;

    case vertex_format::pnt:
      copy_geometry<vertex_pnt>(buffer_init_data, geometry);
      break;

    default:
      assert(false && "Unsupported vertex format !");
      break;
    }

  } else {
    buffer_bytes =
        static_cast<GLsizeiptr>(geometry.vertex_count * sizeof(vertex_pntt));
    buffer_init_data = raw_ptr(geometry.geometry);
  }

  _vertexbuffer = [buffer_bytes, buffer_init_data]() {
    GLuint vbuff{};
    gl::CreateBuffers(1, &vbuff);
    gl::NamedBufferStorage(vbuff, buffer_bytes, buffer_init_data, 0);
    return vbuff;
  }();

  _indexbuffer = [
    data     = raw_ptr(geometry.indices),
    bytesize = geometry.index_count * sizeof(uint32_t)
  ]() {
    GLuint vbuff{};
    gl::CreateBuffers(1, &vbuff);
    gl::NamedBufferStorage(vbuff, static_cast<GLsizeiptr>(bytesize), data, 0);
    return vbuff;
  }
  ();

  create_vertexarray();

  _valid = true;
}

static constexpr GLuint COMPONENT_TYPES[] = {
    gl::BYTE,           ///< 8 bits int, signed
    gl::UNSIGNED_BYTE,  ///< 8 bits int, unsigned
    gl::SHORT,          ///< 16 bits int, signed
    gl::UNSIGNED_SHORT, ///< 16 bits int, unsigned
    gl::INT,            ///< 32 bits int, signed
    gl::UNSIGNED_INT,   ///< 32 bits int, unsigned
    gl::FLOAT,          ///< 32 bits, simple precision floating point
    gl::DOUBLE          ///< 64 bits, double precision floating point
};

struct helpers {
  static GLuint map_component_type(const uint32_t comp_type) noexcept {
    assert(comp_type < XR_U32_COUNTOF__(COMPONENT_TYPES));
    return COMPONENT_TYPES[comp_type];
  }
};

void xray::rendering::simple_mesh::create_vertexarray() {
  _vertexarray = [
    vb = raw_handle(_vertexbuffer), ib = raw_handle(_indexbuffer),
    fmt_desc = get_vertex_format_description(_vertexformat)
  ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ib);
    gl::VertexArrayVertexBuffer(vao, 0, vb, 0,
                                static_cast<GLsizei>(fmt_desc.element_size));

    for (uint32_t idx = 0; idx < fmt_desc.components; ++idx) {
      const auto& component_desc = fmt_desc.description[idx];
      gl::EnableVertexArrayAttrib(vao, idx);
      gl::VertexArrayAttribFormat(
          vao, idx, static_cast<GLint>(component_desc.component_count),
          helpers::map_component_type(component_desc.component_type),
          gl::FALSE_, component_desc.component_offset);
      gl::VertexArrayAttribBinding(vao, idx, 0);
    }

    gl::BindVertexArray(0);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, 0);
    return vao;
  }
  ();
}
