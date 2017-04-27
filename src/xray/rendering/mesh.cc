#include "xray/rendering/mesh.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/enum_cast.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/math/constants.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/objects/aabb3_math.hpp"
#include "xray/math/objects/sphere_math.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar3_math.hpp"
#include "xray/rendering/geometry/geometry_data.hpp"
#include "xray/rendering/geometry/geometry_factory.hpp"
#include "xray/rendering/opengl/scoped_state.hpp"
#include "xray/rendering/vertex_format/vertex_p.hpp"
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
#include <vector>

using namespace std;
using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;

static constexpr GLenum COMPONENT_TYPES_GL[] = {gl::BYTE,
                                                gl::UNSIGNED_BYTE,
                                                gl::SHORT,
                                                gl::UNSIGNED_SHORT,
                                                gl::INT,
                                                gl::UNSIGNED_INT,
                                                gl::FLOAT,
                                                gl::DOUBLE};

static GLenum translate_component_type(const uint32_t ctype) {
  assert(ctype < XR_U32_COUNTOF__(COMPONENT_TYPES_GL));
  return COMPONENT_TYPES_GL[ctype];
}

struct vertex_load_option {
  enum { load_normals = 1u, load_texcoord = 1u << 1, load_tangents = 1u << 2 };
};

vec3f ai_vector_to_xray_vector(const aiVector3D& input) noexcept {
  return {input.x, input.y, input.z};
}

vec2f ai_vector_to_xray_vector(const aiVector2D& input) noexcept {
  return {input.x, input.y};
}

template <typename OutVec, typename InVec>
struct vector_cast_impl;

template <>
struct vector_cast_impl<xray::math::vec3f, aiVector3D> {
  static xray::math::vec3f cast(const aiVector3D& vec) noexcept {
    return {vec.x, vec.y, vec.z};
  }
};

template <>
struct vector_cast_impl<xray::math::vec2f, aiVector3D> {
  static xray::math::vec2f cast(const aiVector3D& vec) noexcept {
    return {vec.x, vec.y};
  }
};

template <typename OutputVectorType, typename InputVectorType>
inline OutputVectorType vector_cast(const InputVectorType& input_vec) noexcept {
  return vector_cast_impl<OutputVectorType, InputVectorType>::cast(input_vec);
}

template <xray::rendering::vertex_format vfmt>
vertex_format_info                       describe_vertex_format() {
  using fmt_traits = vertex_format_traits<vfmt>;
  return {
    fmt_traits::components, fmt_traits::bytes_size, fmt_traits::description()};
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

  case vertex_format::p:
    return describe_vertex_format<vertex_format::p>();
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

static void mesh_load_pos(vec3f*         where,
                          const uint32_t stride,
                          const aiMesh*  mesh,
                          const uint32_t base_vertex) {
  mesh_load_vertex_data(
    mesh->mVertices, mesh->mNumVertices, stride, base_vertex, where);
}

static void mesh_load_normal(vec3f*         where,
                             const uint32_t stride,
                             const aiMesh*  mesh,
                             const uint32_t base_vertex) {
  mesh_load_vertex_data(
    mesh->mNormals, mesh->mNumVertices, stride, base_vertex, where);
}

static void mesh_load_texcoord(vec2f*         where,
                               const uint32_t stride,
                               const aiMesh*  mesh,
                               const uint32_t base_vertex) {
  mesh_load_vertex_data(
    mesh->mTextureCoords[0], mesh->mNumVertices, stride, base_vertex, where);
}

static void mesh_load_tangent(vec3f*         where,
                              const uint32_t stride,
                              const aiMesh*  mesh,
                              const uint32_t base_vertex) {
  mesh_load_vertex_data(
    mesh->mTangents, mesh->mNumVertices, stride, base_vertex, where);
}

static void mesh_load_vertex_pn(void*          output,
                                const aiMesh*  mesh,
                                const uint32_t base_vertex) {
  auto           dst    = static_cast<vertex_pn*>(output);
  constexpr auto stride = static_cast<uint32_t>(sizeof(vertex_pn));
  mesh_load_pos(&dst->position, stride, mesh, base_vertex);
  mesh_load_normal(&dst->normal, stride, mesh, base_vertex);
}

static void mesh_load_vertex_pnt(void*          where,
                                 const aiMesh*  mesh,
                                 const uint32_t base_vertex) {
  auto           dst    = static_cast<vertex_pnt*>(where);
  constexpr auto stride = static_cast<uint32_t>(sizeof(vertex_pnt));

  mesh_load_pos(&dst->position, stride, mesh, base_vertex);
  mesh_load_normal(&dst->normal, stride, mesh, base_vertex);
  mesh_load_texcoord(&dst->texcoord, stride, mesh, base_vertex);
}

void mesh_load_vertex_pntt(void*          where,
                           const aiMesh*  mesh,
                           const uint32_t base_vertex) {
  auto           dst    = static_cast<vertex_pntt*>(where);
  constexpr auto stride = static_cast<uint32_t>(sizeof(vertex_pntt));

  mesh_load_pos(&dst->position, stride, mesh, base_vertex);
  mesh_load_normal(&dst->normal, stride, mesh, base_vertex);
  mesh_load_texcoord(&dst->texcoords, stride, mesh, base_vertex);
  mesh_load_tangent(&dst->tangent, stride, mesh, base_vertex);
}

template <typename IndexType>
void mesh_load_face_indices(void*          output,
                            const uint32_t output_offset,
                            const uint32_t input_offset,
                            const aiFace*  face) {
  auto dst = static_cast<IndexType*>(output) + output_offset;
  for (uint32_t i = 0; i < face->mNumIndices; ++i) {
    dst[i] = static_cast<IndexType>(face->mIndices[i] + input_offset);
  }
}

bool xray::rendering::simple_mesh::load_model_impl(
  const char*    model_data,
  const size_t   data_size,
  const uint32_t mesh_process_opts,
  const uint32_t mesh_import_opts) {
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
    aiSetImportPropertyInteger(
      raw_ptr(import_props), AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);

    if (mesh_import_opts & mesh_load_option::remove_points_lines) {
      aiSetImportPropertyInteger(raw_ptr(import_props),
                                 AI_CONFIG_PP_SBP_REMOVE,
                                 aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    }
  }

  auto imported_scene = aiImportFileFromMemoryWithProperties(
    model_data, data_size, mesh_process_opts, nullptr, raw_ptr(import_props));

  if (!imported_scene) {
    XR_LOG_ERR("Assimp import error : failed to load scene from model file !");
    return false;
  }

  size_t num_faces = 0;

  //
  //  Count vertices and indices.
  for (uint32_t mesh_index = 0; mesh_index < imported_scene->mNumMeshes;
       ++mesh_index) {
    const aiMesh* curr_mesh = imported_scene->mMeshes[mesh_index];
    _vertexcount += curr_mesh->mNumVertices;
    num_faces += curr_mesh->mNumFaces;

    for (uint32_t face_index = 0; face_index < curr_mesh->mNumFaces;
         ++face_index) {
      const aiFace* curr_face = &curr_mesh->mFaces[face_index];
      _indexcount += curr_face->mNumIndices;
    }
  }

  _vertex_format_info    = get_vertex_format_description(_vertexformat);
  const auto vbuff_bytes = _vertex_format_info.element_size * _vertexcount;

  _vertices = unique_pointer<void, malloc_deleter>{malloc(vbuff_bytes)};
  if (_indexcount > 65535)
    _indexformat = index_format::u32;

  constexpr size_t INDEX_ELEMENT_SIZE[] = {sizeof(uint16_t), sizeof(uint32_t)};
  const auto       index_buffer_bytes_size =
    _indexcount * INDEX_ELEMENT_SIZE[_indexformat == index_format::u32];

  _indices =
    unique_pointer<void, malloc_deleter>{malloc(index_buffer_bytes_size)};

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
      mesh_load_vertex_pn(raw_ptr(_vertices), curr_mesh, base_vertex);
      break;

    case vertex_format::pnt:
      mesh_load_vertex_pnt(raw_ptr(_vertices), curr_mesh, base_vertex);
      break;

    case vertex_format::pntt:
      mesh_load_vertex_pntt(raw_ptr(_vertices), curr_mesh, base_vertex);
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
        load_index_func(
          raw_ptr(_indices), output_base_index, input_base_index, curr_face);
      }

      output_base_index += curr_face->mNumIndices;
    }

    input_base_index += curr_mesh->mNumVertices;
    base_vertex += curr_mesh->mNumVertices;
  }

  const create_buffers_args create_args{raw_ptr(_vertices),
                                        vbuff_bytes,
                                        _vertexcount,
                                        raw_ptr(_indices),
                                        index_buffer_bytes_size,
                                        &_vertex_format_info};

  create_buffers(&create_args);
  return true;
}

xray::rendering::simple_mesh::simple_mesh(const vertex_format fmt,
                                          const char*         mesh_file,
                                          const uint32_t      load_options)
  : _vertexformat{fmt}, _topology{primitive_topology::triangle} {
  constexpr uint32_t default_processing_opts =
    aiProcess_CalcTangentSpace |         // calculate tangents and
    aiProcess_JoinIdenticalVertices |    // join identical vertices/
    aiProcess_ValidateDataStructure |    // perform a full validation of the
    aiProcess_ImproveCacheLocality |     // improve the cache locality of
    aiProcess_RemoveRedundantMaterials | // remove redundant materials
    aiProcess_FindDegenerates |          // remove degenerated polygons from the
    aiProcess_FindInvalidData |          // detect invalid model data, such as
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
    post_processing_opts | (load_options & mesh_load_option::convert_left_handed
                              ? aiProcess_ConvertToLeftHanded
                              : 0);

  try {
    platformstl::memory_mapped_file mesh_mmfile{mesh_file};
    _valid = load_model_impl(static_cast<const char*>(mesh_mmfile.memory()),
                             mesh_mmfile.size(),
                             all_processing_opts,
                             load_options);
  } catch (const std::exception&) {
    XR_LOG_ERR("Failed to import model file {}", mesh_file);
  }
}

void xray::rendering::simple_mesh::draw() const noexcept {
  assert(valid());

  scoped_vertex_array_binding vao_binding{raw_handle(_vertexarray)};
  XR_UNUSED_ARG(vao_binding);

  const GLuint index_element_type[] = {gl::UNSIGNED_SHORT, gl::UNSIGNED_INT};
  gl::DrawElements(gl::TRIANGLES,
                   _indexcount,
                   index_element_type[_indexformat == index_format::u32],
                   nullptr);
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

template <>
struct format_cast_impl<xray::rendering::vertex_p,
                        xray::rendering::vertex_pntt> {
  static xray::rendering::vertex_p
  cast(const xray::rendering::vertex_pntt& vs_in) {
    return {vs_in.position};
  }
};

template <typename OutputFormatType, typename InputFormatType>
OutputFormatType format_cast(const InputFormatType& vs_in) {
  return format_cast_impl<OutputFormatType, InputFormatType>::cast(vs_in);
}

template <typename OutputFormatType>
void copy_geometry(void* dest, const geometry_data_t& geometry) {
  const auto input_span = gsl::span<const vertex_pntt>{
    raw_ptr(geometry.geometry), static_cast<ptrdiff_t>(geometry.vertex_count)};

  auto out = static_cast<OutputFormatType*>(dest);
  for (const auto& vs_in : input_span)
    *out++ = format_cast<OutputFormatType>(vs_in);
}

xray::rendering::simple_mesh::simple_mesh(const vertex_format      fmt,
                                          const geometry_data_t&   geometry,
                                          const primitive_topology topology)
  : _vertexcount{static_cast<uint32_t>(geometry.vertex_count)}
  , _indexcount{static_cast<uint32_t>(geometry.index_count)}
  , _vertexformat{fmt}
  , _indexformat{index_format::u32}
  , _topology{topology}
  , _vertex_format_info{get_vertex_format_description(_vertexformat)} {
  size_t vertex_bytes{};

  if (fmt != vertex_format::pntt) {
    vertex_bytes = geometry.vertex_count * _vertex_format_info.element_size;
    _vertices    = unique_pointer<void, malloc_deleter>{malloc(vertex_bytes)};

    switch (fmt) {
    case vertex_format::pn:
      copy_geometry<vertex_pn>(raw_ptr(_vertices), geometry);
      break;

    case vertex_format::pnt:
      copy_geometry<vertex_pnt>(raw_ptr(_vertices), geometry);
      break;

    case vertex_format::p:
      copy_geometry<vertex_p>(raw_ptr(_vertices), geometry);
      break;

    default:
      assert(false && "Unsupported vertex format !");
      break;
    }
  } else {
    vertex_bytes = geometry.vertex_count * sizeof(vertex_pntt);
  }

  size_t index_bytes{_indexcount * sizeof(geometry.indices[0])};
  {
    _indices = unique_pointer<void, malloc_deleter>{malloc(index_bytes)};
    memcpy(raw_ptr(_indices), raw_ptr(geometry.indices), index_bytes);
  }

  const create_buffers_args create_args{raw_ptr(_vertices),
                                        vertex_bytes,
                                        geometry.vertex_count,
                                        raw_ptr(_indices),
                                        index_bytes,
                                        &_vertex_format_info};

  create_buffers(&create_args);
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

void xray::rendering::simple_mesh::create_buffers(
  const create_buffers_args* args) {
  assert(args != nullptr);

  _vertexbuffer = [args]() {
    GLuint vbuff{};
    gl::CreateBuffers(1, &vbuff);
    gl::NamedBufferStorage(vbuff,
                           static_cast<GLsizeiptr>(args->vertexbuffer_bytes),
                           args->vertexbuffer_data,
                           0);
    return vbuff;
  }();

  _indexbuffer = [args]() {
    GLuint ibuff{};
    gl::CreateBuffers(1, &ibuff);
    gl::NamedBufferStorage(ibuff,
                           static_cast<GLsizeiptr>(args->indexbuffer_size),
                           args->indexbuffer_data,
                           0);
    return ibuff;
  }();

  _vertexarray =
    [ vb = raw_handle(_vertexbuffer), ib = raw_handle(_indexbuffer), args ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::BindVertexArray(vao);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, ib);
    gl::VertexArrayVertexBuffer(
      vao, 0, vb, 0, static_cast<GLsizei>(args->fmt_into->element_size));

    for (uint32_t idx = 0; idx < args->fmt_into->components; ++idx) {
      const auto& component_desc = args->fmt_into->description[idx];
      gl::EnableVertexArrayAttrib(vao, idx);
      gl::VertexArrayAttribFormat(
        vao,
        idx,
        static_cast<GLint>(component_desc.component_count),
        helpers::map_component_type(component_desc.component_type),
        gl::FALSE_,
        component_desc.component_offset);
      gl::VertexArrayAttribBinding(vao, idx, 0);
    }

    gl::BindVertexArray(0);
    gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, 0);
    return vao;
  }
  ();

  timer_highp tmr{};
  tmr.start();
  //          [args]() {

  //    //    XRAY_TIMED_SCOPE("mesh aabb computation");
  _boundingbox = bounding_box3_axis_aligned(
    static_cast<const vec3f*>(args->vertexbuffer_data),
    args->vertexcount,
    args->fmt_into->element_size);

  //    struct aabb_reduce_body {
  //      using reduce_range_type = tbb::blocked_range<size_t>;

  //      aabb_reduce_body(const void* points, const size_t stride) noexcept
  //          : _points{points}, _stride{stride} {}

  //      aabb_reduce_body(const aabb_reduce_body& other, tbb::split)
  //      noexcept
  //          : _points{other._points}
  //          , _stride{other._stride}
  //          , _boundingbox{other._boundingbox} {}

  //      void operator()(const reduce_range_type& range) noexcept {
  //        for (size_t idx = range.begin(), pts_cnt = range.end(); idx <
  //        pts_cnt;
  //             ++idx) {
  //          const auto pt = reinterpret_cast<const vec3f*>(
  //              static_cast<const uint8_t*>(_points) + idx * _stride);

  //          _boundingbox.min.x = math::min(_boundingbox.min.x, pt->x);
  //          _boundingbox.min.y = math::min(_boundingbox.min.y, pt->y);
  //          _boundingbox.min.z = math::min(_boundingbox.min.z, pt->z);

  //          _boundingbox.max.x = math::max(_boundingbox.max.x, pt->x);
  //          _boundingbox.max.y = math::max(_boundingbox.max.y, pt->y);
  //          _boundingbox.max.z = math::max(_boundingbox.max.z, pt->z);
  //        }
  //      }

  //      void join(const aabb_reduce_body& rhs) noexcept {
  //        _boundingbox = math::merge(_boundingbox, rhs._boundingbox);
  //      }

  //      const void* _points{nullptr};
  //      size_t      _stride{};
  //      aabb3f      _boundingbox{aabb3f::stdc::identity};
  //    };

  //    aabb_reduce_body body{args->vertexbuffer_data,
  //                          args->fmt_into->element_size};
  //    tbb::parallel_reduce(
  //        aabb_reduce_body::reduce_range_type{0, args->vertexcount},
  //        body);

  //    return body._boundingbox;
  //  }();

  tmr.end();
  XR_LOG_TRACE("Mesh aabb parallel reduce took {} msec", tmr.elapsed_millis());

  XR_LOG_TRACE("Bounding box : min [{}, {}, {}], max [{}, {}, {}], center [{}, "
               "{}, {}], width {}, height {}, depth {}",
               _boundingbox.min.x,
               _boundingbox.min.y,
               _boundingbox.min.z,
               _boundingbox.max.x,
               _boundingbox.max.y,
               _boundingbox.max.z,
               _boundingbox.center().x,
               _boundingbox.center().y,
               _boundingbox.center().z,
               _boundingbox.width(),
               _boundingbox.height(),
               _boundingbox.depth());
}

xray::rendering::mesh_graphics_rep::mesh_graphics_rep(
  const simple_mesh& mesh_geometry)
  : _geometry{&mesh_geometry} {
  _vertexbuffer = [ge = _geometry]() {
    GLuint vb{};
    gl::CreateBuffers(1, &vb);

    const auto buff_bytesize =
      static_cast<GLsizeiptr>(ge->byte_size_vertices());
    gl::NamedBufferStorage(vb, buff_bytesize, ge->vertices(), 0);
    return vb;
  }
  ();

  if (_geometry->indexed()) {
    _indexbuffer = [ge = _geometry]() {
      GLuint ib{};
      gl::CreateBuffers(1, &ib);

      const auto buff_bytesize =
        static_cast<GLsizeiptr>(ge->byte_size_indices());
      gl::NamedBufferStorage(ib, buff_bytesize, ge->indices(), 0);
      return ib;
    }
    ();
  }

  _vertexarray = [
    ge = _geometry,
    vb = raw_handle(_vertexbuffer),
    ib = raw_handle(_indexbuffer)
  ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);

    const auto vertex_format_desc = ge->vertex_fmt_description();

    gl::VertexArrayVertexBuffer(
      vao, 0, vb, 0, vertex_format_desc->element_size);
    if (ib) {
      gl::VertexArrayElementBuffer(vao, ib);
    }

    for (uint32_t idx = 0; idx < vertex_format_desc->components; ++idx) {
      const auto elem_desc = vertex_format_desc->description + idx;
      gl::VertexArrayAttribFormat(
        vao,
        idx,
        elem_desc->component_count,
        translate_component_type(elem_desc->component_type),
        gl::FALSE_,
        elem_desc->component_offset);
      gl::VertexArrayAttribBinding(vao, idx, 0);
      gl::EnableVertexArrayAttrib(vao, idx);
    }

    return vao;
  }
  ();
}

static constexpr GLenum PRIMITIVE_TOPOLOGY_TRANSLATION_TABLE[] = {
  gl::INVALID_ENUM,
  gl::POINTS,
  gl::LINES,
  gl::LINE_STRIP,
  gl::TRIANGLES,
  gl::TRIANGLE_STRIP};

static inline constexpr auto
map_topology(const xray::rendering::primitive_topology topo) {
  using namespace xray::base;
  return PRIMITIVE_TOPOLOGY_TRANSLATION_TABLE[enum_helper::to_underlying_type(
    topo)];
}

static constexpr GLenum INDEX_TYPE_TRANSLATION_TABLE[] = {gl::UNSIGNED_SHORT,
                                                          gl::UNSIGNED_INT};

void xray::rendering::mesh_graphics_rep::draw() noexcept {
  assert(valid());

  using namespace xray::base;

  gl::BindVertexArray(vertex_array());

  const auto topology = map_topology(geometry()->topology());
  const auto ge       = geometry();

  if (ge->indexed()) {
    const auto index_type =
      INDEX_TYPE_TRANSLATION_TABLE[enum_helper::to_underlying_type(
        ge->index_fmt())];
    gl::DrawElements(topology, ge->index_count(), index_type, nullptr);
  } else {
    gl::DrawArrays(topology, 0, ge->vertex_count());
  }
}

struct geometry_blob {
  std::vector<xray::rendering::vertex_pnt> vertices;
  std::vector<uint32_t>                    indices;
};

static bool
load_geometry_impl(const char*                                model_data_ptr,
                   const size_t                               data_size,
                   const uint32_t                             load_flags,
                   const xray::rendering::mesh_import_options import_opts,
                   geometry_blob*                             mesh_data) {
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
    aiSetImportPropertyInteger(
      raw_ptr(import_props), AI_CONFIG_IMPORT_TER_MAKE_UVS, 1);

    if (import_opts & mesh_import_options::remove_points_lines) {
      aiSetImportPropertyInteger(raw_ptr(import_props),
                                 AI_CONFIG_PP_SBP_REMOVE,
                                 aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    }
  }

  auto imported_scene = aiImportFileFromMemoryWithProperties(
    model_data_ptr, data_size, load_flags, nullptr, raw_ptr(import_props));

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

  // mesh_data->setup(num_vertices, num_indices);
  mesh_data->vertices.resize(num_vertices);
  mesh_data->indices.resize(size_t{num_indices});

  uint32_t vertex_count   = 0;
  uint32_t index_count    = 0;
  uint32_t indices_offset = 0;

  for (uint32_t mesh_index = 0; mesh_index < imported_scene->mNumMeshes;
       ++mesh_index) {
    const aiMesh* curr_mesh = imported_scene->mMeshes[mesh_index];

    if (!curr_mesh->mVertices)
      continue;

    //
    // Collect vertices
    for (uint32_t vertex_idx = 0; vertex_idx < curr_mesh->mNumVertices;
         ++vertex_idx) {
      const auto  output_pos = vertex_idx + vertex_count;
      const auto& vtx        = curr_mesh->mVertices[vertex_idx];

      mesh_data->vertices[output_pos].position = {vtx.x, vtx.y, vtx.z};
    }

    //
    // collect normals

    {
      struct msvc_fuck_off {
        static auto get_normal_from_mesh(const aiVector3D* input,
                                         const uint32_t    pos) noexcept {
          return vec3f{input[pos].x, input[pos].y, input[pos].z};
        }

        static auto get_null_normal(const aiVector3D* /*input*/,
                                    const uint32_t /*pos*/) noexcept {
          return vec3f::stdc::zero;
        }
      };

      auto fn_get_normal = curr_mesh->HasNormals()
                             ? &msvc_fuck_off::get_normal_from_mesh
                             : &msvc_fuck_off::get_null_normal;

      for (uint32_t idx = 0; idx < curr_mesh->mNumVertices; ++idx) {
        const auto output_pos = idx + vertex_count;
        mesh_data->vertices[output_pos].normal =
          fn_get_normal(curr_mesh->mNormals, idx);
      }
    }

    //
    // collect texture coordinates.
    {
      struct msvc_fuckery {
        static auto get_texcoord_from_mesh(const aiVector3D* const* texcoords,
                                           const uint32_t pos) noexcept {
          return vec2f{texcoords[0][pos].x, texcoords[0][pos].y};
        }

        static auto
        get_default_texcoords(const aiVector3D* const* /*texcoords*/,
                              const uint32_t /*pos*/) noexcept {
          return vec2f{0.5f, 0.5f};
        }
      };

      auto fn_get_texcoords = curr_mesh->HasTextureCoords(0)
                                ? &msvc_fuckery::get_texcoord_from_mesh
                                : &msvc_fuckery::get_default_texcoords;

      for (uint32_t idx = 0; idx < curr_mesh->mNumVertices; ++idx) {
        const auto output_pos = idx + vertex_count;
        mesh_data->vertices[output_pos].texcoord =
          fn_get_texcoords(curr_mesh->mTextureCoords, idx);
      }
    }

    //
    //  Collect indices defining primitives.
    for (uint32_t face_index = 0; face_index < curr_mesh->mNumFaces;
         ++face_index) {
      const aiFace* curr_face = &curr_mesh->mFaces[face_index];

      for (uint32_t i = 0; i < curr_face->mNumIndices; ++i) {
        mesh_data->indices[index_count++] =
          curr_face->mIndices[i] + indices_offset;
      }
    }

    indices_offset += curr_mesh->mNumVertices;
    vertex_count += curr_mesh->mNumVertices;
  }

  return true;
}

static bool
load_geometry(geometry_blob*                             mesh_data,
              const char*                                file_path,
              const xray::rendering::mesh_import_options import_opts) {
  assert(mesh_data != nullptr);
  assert(file_path != nullptr);

  constexpr uint32_t default_processing_opts =
    aiProcess_CalcTangentSpace |         // calculate tangents and
    aiProcess_JoinIdenticalVertices |    // join identical vertices/
    aiProcess_ValidateDataStructure |    // perform a full validation of the
    aiProcess_ImproveCacheLocality |     // improve the cache locality of
    aiProcess_RemoveRedundantMaterials | // remove redundant materials
    aiProcess_FindDegenerates |          // remove degenerated polygons from the
    aiProcess_FindInvalidData |          // detect invalid model data, such as
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
    (import_opts & mesh_import_options::convert_left_handed
       ? aiProcess_ConvertToLeftHanded
       : 0);

  try {
    platformstl::memory_mapped_file mesh_mmfile{file_path};

    return load_geometry_impl(static_cast<const char*>(mesh_mmfile.memory()),
                              mesh_mmfile.size(),
                              all_processing_opts,
                              import_opts,
                              mesh_data);
  } catch (const std::exception&) {
    XR_LOG_ERR("Failed to import model file {}", file_path);
  }

  return false;
}

xray::rendering::basic_mesh::basic_mesh(const char* path) {
  using namespace xray::rendering;

  geometry_blob geometry;
  load_geometry(&geometry,
                path,
                mesh_import_options::convert_left_handed |
                  mesh_import_options::remove_points_lines);

  if (geometry.vertices.empty() || geometry.indices.empty()) {
    return;
  }

  _vertices = std::move(geometry.vertices);
  _indices  = std::move(geometry.indices);

  gl::CreateBuffers(1, raw_handle_ptr(_vertexbuffer));
  gl::NamedBufferStorage(raw_handle(_vertexbuffer),
                         xray::base::container_bytes_size(_vertices),
                         _vertices.data(),
                         0);

  gl::CreateBuffers(1, raw_handle_ptr(_indexbuffer));
  gl::NamedBufferStorage(raw_handle(_indexbuffer),
                         xray::base::container_bytes_size(_indices),
                         _indices.data(),
                         0);

  gl::CreateVertexArrays(1, raw_handle_ptr(_vertexarray));
  const auto vao = raw_handle(_vertexarray);

  gl::VertexArrayVertexBuffer(
    vao, 0, raw_handle(_vertexbuffer), 0, sizeof(vertex_pnt));
  gl::VertexArrayElementBuffer(vao, raw_handle(_indexbuffer));

  gl::EnableVertexArrayAttrib(vao, 0);
  gl::VertexArrayAttribFormat(
    vao, 0, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, position));
  gl::VertexArrayAttribBinding(vao, 0, 0);

  gl::EnableVertexArrayAttrib(vao, 1);
  gl::VertexArrayAttribFormat(
    vao, 1, 3, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, normal));
  gl::VertexArrayAttribBinding(vao, 1, 0);

  gl::EnableVertexArrayAttrib(vao, 2);
  gl::VertexArrayAttribFormat(
    vao, 2, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex_pnt, texcoord));
  gl::VertexArrayAttribBinding(vao, 2, 0);
}
