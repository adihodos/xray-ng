#include "xray/rendering/geometry.importer.gltf.hpp"

#include <vector>
#include <unordered_map>
#include <string_view>

#include <Lz/Lz.hpp>
#include <stb/stb_image.h>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#include <itlib/small_vector.hpp>
#include <mio/mmap.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include "xray/base/logger.hpp"
#include "xray/base/variant.helpers.hpp"
#include "xray/base/scoped_guard.hpp"
#include "xray/base/basic_timer.hpp"
#include "xray/math/objects/aabb3_math.hpp"
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
LoadedGeometry::LoadedGeometry(fastgltf::Asset&& asset)
    : gltf_asset{ std::make_unique<fastgltf::Asset>(std::move(asset)) }
{
}

LoadedGeometry::LoadedGeometry(LoadedGeometry&& rhs) noexcept
    : nodes{ std::move(rhs.nodes) }
    , bounding_box{ rhs.bounding_box }
    , bounding_sphere{ rhs.bounding_sphere }
    , gltf{ std::move(rhs.gltf) }
    , gltf_asset{ std::move(rhs.gltf_asset) }

{
}

LoadedGeometry::~LoadedGeometry() {}

tl::expected<LoadedGeometry, GeometryImportError>
LoadedGeometry::from_file(const filesystem::path& path)
{
    {
        base::timer_highp op_timer{};
        op_timer.start();
        XRAY_SCOPE_EXIT noexcept
        {
            op_timer.end();
            XR_LOG_INFO("[[FASTGLTF]] parsed {} in {} ms", path.generic_string(), op_timer.elapsed_millis());
        };
        static constexpr auto supportedExtensions = fastgltf::Extensions::KHR_mesh_quantization |
                                                    fastgltf::Extensions::KHR_texture_transform |
                                                    fastgltf::Extensions::KHR_materials_variants;

        fastgltf::Parser parser(supportedExtensions);

        constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                     fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages;

        auto gltfFile = fastgltf::MappedGltfFile::FromPath(path);
        if (gltfFile.error() != fastgltf::Error::None) {
            return tl::make_unexpected(
                GeometryImportParseError{ .err_data = fastgltf::getErrorMessage(gltfFile.error()).data() });
        }
        auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);
        if (asset.error() != fastgltf::Error::None) {
            return tl::make_unexpected(
                GeometryImportParseError{ .err_data = fastgltf::getErrorMessage(asset.error()).data() });
        }
        return LoadedGeometry{ std::move(asset.get()) };
    }

    // error_code e{};
    // mio::mmap_source mmaped_file = mio::make_mmap_source(path.string(), e);
    // if (e) {
    //     return tl::make_unexpected<GeometryImportError>(e);
    // }
    // return from_memory(
    //     span<const uint8_t>{ reinterpret_cast<const uint8_t*>(mmaped_file.data()), mmaped_file.mapped_length() });
}

tl::expected<LoadedGeometry, GeometryImportError>
LoadedGeometry::from_memory(const std::span<const uint8_t> bytes)
{
    base::timer_highp op_timer{};
    op_timer.start();
    XRAY_SCOPE_EXIT noexcept
    {
        op_timer.end();
        XR_LOG_INFO("GLTF load time {}", op_timer.elapsed_millis());
    };

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
LoadedGeometry::compute_node_vertex_index_count(const fastgltf::Node& node) const
{
    math::vec2ui32 result{ 0, 0 };
    for (const size_t child_idx : node.children) {
        result += compute_node_vertex_index_count(gltf_asset->nodes[child_idx]);
    }

    auto node_count = node.meshIndex.transform([this](const size_t mesh_index) {
        const fastgltf::Mesh* mesh = &gltf_asset->meshes[mesh_index];
        math::vec2ui32 node_counts{ math::vec2ui32::stdc::zero };

        for (const fastgltf::Primitive& prim : mesh->primitives) {
            assert(prim.type == fastgltf::PrimitiveType::Triangles);

            auto attr_itr = ranges::find_if(
                prim.attributes, [](const fastgltf::Attribute& attrib) { return attrib.name == "POSITION"; });
            if (attr_itr == cend(prim.attributes)) {
                XR_LOG_ERR("Missing POSITION attribute");
                continue;
            }

            if (!prim.indicesAccessor) {
                XR_LOG_ERR("Unindexed geometry is not supported.");
                continue;
            }

            const fastgltf::Accessor& pos_accessor = gltf_asset->accessors[attr_itr->accessorIndex];
            assert(pos_accessor.componentType == fastgltf::ComponentType::Float);
            assert(pos_accessor.type == fastgltf::AccessorType::Vec3);

            node_counts.x += pos_accessor.count;

            const fastgltf::Accessor& index_accesor = gltf_asset->accessors[*prim.indicesAccessor];
            assert(index_accesor.componentType == fastgltf::ComponentType::UnsignedShort ||
                   index_accesor.componentType == fastgltf::ComponentType::UnsignedInt);
            assert(index_accesor.type == fastgltf::AccessorType::Scalar);

            node_counts.y += index_accesor.count;
        }

        return node_counts;
    });

    if (node_count)
        result += *node_count;

    return result;
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

size_t
LoadedGeometry::compute_image_bytes_size() const
{
    return lz::chain(gltf_asset->images)
        .map([g = gltf_asset.get()](const fastgltf::Image& image) {
            return std::visit(
                base::VariantVisitor{
                    [](auto arg) { return size_t{}; },
                    [](const fastgltf::sources::URI& file_path) {
                        int32_t width{};
                        int32_t height{};
                        int32_t components{};
                        stbi_info(file_path.uri.c_str(), &width, &height, &components);
                        return static_cast<size_t>(width * height * 4);
                    },
                    [](const fastgltf::sources::Array& vec) {
                        int32_t width{};
                        int32_t height{};
                        int32_t components{};
                        stbi_info_from_memory(reinterpret_cast<const stbi_uc*>(vec.bytes.data()),
                                              static_cast<int32_t>(vec.bytes.size_bytes()),
                                              &width,
                                              &height,
                                              &components);
                        return static_cast<size_t>(width * height * 4);
                    },
                    [g](const fastgltf::sources::BufferView& view) {
                        auto& buffer_view = g->bufferViews[view.bufferViewIndex];
                        auto& buffer = g->buffers[buffer_view.bufferIndex];

                        return std::visit(
                            fastgltf::visitor{
                                // We only care about VectorWithMime here, because we specify LoadExternalBuffers,
                                // meaning
                                // all buffers are already loaded into a vector.
                                [](auto& arg) { return size_t{}; },
                                [&](fastgltf::sources::Array& vec) {
                                    int32_t width{};
                                    int32_t height{};
                                    int32_t components{};
                                    stbi_info_from_memory(reinterpret_cast<const stbi_uc*>(vec.bytes.data()),
                                                          static_cast<int32_t>(vec.bytes.size_bytes()),
                                                          &width,
                                                          &height,
                                                          &components);
                                    return static_cast<size_t>(width * height * 4);
                                } },
                            buffer.data);
                    } },
                image.data);
        })
        .sum();
}

math::vec2ui32
LoadedGeometry::compute_vertex_index_count() const
{
    math::vec2ui32 result{ 0, 0 };
    for (const fastgltf::Scene& s : gltf_asset->scenes) {
        for (const int node_idx : s.nodeIndices) {
            result += compute_node_vertex_index_count(gltf_asset->nodes[node_idx]);
        }
    }

    // for (const tinygltf::Scene& s : gltf->scenes) {
    //     for (const int node_idx : s.nodes) {
    //         result += compute_node_vertex_index_count(gltf->nodes[node_idx]);
    //     }
    // }

    return result;
}

auto
decode_image_from_memory_fn(const stbi_uc* pixels,
                            const size_t length,
                            std::string tag,
                            ExtractedMaterialsWithImageSourcesBundle* b,
                            std::byte* dst_buf) -> tl::expected<std::size_t, GeometryImportError>
{
    int32_t width{};
    int32_t height{};
    int32_t components{};

    stbi_uc* decoded_image = stbi_load_from_memory(pixels, static_cast<int>(length), &width, &height, &components, 4);

    if (!decoded_image) {
        return tl::make_unexpected(std::string{ stbi_failure_reason() });
    }

    XRAY_SCOPE_EXIT noexcept
    {
        stbi_image_free(reinterpret_cast<void*>(decoded_image));
    };

    const size_t bytes_size = static_cast<size_t>(width * height * components);
    memcpy(dst_buf, decoded_image, bytes_size);

    b->image_sources.push_back(ExtractedImageData{
        .tag = std::move(tag),
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .bits = static_cast<uint8_t>(components * 8),
        .pixels = {},
    });

    return bytes_size;
};

tl::expected<ExtractedMaterialsWithImageSourcesBundle, GeometryImportError>
LoadedGeometry::extract_materials(void* dst_buffer) const noexcept
{
    std::byte* buff_ptr{ static_cast<std::byte*>(dst_buffer) };
    ExtractedMaterialsWithImageSourcesBundle bundle;

    for (const fastgltf::Image& image : gltf_asset->images) {
        const auto extract_result = std::visit(
            base::VariantVisitor{
                [](auto arg) -> tl::expected<std::size_t, GeometryImportError> { return std::size_t{}; },
                [b = &bundle,
                 buff_ptr](const fastgltf::sources::URI& file_path) -> tl::expected<std::size_t, GeometryImportError> {
                    error_code e{};
                    mio::mmap_source mmaped_file = mio::make_mmap_source(file_path.uri.string(), e);
                    if (e) {
                        return tl::make_unexpected(e);
                    }

                    return decode_image_from_memory_fn(reinterpret_cast<const stbi_uc*>(mmaped_file.data()) +
                                                           file_path.fileByteOffset,
                                                       mmaped_file.mapped_length() - file_path.fileByteOffset,
                                                       file_path.uri.string().data(),
                                                       b,
                                                       buff_ptr);
                },
                [buff_ptr,
                 b = &bundle](const fastgltf::sources::Array& vec) -> tl::expected<std::size_t, GeometryImportError> {
                    return decode_image_from_memory_fn(reinterpret_cast<const stbi_uc*>(vec.bytes.data()),
                                                       vec.bytes.size_bytes(),
                                                       "bytesvec",
                                                       b,
                                                       buff_ptr);
                },
                [buff_ptr, g = gltf_asset.get(), b = &bundle](
                    const fastgltf::sources::BufferView& view) -> tl::expected<std::size_t, GeometryImportError> {
                    auto& buffer_view = g->bufferViews[view.bufferViewIndex];
                    auto& buffer = g->buffers[buffer_view.bufferIndex];

                    if (const fastgltf::sources::Array* arr = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
                        return decode_image_from_memory_fn(reinterpret_cast<const stbi_uc*>(arr->bytes.data()),
                                                           arr->bytes.size_bytes(),
                                                           "array",
                                                           b,
                                                           buff_ptr);
                    }

                    return std::size_t{};
                } },
            image.data);

        if (!extract_result)
            return tl::make_unexpected(extract_result.error());

        buff_ptr = reinterpret_cast<std::byte*>(buff_ptr) + *extract_result;
    }

    ranges::transform(
        gltf_asset->materials,
        back_inserter(bundle.materials),
        [g = this->gltf_asset.get()](const fastgltf::Material& mtl) {
            uint32_t null_texture{};
            return ExtractedMaterialDefinition{
                mtl.name.c_str(),
                mtl.pbrData.baseColorTexture
                    ? null_texture
                    : static_cast<uint32_t>(*g->textures[mtl.pbrData.baseColorTexture->textureIndex].imageIndex),
                mtl.pbrData.metallicRoughnessTexture
                    ? null_texture
                    : static_cast<uint32_t>(
                          *g->textures[mtl.pbrData.metallicRoughnessTexture->textureIndex].imageIndex),
                mtl.emissiveTexture ? null_texture
                                    : static_cast<uint32_t>(*g->textures[mtl.emissiveTexture->textureIndex].imageIndex),
                math::vec4f{
                    mtl.pbrData.baseColorFactor.x(),
                    mtl.pbrData.baseColorFactor.y(),
                    mtl.pbrData.baseColorFactor.z(),
                    mtl.pbrData.baseColorFactor.w(),
                },
                mtl.pbrData.metallicFactor,
                mtl.pbrData.roughnessFactor,
            };
        });

    return tl::expected<ExtractedMaterialsWithImageSourcesBundle, GeometryImportError>{ std::move(bundle) };
}

ExtractedMaterialsWithImageSourcesBundle
LoadedGeometry::extract_materials(const uint32_t null_texture) const noexcept
{
    ExtractedMaterialsWithImageSourcesBundle result{};

    // ranges::transform(gltf_asset->images,
    //                   back_inserter(result.image_sources),
    //                   [gltf = this->gltf_asset.get()](const fastgltf::Image& img) {
    //                       std::visit(base::VariantVisitor{ [](auto arg) {},
    //                                                        [](const fastgltf::sources::URI& file_path) {
    //                                                            //
    //                                                            // TODO: handle loading from local file
    //                                                        },
    //                                                        [](const fastgltf::sources::Array& vec) {
    //                                                        return ExtractedImageData {
    //                                                        img.name.data(),
    //                                                        }
    //                                                        },
    //                                                        [](const fastgltf::sources::BufferView& buffer_view)
    //                                                        {}
    //
    //                                  },
    //                                  img);
    //
    //                       // if (!img.image.empty()) {
    //     return ExtractedImageData{
    //         img.name, (uint32_t)img.width, (uint32_t)img.height, (uint8_t)img.bits, std::span{
    //         img.image }
    //     };
    // }
    //
    // const size_t bytes_per_channel = [i = &img]() {
    //     assert(i->pixel_type != -1);
    //
    //     switch (i->pixel_type) {
    //         default:
    //         case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    //             return 1;
    //
    //         case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    //             return 2;
    //
    //         case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    //             return 4;
    //     }
    // }();
    //
    // //
    // // in a buffer ?!
    // assert(img.bufferView != -1);
    // const tinygltf::BufferView& buffer_view{ gltf->bufferViews[img.bufferView] };
    // const tinygltf::Buffer& buffer{ gltf->buffers[buffer_view.buffer] };
    // return ExtractedImageData{ img.name,
    //                            (uint32_t)img.width,
    //                            (uint32_t)img.height,
    //                            (uint8_t)img.bits,
    //                            std::span{ buffer.data.data() + buffer_view.byteOffset,
    //                                       (size_t)(img.width * img.height * bytes_per_channel)
    //                                       } };
    // });

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
                math::vec4f{
                    mtl.pbrMetallicRoughness.baseColorFactor[0],
                    mtl.pbrMetallicRoughness.baseColorFactor[1],
                    mtl.pbrMetallicRoughness.baseColorFactor[2],
                    mtl.pbrMetallicRoughness.baseColorFactor[3],
                },
                (float)mtl.pbrMetallicRoughness.metallicFactor,
                (float)mtl.pbrMetallicRoughness.roughnessFactor
            };
        });

    // ranges::transform(
    //     gltf->images, back_inserter(result.image_sources), [gltf = this->gltf.get()](const tinygltf::Image& img)
    //     {
    //         if (!img.image.empty()) {
    //             return ExtractedImageData{
    //                 img.name, (uint32_t)img.width, (uint32_t)img.height, (uint8_t)img.bits, std::span{ img.image
    //                 }
    //             };
    //         }
    //
    //         const size_t bytes_per_channel = [i = &img]() {
    //             assert(i->pixel_type != -1);
    //
    //             switch (i->pixel_type) {
    //                 default:
    //                 case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    //                     return 1;
    //
    //                 case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    //                     return 2;
    //
    //                 case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    //                     return 4;
    //             }
    //         }();
    //
    //         //
    //         // in a buffer ?!
    //         assert(img.bufferView != -1);
    //         const tinygltf::BufferView& buffer_view{ gltf->bufferViews[img.bufferView] };
    //         const tinygltf::Buffer& buffer{ gltf->buffers[buffer_view.buffer] };
    //         return ExtractedImageData{ img.name,
    //                                    (uint32_t)img.width,
    //                                    (uint32_t)img.height,
    //                                    (uint8_t)img.bits,
    //                                    std::span{ buffer.data.data() + buffer_view.byteOffset,
    //                                               (size_t)(img.width * img.height * bytes_per_channel) } };
    //     });
    //
    // ranges::transform(
    //     gltf->materials,
    //     back_inserter(result.materials),
    //     [g = this->gltf.get(), null_texture](const tinygltf::Material& mtl) {
    //         uint32_t null_texture{};
    //         return ExtractedMaterialDefinition{
    //             mtl.name,
    //             mtl.pbrMetallicRoughness.baseColorTexture.index == -1
    //                 ? null_texture
    //                 : (uint32_t)g->textures[mtl.pbrMetallicRoughness.baseColorTexture.index].source,
    //             mtl.pbrMetallicRoughness.metallicRoughnessTexture.index == -1
    //                 ? null_texture
    //                 : (uint32_t)g->textures[mtl.pbrMetallicRoughness.metallicRoughnessTexture.index].source,
    //             mtl.normalTexture.index == -1 ? null_texture :
    //             (uint32_t)g->textures[mtl.normalTexture.index].source, math::vec4f{
    //                 mtl.pbrMetallicRoughness.baseColorFactor[0],
    //                 mtl.pbrMetallicRoughness.baseColorFactor[1],
    //                 mtl.pbrMetallicRoughness.baseColorFactor[2],
    //                 mtl.pbrMetallicRoughness.baseColorFactor[3],
    //             },
    //             (float)mtl.pbrMetallicRoughness.metallicFactor,
    //             (float)mtl.pbrMetallicRoughness.roughnessFactor
    //         };
    //     });

    return result;
}

math::vec2ui32
LoadedGeometry::extract_data(void* vertex_buffer,
                             void* index_buffer,
                             const math::vec2ui32 offsets,
                             const uint32_t mtl_offset)
{
    math::vec2ui32 buffer_offsets{ offsets };
    for (const fastgltf::Scene& s : gltf_asset->scenes) {
        for (const size_t node_idx : s.nodeIndices) {
            extract_single_node_data(
                vertex_buffer, index_buffer, &buffer_offsets, mtl_offset, gltf_asset->nodes[node_idx], tl::nullopt);
        }
    }

    // for (const tinygltf::Scene& s : gltf->scenes) {
    //     for (const int node_idx : s.nodes) {
    //         extract_single_node_data(
    //             vertex_buffer, index_buffer, &buffer_offsets, mtl_offset, gltf->nodes[node_idx], tl::nullopt);
    //     }
    // }

    bounding_box = math::aabb3f::stdc::identity;
    for (size_t idx = 0, count = nodes.size(); idx < count; ++idx) {
        bounding_box = math::merge(bounding_box, nodes[idx].boundingbox);
    }
    bounding_sphere = math::sphere3f{ bounding_box.center(), math::length(bounding_box.extents()) };

    return buffer_offsets;
}

void
LoadedGeometry::extract_single_node_data(void* vertex_buffer,
                                         void* index_buffer,
                                         math::vec2ui32* buffer_offsets,
                                         const uint32_t mtl_offset,
                                         const tinygltf::Node& node,
                                         const tl::optional<uint32_t> parent)
{
    const math::mat4f node_transform = [n = &node]() {
        if (!n->matrix.empty()) {
            return math::transpose(math::mat4f{ n->matrix.data(), n->matrix.size() });
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
    this->nodes.emplace_back(parent,
                             node.name,
                             math::mat4f{ node_transform },
                             math::aabb3f::stdc::identity,
                             math::sphere3f::stdc::null,
                             0,
                             0,
                             0,
                             0);

    for (const int child_idx : node.children) {
        extract_single_node_data(vertex_buffer,
                                 index_buffer,
                                 buffer_offsets,
                                 mtl_offset,
                                 gltf->nodes[child_idx],
                                 tl::optional<uint32_t>{ node_id });
    }

    this->nodes[node_id].vertex_offset = buffer_offsets->x;
    this->nodes[node_id].index_offset = buffer_offsets->y;

    if (node.mesh != -1) {
        tl::optional<uint32_t> ancestor{ parent };
        math::mat4f transform{ node_transform };

        while (ancestor) {
            const GeometryNode* a = &nodes[*ancestor];
            transform = a->transform * transform;
            ancestor = a->parent;
        }

        nodes[node_id].transform = transform;
        const math::mat4f normals_matrix = math::transpose(math::invert(transform));

        const tinygltf::Mesh* mesh = &gltf->meshes[node.mesh];
        for (const tinygltf::Primitive& prim : mesh->primitives) {
            assert(prim.mode == TINYGLTF_MODE_TRIANGLES);
            assert(prim.material != -1);

            if (prim.indices == -1) {
                XR_LOG_ERR("Unindexed geometry is not supported.");
                continue;
            }

            VertexPBR* dst_data{ reinterpret_cast<VertexPBR*>(vertex_buffer) + buffer_offsets->x };

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

                    math::aabb3f* bbox = &nodes[node_id].boundingbox;
                    bbox->min = math::min(bbox->min, dst_data[vtx].pos);
                    bbox->max = math::max(bbox->max, dst_data[vtx].pos);
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
                    dst_data[vtx].color = src_data[vtx];
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

                uint32_t* dst_data{ reinterpret_cast<uint32_t*>(index_buffer) + buffer_offsets->y };

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
                    dst_data[idx] += buffer_offsets->x;
                }

                buffer_offsets->x += primitive_vertices;
                buffer_offsets->y += accessor.count;
                this->nodes[node_id].index_count += accessor.count;
                this->nodes[node_id].vertex_count += primitive_vertices;
            }
        }
    }

    //
    // make sphere from the bounding box
    GeometryNode* this_node = &nodes[node_id];
    this_node->bounding_sphere =
        math::sphere3f{ this_node->boundingbox.center(), math::length(this_node->boundingbox.extents()) };

    XR_LOG_INFO("Node {}, offsets {} {}, vertices {}, indices {}",
                nodes[node_id].name,
                nodes[node_id].vertex_offset,
                nodes[node_id].index_offset,
                nodes[node_id].vertex_count,
                nodes[node_id].index_count);
}

void
LoadedGeometry::extract_single_node_data(void* vertex_buffer,
                                         void* index_buffer,
                                         math::vec2ui32* buffer_offsets,
                                         const uint32_t mtl_offset,
                                         const fastgltf::Node& node,
                                         const tl::optional<uint32_t> parent)
{
    const math::mat4f node_transform =
        std::visit(base::VariantVisitor{
                       [](const fastgltf::TRS& trs) {
                           const math::mat4f scale = math::R4::scaling(trs.scale.x(), trs.scale.y(), trs.scale.z());

                           const math::mat4f rotation = math::rotation_matrix(math::quatf{
                               static_cast<float>(trs.rotation.w()),
                               static_cast<float>(trs.rotation.x()),
                               static_cast<float>(trs.rotation.y()),
                               static_cast<float>(trs.rotation.z()),
                           });

                           const math::mat4f translate = math::R4::translate(static_cast<float>(trs.translation.x()),
                                                                             static_cast<float>(trs.translation.y()),
                                                                             static_cast<float>(trs.translation.z()));

                           return translate * rotation * scale;
                       },
                       [](const fastgltf::math::fmat4x4& mtx) {
                           return math::transpose(math::mat4f{ mtx.data(), 16 });
                       },
                   },
                   node.transform);

    const uint32_t node_id = static_cast<uint32_t>(nodes.size());
    this->nodes.emplace_back(parent,
                             string{ node.name.data() },
                             math::mat4f{ node_transform },
                             math::aabb3f::stdc::identity,
                             math::sphere3f::stdc::null,
                             0,
                             0,
                             0,
                             0);

    for (const size_t child_idx : node.children) {
        extract_single_node_data(vertex_buffer,
                                 index_buffer,
                                 buffer_offsets,
                                 mtl_offset,
                                 gltf_asset->nodes[child_idx],
                                 tl::optional<uint32_t>{ node_id });
    }

    this->nodes[node_id].vertex_offset = buffer_offsets->x;
    this->nodes[node_id].index_offset = buffer_offsets->y;

    if (node.meshIndex) {
        tl::optional<uint32_t> ancestor{ parent };
        math::mat4f transform{ node_transform };

        while (ancestor) {
            const GeometryNode* a = &nodes[*ancestor];
            transform = a->transform * transform;
            ancestor = a->parent;
        }

        nodes[node_id].transform = transform;
        const math::mat4f normals_matrix = math::transpose(math::invert(transform));

        const fastgltf::Mesh* mesh = &gltf_asset->meshes[*node.meshIndex];
        for (const fastgltf::Primitive& prim : mesh->primitives) {
            assert(prim.type == fastgltf::PrimitiveType::Triangles);
            assert(prim.materialIndex.has_value());

            if (!prim.indicesAccessor) {
                XR_LOG_ERR("Unindexed geometry is not supported.");
                continue;
            }

            VertexPBR* dst_data{ reinterpret_cast<VertexPBR*>(vertex_buffer) + buffer_offsets->x };

            uint32_t primitive_vertices{};
            if (auto attr_itr =
                    ranges::find_if(prim.attributes, [](const fastgltf::Attribute& a) { return a.name == "POSITION"; });
                attr_itr != cend(prim.attributes)) {

                const fastgltf::Accessor& accessor = gltf_asset->accessors[attr_itr->accessorIndex];
                assert(accessor.componentType == fastgltf::ComponentType::Float);
                assert(accessor.type == fastgltf::AccessorType::Vec3);

                assert(accessor.bufferViewIndex);
                const fastgltf::BufferView& buffer_view = gltf_asset->bufferViews[*accessor.bufferViewIndex];

                assert(buffer_view.target == fastgltf::BufferTarget::ArrayBuffer);
                // assert(buffer_view.bufferIndex);
                const fastgltf::Buffer& buffer = gltf_asset->buffers[buffer_view.bufferIndex];

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    *gltf_asset, accessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                        dst_data[idx].pos = math::mul_point(transform, math::vec3f{ pos.x(), pos.y(), pos.z() });

                        dst_data[idx].color = math::vec4f{ 0.0f, 0.0f, 0.0f, 1.0f };
                        dst_data[idx].pbr_buf_id = (uint32_t)*prim.materialIndex + mtl_offset;

                        math::aabb3f* bbox = &nodes[node_id].boundingbox;
                        bbox->min = math::min(bbox->min, dst_data[idx].pos);
                        bbox->max = math::max(bbox->max, dst_data[idx].pos);
                    });

                primitive_vertices = accessor.count;
            } else {
                XR_LOG_ERR("Missing POSITION attribute");
                continue;
            }

            if (auto attr_normals = prim.findAttribute("NORMAL"); attr_normals != cend(prim.attributes)) {
                const fastgltf::Accessor& accessor = gltf_asset->accessors[attr_normals->accessorIndex];

                assert(accessor.componentType == fastgltf::ComponentType::Float);
                assert(accessor.type == fastgltf::AccessorType::Vec3);

                assert(accessor.bufferViewIndex);
                const fastgltf::BufferView& buffer_view = gltf_asset->bufferViews[*accessor.bufferViewIndex];
                assert(buffer_view.target == fastgltf::BufferTarget::ArrayBuffer);

                const fastgltf::Buffer& buffer = gltf_asset->buffers[buffer_view.bufferIndex];

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                    *gltf_asset, accessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                        dst_data[idx].normal =
                            math::normalize(math::mul_vec(normals_matrix, math::vec3f{ pos.x(), pos.y(), pos.z() }));
                    });
            }

            if (auto attr_texcoords = prim.findAttribute("TEXCOORD_0"); attr_texcoords != cend(prim.attributes)) {
                const fastgltf::Accessor& accessor = gltf_asset->accessors[attr_texcoords->accessorIndex];

                assert(accessor.componentType == fastgltf::ComponentType::Float);
                assert(accessor.type == fastgltf::AccessorType::Vec2);

                assert(accessor.bufferViewIndex);
                const fastgltf::BufferView& buffer_view = gltf_asset->bufferViews[*accessor.bufferViewIndex];
                assert(buffer_view.target == fastgltf::BufferTarget::ArrayBuffer);

                const fastgltf::Buffer& buffer = gltf_asset->buffers[buffer_view.bufferIndex];

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                    *gltf_asset, accessor, [&](fastgltf::math::fvec2 pos, std::size_t idx) {
                        dst_data[idx].uv = math::vec2f{
                            pos.x(),
                            pos.y(),
                        };
                    });
            }

            if (auto attr_tangent = prim.findAttribute("TANGENT"); attr_tangent != cend(prim.attributes)) {
                const fastgltf::Accessor& accessor = gltf_asset->accessors[attr_tangent->accessorIndex];

                assert(accessor.componentType == fastgltf::ComponentType::Float);
                assert(accessor.type == fastgltf::AccessorType::Vec4);

                const fastgltf::BufferView& buffer_view = gltf_asset->bufferViews[*accessor.bufferViewIndex];
                assert(buffer_view.target == fastgltf::BufferTarget::ArrayBuffer);

                const fastgltf::Buffer& buffer = gltf_asset->buffers[buffer_view.bufferIndex];

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                    *gltf_asset, accessor, [&](fastgltf::math::fvec4 pos, std::size_t idx) {
                        dst_data[idx].tangent = math::vec4f{ pos.x(), pos.y(), pos.z(), pos.w() };
                    });
            }

            if (auto attr_color = prim.findAttribute("COLOR_0"); attr_color != cend(prim.attributes)) {
                const fastgltf::Accessor& accessor = gltf_asset->accessors[attr_color->accessorIndex];

                assert(accessor.componentType == fastgltf::ComponentType::Float);
                assert(accessor.type == fastgltf::AccessorType::Vec4);

                const fastgltf::BufferView& buffer_view = gltf_asset->bufferViews[*accessor.bufferViewIndex];
                assert(buffer_view.target == fastgltf::BufferTarget::ArrayBuffer);

                const fastgltf::Buffer& buffer = gltf_asset->buffers[buffer_view.bufferIndex];

                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(
                    *gltf_asset, accessor, [&](fastgltf::math::fvec4 pos, std::size_t idx) {
                        dst_data[idx].color = math::vec4f{ pos.x(), pos.y(), pos.z(), pos.w() };
                    });
            }

            //
            // indices
            {
                assert(prim.indicesAccessor);

                const fastgltf::Accessor& accessor = gltf_asset->accessors[*prim.indicesAccessor];
                assert(accessor.componentType == fastgltf::ComponentType::UnsignedShort ||
                       accessor.componentType == fastgltf::ComponentType::UnsignedInt);
                assert(accessor.type == fastgltf::AccessorType::Scalar);

                assert(accessor.bufferViewIndex);
                const fastgltf::BufferView& buffer_view = gltf_asset->bufferViews[*accessor.bufferViewIndex];
                // assert(buffer_view.byteStride == 0);
                const fastgltf::Buffer& buffer = gltf_asset->buffers[buffer_view.bufferIndex];

                uint32_t* dst_data{ reinterpret_cast<uint32_t*>(index_buffer) + buffer_offsets->y };

                if (accessor.componentType == fastgltf::ComponentType::UnsignedShort) {
                    fastgltf::iterateAccessorWithIndex<uint16_t>(
                        *gltf_asset, accessor, [&](uint16_t value, std::size_t idx) { dst_data[idx] = value; });

                } else {
                    fastgltf::copyFromAccessor<uint32_t>(*gltf_asset, accessor, dst_data);
                }

                //
                // correct indices to account for the vertex offset
                for (size_t idx = 0; idx < static_cast<size_t>(accessor.count); ++idx) {
                    dst_data[idx] += buffer_offsets->x;
                }

                buffer_offsets->x += primitive_vertices;
                buffer_offsets->y += accessor.count;
                this->nodes[node_id].index_count += accessor.count;
                this->nodes[node_id].vertex_count += primitive_vertices;
            }
        }
    }

    //
    // make sphere from the bounding box
    GeometryNode* this_node = &nodes[node_id];
    this_node->bounding_sphere =
        math::sphere3f{ this_node->boundingbox.center(), math::length(this_node->boundingbox.extents()) };

    XR_LOG_INFO("Node {}, offsets {} {}, vertices {}, indices {}",
                nodes[node_id].name,
                nodes[node_id].vertex_offset,
                nodes[node_id].index_offset,
                nodes[node_id].vertex_count,
                nodes[node_id].index_count);
}
}
