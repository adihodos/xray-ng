#include "xray/rendering/geometry.importer.gltf.hpp"

#include <vector>
#include <unordered_map>
#include <ranges>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <itlib/small_vector.hpp>
#include <mio/mmap.hpp>

#include "xray/base/logger.hpp"
#include "xray/math/scalar4.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/quaternion.hpp"
#include "xray/math/quaternion_math.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/vertex_format/vertex.format.pbr.hpp"

using namespace std;

namespace xray::rendering {

LoadedGeometry::LoadedGeometry()
    : gltf{ std::make_unique<tinygltf::Model>() }
{
}

LoadedGeometry::~LoadedGeometry() {}

tl::expected<LoadedGeometry, GeometryImportError>
LoadedGeometry::from_file(const filesystem::path& path)
{
    error_code e{};
    mio::mmap_source mmaped_file = mio::make_mmap_source(path.string(), e);
    if (e) {
        return tl::make_unexpected<GeometryImportError>(e);
    }
    return from_memory(
        span<const uint8_t>{ reinterpret_cast<const uint8_t*>(mmaped_file.data()), mmaped_file.mapped_length() });
}

tl::expected<LoadedGeometry, GeometryImportError>
LoadedGeometry::from_memory(const std::span<const uint8_t> bytes)
{
    LoadedGeometry geometry{};
    tinygltf::TinyGLTF loader;

    string error;
    string warning;
    if (!loader.LoadBinaryFromMemory(geometry.gltf.get(), &error, &warning, bytes.data(), bytes.size_bytes())) {
        return tl::make_unexpected(GeometryImportError{ GeometryImportParseError{ error, warning } });
    }

    return tl::expected<LoadedGeometry, GeometryImportError>{ std::move(geometry) };
}

math::vec2ui32
LoadedGeometry::compute_node_vertex_index_count(const tinygltf::Node& node) const
{
    math::vec2ui32 result{ 0, 0 };
    for (const int child_idx : node.children) {
        result += compute_node_vertex_index_count(gltf->nodes[child_idx]);
    }

    if (node.mesh != -1) {
        const tinygltf::Mesh* mesh = &gltf->meshes[node.mesh];
        for (const tinygltf::Primitive& prim : mesh->primitives) {
            assert(prim.mode == TINYGLTF_MODE_TRIANGLES);

            auto attr_itr = prim.attributes.find("POSITION");
            if (attr_itr == cend(prim.attributes)) {
                XR_LOG_ERR("Missing POSITION attribute");
                continue;
            }

            if (prim.indices == -1) {
                XR_LOG_ERR("Unindexed geometry is not supported.");
                continue;
            }

            const tinygltf::Accessor& pos_accessor = gltf->accessors[attr_itr->second];
            assert(pos_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            assert(pos_accessor.type == TINYGLTF_TYPE_VEC3);

            result.x += pos_accessor.count;

            const tinygltf::Accessor& index_accesor = gltf->accessors[prim.indices];
            assert(index_accesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
                   index_accesor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
            assert(index_accesor.type == TINYGLTF_TYPE_SCALAR);

            result.y += index_accesor.count;
        }
    }

    return result;
}

math::vec2ui32
LoadedGeometry::compute_vertex_index_count() const
{
    math::vec2ui32 result{ 0, 0 };
    for (const tinygltf::Scene& s : gltf->scenes) {
        for (const int node_idx : s.nodes) {
            result += compute_node_vertex_index_count(gltf->nodes[node_idx]);
        }
    }

    return result;
}

ExtractedMaterialsWithImageSourcesBundle
LoadedGeometry::extract_images_info(const uint32_t null_texture) const noexcept
{
    ExtractedMaterialsWithImageSourcesBundle result{};

    ranges::transform(
        gltf->images, back_inserter(result.image_sources), [gltf = this->gltf.get()](const tinygltf::Image& img) {
            if (!img.image.empty()) {
                return ExtractedImageData{
                    img.name, (uint32_t)img.width, (uint32_t)img.height, (uint8_t)img.bits, std::span{ img.image }
                };
            }

            const size_t bytes_per_channel = [i = &img]() {
                assert(i->pixel_type != -1);

                switch (i->pixel_type) {
                    default:
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                        return 1;

                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        return 2;

                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        return 4;
                }
            }();

            //
            // in a buffer ?!
            assert(img.bufferView != -1);
            const tinygltf::BufferView& buffer_view{ gltf->bufferViews[img.bufferView] };
            const tinygltf::Buffer& buffer{ gltf->buffers[buffer_view.buffer] };
            return ExtractedImageData{ img.name,
                                       (uint32_t)img.width,
                                       (uint32_t)img.height,
                                       (uint8_t)img.bits,
                                       std::span{ buffer.data.data() + buffer_view.byteOffset,
                                                  (size_t)(img.width * img.height * bytes_per_channel) } };
        });

    ranges::transform(
        gltf->materials,
        back_inserter(result.materials),
        [g = this->gltf.get(), null_texture](const tinygltf::Material& mtl) {
            uint32_t null_texture{};
            return ExtractedMaterialDefinition{
                mtl.name,
                mtl.pbrMetallicRoughness.baseColorTexture.index == -1
                    ? null_texture
                    : (uint32_t)g->textures[mtl.pbrMetallicRoughness.baseColorTexture.index].source,
                mtl.pbrMetallicRoughness.metallicRoughnessTexture.index == -1
                    ? null_texture
                    : (uint32_t)g->textures[mtl.pbrMetallicRoughness.metallicRoughnessTexture.index].source,
                mtl.normalTexture.index == -1 ? null_texture : (uint32_t)g->textures[mtl.normalTexture.index].source,
                math::vec4f{ mtl.pbrMetallicRoughness.baseColorFactor.data(), 4 },
                (float)mtl.pbrMetallicRoughness.metallicFactor,
                (float)mtl.pbrMetallicRoughness.roughnessFactor
            };
        });

    return result;
}

math::vec2ui32
LoadedGeometry::extract_data(void* vertex_buffer,
                             void* index_buffer,
                             const math::vec2ui32 offsets,
                             const uint32_t mtl_offset)
{
    math::vec2ui32 acc_offsets{ offsets };
    for (const tinygltf::Scene& s : gltf->scenes) {
        for (const int node_idx : s.nodes) {
            acc_offsets += extract_single_node_data(
                vertex_buffer, index_buffer, acc_offsets, mtl_offset, gltf->nodes[node_idx], tl::nullopt);
        }
    }

    return acc_offsets;
}

math::vec2ui32
LoadedGeometry::extract_single_node_data(void* vertex_buffer,
                                         void* index_buffer,
                                         const math::vec2ui32 initial_buffer_offsets,
                                         const uint32_t mtl_offset,
                                         const tinygltf::Node& node,
                                         const tl::optional<uint32_t> parent)
{
    const math::mat4f node_transform = [n = &node]() {
        if (!n->matrix.empty()) {
            return math::mat4f{ n->matrix.data(), n->matrix.size() };
        }

        const math::mat4f scale = [n]() {
            return n->scale.empty() ? math::mat4f::stdc::identity
                                    : math::R4::scaling(static_cast<float>(n->scale[0]),
                                                        static_cast<float>(n->scale[1]),
                                                        static_cast<float>(n->scale[2]));
        }();

        const math::mat4f rotation = [n]() {
            return n->rotation.empty() ? math::mat4f::stdc::identity
                                       : math::rotation_matrix(math::quatf{ static_cast<float>(n->rotation[3]),
                                                                            static_cast<float>(n->rotation[0]),
                                                                            static_cast<float>(n->rotation[1]),
                                                                            static_cast<float>(n->rotation[2]) });
        }();

        const math::mat4f translate = [n]() {
            return n->translation.empty() ? math::mat4f::stdc::identity
                                          : math::R4::translate(static_cast<float>(n->translation[0]),
                                                                static_cast<float>(n->translation[1]),
                                                                static_cast<float>(n->translation[2]));
        }();

        return translate * rotation * scale;
    }();

    const uint32_t node_id = static_cast<uint32_t>(nodes.size());
    this->nodes.emplace_back(parent, node.name, math::mat4f{ node_transform }, math::aabb3f::stdc::identity, 0, 0, 0);

    math::vec2ui32 accum_buffer_offsets{ initial_buffer_offsets };
    for (const int child_idx : node.children) {
        accum_buffer_offsets += extract_single_node_data(vertex_buffer,
                                                         index_buffer,
                                                         accum_buffer_offsets,
                                                         mtl_offset,
                                                         gltf->nodes[child_idx],
                                                         tl::optional<uint32_t>{ node_id });
    }

    this->nodes[node_id].vertex_offset = accum_buffer_offsets.x;
    this->nodes[node_id].index_offset = accum_buffer_offsets.y;

    if (node.mesh != -1) {
        tl::optional<uint32_t> ancestor{ parent };
        math::mat4f transform{ node_transform };

        while (ancestor) {
            const GeometryNode* a = &nodes[*ancestor];
            transform = a->transform * transform;
            ancestor = a->parent;
        }

        nodes[node_id].transform = transform;
        const math::mat4f normals_matrix = math::mat4f::stdc::identity;
        // math::transpose(math::invert(transform));

        const tinygltf::Mesh* mesh = &gltf->meshes[node.mesh];
        for (const tinygltf::Primitive& prim : mesh->primitives) {
            assert(prim.mode == TINYGLTF_MODE_TRIANGLES);
            assert(prim.material != -1);

            if (prim.indices == -1) {
                XR_LOG_ERR("Unindexed geometry is not supported.");
                continue;
            }

            VertexPBR* dst_data{ reinterpret_cast<VertexPBR*>(vertex_buffer) + accum_buffer_offsets.x };

            uint32_t primitive_vertices{};
            if (auto attr_itr = prim.attributes.find("POSITION"); attr_itr != cend(prim.attributes)) {
                const tinygltf::Accessor& accessor = gltf->accessors[attr_itr->second];
                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                assert(accessor.type == TINYGLTF_TYPE_VEC3);

                assert(accessor.bufferView != -1);
                const tinygltf::BufferView& buffer_view = gltf->bufferViews[accessor.bufferView];

                assert(buffer_view.target == TINYGLTF_TARGET_ARRAY_BUFFER);
                assert(buffer_view.buffer != -1);
                const tinygltf::Buffer& buffer = gltf->buffers[buffer_view.buffer];

                span<const math::vec3f> src_data{ reinterpret_cast<const math::vec3f*>(buffer.data.data() +
                                                                                       buffer_view.byteOffset +
                                                                                       accessor.byteOffset),
                                                  static_cast<size_t>(accessor.count) };

                for (size_t vtx = 0; vtx < static_cast<size_t>(accessor.count); ++vtx) {
                    dst_data[vtx].pos = math::mul_point(transform, src_data[vtx]);
                    dst_data[vtx].color = math::vec4f{ 0.0f, 0.0f, 0.0f, 1.0f };
                    dst_data[vtx].pbr_buf_id = (uint32_t)prim.material + mtl_offset;
                }

                primitive_vertices = accessor.count;
            } else {
                XR_LOG_ERR("Missing POSITION attribute");
                continue;
            }

            if (auto attr_normals = prim.attributes.find("NORMAL"); attr_normals != cend(prim.attributes)) {
                const tinygltf::Accessor& accessor = gltf->accessors[attr_normals->second];
                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                assert(accessor.type == TINYGLTF_TYPE_VEC3);

                assert(accessor.bufferView != -1);
                const tinygltf::BufferView& buffer_view = gltf->bufferViews[accessor.bufferView];
                assert(buffer_view.target == TINYGLTF_TARGET_ARRAY_BUFFER);
                assert(buffer_view.buffer != -1);

                const tinygltf::Buffer& buffer = gltf->buffers[buffer_view.buffer];

                span<const math::vec3f> src_data{ reinterpret_cast<const math::vec3f*>(buffer.data.data() +
                                                                                       buffer_view.byteOffset +
                                                                                       accessor.byteOffset),
                                                  static_cast<size_t>(accessor.count) };

                for (size_t vtx = 0; vtx < static_cast<size_t>(accessor.count); ++vtx) {
                    dst_data[vtx].normal = math::normalize(math::mul_vec(normals_matrix, src_data[vtx]));
                }
            }

            if (auto attr_texcoords = prim.attributes.find("TEXCOORD_0"); attr_texcoords != cend(prim.attributes)) {
                const tinygltf::Accessor& accessor = gltf->accessors[attr_texcoords->second];
                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                assert(accessor.type == TINYGLTF_TYPE_VEC2);

                assert(accessor.bufferView != -1);
                const tinygltf::BufferView& buffer_view = gltf->bufferViews[accessor.bufferView];
                assert(buffer_view.target == TINYGLTF_TARGET_ARRAY_BUFFER);
                assert(buffer_view.buffer != -1);

                const tinygltf::Buffer& buffer = gltf->buffers[buffer_view.buffer];

                span<const math::vec2f> src_data{ reinterpret_cast<const math::vec2f*>(buffer.data.data() +
                                                                                       buffer_view.byteOffset +
                                                                                       accessor.byteOffset),
                                                  static_cast<size_t>(accessor.count) };

                for (size_t vtx = 0; vtx < static_cast<size_t>(accessor.count); ++vtx) {
                    dst_data[vtx].uv = src_data[vtx];
                }
            }

            if (auto attr_tangent = prim.attributes.find("TANGENT"); attr_tangent != cend(prim.attributes)) {
                const tinygltf::Accessor& accessor = gltf->accessors[attr_tangent->second];
                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                assert(accessor.type == TINYGLTF_TYPE_VEC4);

                assert(accessor.bufferView != -1);
                const tinygltf::BufferView& buffer_view = gltf->bufferViews[accessor.bufferView];
                assert(buffer_view.target == TINYGLTF_TARGET_ARRAY_BUFFER);
                assert(buffer_view.buffer != -1);

                const tinygltf::Buffer& buffer = gltf->buffers[buffer_view.buffer];

                span<const math::vec4f> src_data{ reinterpret_cast<const math::vec4f*>(buffer.data.data() +
                                                                                       buffer_view.byteOffset +
                                                                                       accessor.byteOffset),
                                                  static_cast<size_t>(accessor.count) };

                for (size_t vtx = 0; vtx < static_cast<size_t>(accessor.count); ++vtx) {
                    dst_data[vtx].tangent = src_data[vtx];
                }
            }

            if (auto attr_color = prim.attributes.find("COLOR_0"); attr_color != cend(prim.attributes)) {
                const tinygltf::Accessor& accessor = gltf->accessors[attr_color->second];
                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
                assert(accessor.type == TINYGLTF_TYPE_VEC4);

                assert(accessor.bufferView != -1);
                const tinygltf::BufferView& buffer_view = gltf->bufferViews[accessor.bufferView];
                assert(buffer_view.target == TINYGLTF_TARGET_ARRAY_BUFFER);
                assert(buffer_view.buffer != -1);

                const tinygltf::Buffer& buffer = gltf->buffers[buffer_view.buffer];

                span<const math::vec4f> src_data{ reinterpret_cast<const math::vec4f*>(buffer.data.data() +
                                                                                       buffer_view.byteOffset +
                                                                                       accessor.byteOffset),
                                                  static_cast<size_t>(accessor.count) };

                for (size_t vtx = 0; vtx < static_cast<size_t>(accessor.count); ++vtx) {
                    dst_data[vtx].tangent = src_data[vtx];
                }
            }

            //
            // indices
            {
                const tinygltf::Accessor& accessor = gltf->accessors[prim.indices];
                assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
                       accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
                assert(accessor.type == TINYGLTF_TYPE_SCALAR);

                assert(accessor.bufferView != -1);
                const tinygltf::BufferView& buffer_view = gltf->bufferViews[accessor.bufferView];
                assert(buffer_view.buffer != -1);
                assert(buffer_view.byteStride == 0);
                const tinygltf::Buffer& buffer = gltf->buffers[buffer_view.buffer];

                uint32_t* dst_data{ reinterpret_cast<uint32_t*>(index_buffer) + accum_buffer_offsets.y };

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    span<const uint16_t> src_data{ reinterpret_cast<const uint16_t*>(buffer.data.data() +
                                                                                     buffer_view.byteOffset +
                                                                                     accessor.byteOffset),
                                                   static_cast<size_t>(accessor.count) };

                    for (size_t idx = 0; idx < static_cast<size_t>(accessor.count); ++idx) {
                        dst_data[idx] = src_data[idx];
                    }
                } else {
                    span<const uint32_t> src_data{ reinterpret_cast<const uint32_t*>(buffer.data.data() +
                                                                                     buffer_view.byteOffset +
                                                                                     accessor.byteOffset),
                                                   static_cast<size_t>(accessor.count) };

                    memcpy(dst_data, src_data.data(), src_data.size_bytes());
                }

                //
                // correct indices to account for the vertex offset
                for (size_t idx = 0; idx < static_cast<size_t>(accessor.count); ++idx) {
                    dst_data[idx] += accum_buffer_offsets.x;
                }

                accum_buffer_offsets.x += primitive_vertices;
                accum_buffer_offsets.y += accessor.count;
            }
        }
    }

    const math::vec2ui32 node_vertex_index_counts = accum_buffer_offsets - initial_buffer_offsets;
    this->nodes[node_id].index_count = node_vertex_index_counts.y;

    XR_LOG_INFO("Node {}, offsets {} {}, vertices {}, indices {}",
                nodes[node_id].name,
                nodes[node_id].vertex_offset,
                nodes[node_id].index_offset,
                node_vertex_index_counts.x,
                node_vertex_index_counts.y);

    return node_vertex_index_counts;
}
}
