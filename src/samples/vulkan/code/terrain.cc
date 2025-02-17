#include "terrain.hpp"

#include <utility>
#include <Lz/Lz.hpp>
#include <imgui/imgui.h>
#include <imgui/IconsFontAwesome.h>

#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/memory.arena.unique.ptr.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/scene/scene.definition.hpp"
#include "events.hpp"
#include "push.constant.packer.hpp"
#include "bindless.pipeline.config.hpp"

using namespace std;
using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;

struct TerrainVertex
{
    vec3f pos;
    vec2f uv;
    // vec2ui32 xy;

    static constexpr TerrainVertex identity() noexcept
    {
        return TerrainVertex{
            .pos = vec3f::stdc::zero, .uv = vec2f::stdc::zero,
            // .xy = vec2ui32::stdc::zero,
        };
    }
};

B5::Terrain::Terrain(PrivateConstructionToken,
                     xray::rendering::VulkanBuffer&& vertexbuffer,
                     xray::rendering::VulkanBuffer&& indexbuffer,
                     xray::rendering::BindlessStorageBufferResourceHandleEntryPair instances,
                     xray::rendering::BindlessImageResourceHandleEntryPair hmap,
                     xray::rendering::BindlessImageResourceHandleEntryPair cmap,
                     xray::base::containers::vector<TerrainLodLevel>&& lod_levels,
                     xray::base::unique_pointer<NoiseGen>&& noise_gen,
                     TerrainParams terrain_params)
    : _noise_gen{ std::move(noise_gen) }
    , _terrain_params{ terrain_params }
    , _renderstate{
        RenderResources{
            .vertexbuffer = std::move(vertexbuffer),
            .indexbuffer = std::move(indexbuffer),
            .instances = instances,
            .heightmap = hmap,
            .colormap = cmap,
            .lod_levels = std::move(lod_levels),
        },
    }
{
}

struct TerrainDetails
{
    uint32_t lod_factor;
    uint32_t points;
    uint32_t vertices;
    uint32_t indices;

    TerrainDetails& operator+=(const TerrainDetails& rhs) noexcept
    {
        lod_factor += rhs.lod_factor;
        points += rhs.points;
        vertices += rhs.vertices;
        indices += rhs.indices;

        return *this;
    }
};

inline TerrainDetails
operator+(const TerrainDetails& a, const TerrainDetails& b) noexcept
{
    TerrainDetails r{ a };
    r += b;
    return r;
}

TerrainDetails
compute_terrain_details_lod(const TerrainParams& params, const uint32_t lod) noexcept
{
    assert(is_power_of_two(params.size));
    assert(lod <= 8);

    const uint32_t lod_factor = lod ? 2 << (lod - 1) : 1;

    const uint32_t points_count = params.size / lod_factor + 1;
    const uint32_t vertex_count = points_count * points_count;
    const uint32_t faces = (points_count - 1) * (points_count - 1) * 2;
    const uint32_t index_count = faces * 3;

    return TerrainDetails{
        .lod_factor = lod_factor,
        .points = points_count,
        .vertices = vertex_count,
        .indices = index_count,
    };
}

vec2ui32
make_terrain_grid(const TerrainParams& params,
                  const uint32_t lod,
                  std::span<TerrainVertex> buffer_vertex,
                  std::span<uint32_t> buffer_index)
{
    assert(is_power_of_two(params.size));
    assert(lod <= 8);

    const auto [lod_factor, points_count, vertex_count, index_count] = compute_terrain_details_lod(params, lod);

    const float hx = static_cast<float>(params.size) * 0.5f;
    const float hz = static_cast<float>(params.size) * 0.5f;
    const float du = 1.0f / (static_cast<float>(points_count));
    const float dz = 1.0f / (static_cast<float>(points_count));

    assert(buffer_vertex.size() == vertex_count);
    assert(buffer_index.size() == index_count);

    for (size_t z = 0; z < points_count; ++z) {
        for (size_t x = 0; x < points_count; ++x) {
            buffer_vertex[z * points_count + x].pos =
                vec3f{ static_cast<float>(x * lod_factor) - hx, .0f, static_cast<float>(z * lod_factor) - hz };
            buffer_vertex[z * points_count + x].uv =
                vec2f{ static_cast<float>(x) * du, 1.0f - static_cast<float>(z) * dz };
        }
    }

    uint32_t* idx = buffer_index.data();
    for (size_t z = 0; z < points_count - 1; ++z) {
        for (size_t x = 0; x < points_count - 1; ++x) {

            *idx++ = static_cast<uint32_t>(z * points_count + x);
            *idx++ = static_cast<uint32_t>(z * points_count + x + 1);
            *idx++ = static_cast<uint32_t>((z + 1) * points_count + x);

            *idx++ = static_cast<uint32_t>(z * points_count + x + 1);
            *idx++ = static_cast<uint32_t>((z + 1) * points_count + x + 1);
            *idx++ = static_cast<uint32_t>((z + 1) * points_count + x);
        }
    }

    return vec2ui32{ vertex_count, index_count };
}

void
make_terrain_heightmap_colormap(const xray::rendering::TerrainParams& params,
                                B5::NoiseGen& noise_gen,
                                containers::vector<float>& heightmap,
                                containers::vector<vec4ui8>& colormap)
{
    noise_gen.base_flat_terrain.SetFrequency(2.0);

    noise::module::ScaleBias flat_terrain;
    flat_terrain.SetSourceModule(0, noise_gen.base_flat_terrain);
    flat_terrain.SetScale(params.scale);
    flat_terrain.SetBias(params.bias);

    noise_gen.terrain_type.SetOctaveCount(params.octaves);
    noise_gen.terrain_type.SetFrequency(0.5);
    noise_gen.terrain_type.SetPersistence(0.25);

    noise_gen.terrain_selector.SetSourceModule(0, flat_terrain);
    noise_gen.terrain_selector.SetSourceModule(1, noise_gen.ridged);
    noise_gen.terrain_selector.SetControlModule(noise_gen.terrain_type);
    noise_gen.terrain_selector.SetBounds(0.0f, 1000.0f);
    noise_gen.terrain_selector.SetEdgeFalloff(0.125);

    noise_gen.final_terrain.SetSourceModule(0, noise_gen.terrain_selector);
    noise_gen.final_terrain.SetFrequency(4.0);
    noise_gen.final_terrain.SetPower(0.125);

    noise_gen.heightmap_builder.SetDestNoiseMap(noise_gen.heightmap);
    noise_gen.heightmap_builder.SetSourceModule(noise_gen.final_terrain);
    noise_gen.heightmap_builder.SetDestSize(params.size, params.size);
    noise_gen.heightmap_builder.SetBounds(params.xmin, params.xmax, params.zmin, params.zmax);
    noise_gen.heightmap_builder.EnableSeamless();
    noise_gen.heightmap_builder.Build();

    heightmap.resize(params.size * params.size);
    for (size_t z = 0; z < params.size; ++z) {
        for (size_t x = 0; x < params.size; ++x) {
            heightmap[z * params.size + x] = noise_gen.heightmap.GetValue(x, z);
        }
    }

    utils::Image height_map_image;
    utils::Image normal_map_image;
    utils::RendererImage renderer_img;
    renderer_img.SetSourceNoiseMap(noise_gen.heightmap);
    renderer_img.SetDestImage(height_map_image);
    renderer_img.EnableWrap();
    renderer_img.ClearGradient();

    renderer_img.AddGradientPoint(-16384.0 + params.sea_level, utils::Color(3, 29, 63, 255));
    renderer_img.AddGradientPoint(-256.0 + params.sea_level, utils::Color(3, 29, 63, 255));
    renderer_img.AddGradientPoint(-1.0 + params.sea_level, utils::Color(7, 106, 127, 255));
    renderer_img.AddGradientPoint(0.0 + params.sea_level, utils::Color(62, 86, 30, 255));
    renderer_img.AddGradientPoint(4.0 + params.sea_level, utils::Color(84, 96, 50, 255));
    renderer_img.AddGradientPoint(8.0 + params.sea_level, utils::Color(130, 127, 97, 255));
    renderer_img.AddGradientPoint(12.0 + params.sea_level, utils::Color(184, 163, 141, 255));
    renderer_img.AddGradientPoint(36.0 + params.sea_level, utils::Color(255, 255, 255, 255));
    renderer_img.AddGradientPoint(44.0 + params.sea_level, utils::Color(128, 255, 255, 255));
    renderer_img.AddGradientPoint(84.0 + params.sea_level, utils::Color(0, 0, 255, 255));

    // renderer_img.AddGradientPoint(-1.0000, utils::Color(0, 0, 128, 255));    // deeps
    // renderer_img.AddGradientPoint(-0.2500, utils::Color(0, 0, 255, 255));    // shallow
    // renderer_img.AddGradientPoint(0.0000, utils::Color(0, 128, 255, 255));   // shore
    // renderer_img.AddGradientPoint(0.0625, utils::Color(240, 240, 64, 255));  // sand
    // renderer_img.AddGradientPoint(0.1250, utils::Color(32, 160, 0, 255));    // grass
    // renderer_img.AddGradientPoint(0.3750, utils::Color(224, 224, 0, 255));   // dirt
    // renderer_img.AddGradientPoint(0.7500, utils::Color(128, 128, 128, 255)); // rock
    // renderer_img.AddGradientPoint(1.0000, utils::Color(255, 255, 255, 255)); // snow
    renderer_img.Render();

    colormap.resize(params.size * params.size);
    for (size_t z = 0; z < params.size; ++z) {
        for (size_t x = 0; x < params.size; ++x) {
            const auto color = height_map_image.GetValue(x, z);
            colormap[z * params.size + x] = vec4ui8{ color.red, color.green, color.blue, color.alpha };
        }
    }
}

tl::expected<B5::Terrain, VulkanError>
B5::Terrain::create(const InitContext& ctx)
{
    TerrainParams params{ ctx.scene_def->terrain_params };

    containers::vector<TerrainDetails> lod_levels =
        lz::chain(lz::range(uint32_t{}, params.lods ? params.lods : 1))
            .map([&params](uint32_t lod) { return compute_terrain_details_lod(params, lod); })
            .toVector(MemoryArenaAllocator<TerrainDetails>(*ctx.temp), std::execution::seq);

    for (const TerrainDetails& td : lod_levels) {
        XR_LOG_INFO("Lod vtx: {} idx: {}", td.vertices, td.indices);
    }

    const containers::vector<vec2ui32> lod_offsets =
        lz::eScan(lz::chain(lod_levels).map([](const TerrainDetails& td) {
            return vec2ui32{ td.vertices, td.indices };
        }),
                  vec2ui32::stdc::zero,
                  std::plus<vec2ui32>{})
            .toVector(MemoryArenaAllocator<vec2ui32>(*ctx.temp), std::execution::seq);

    for (const vec2ui32 v : lod_offsets) {
        XR_LOG_INFO("LOD offsets: {} {}", v.x, v.y);
    }

    const vec2ui32 vertex_index_counts = lz::chain(lod_levels)
                                             .map([](const TerrainDetails& td) {
                                                 return vec2ui32{ td.vertices, td.indices };
                                             })
                                             .sum();

    XR_LOG_INFO("Terrain generator: lod levels: {}, vertices:{}, indices: {}",
                params.lods,
                vertex_index_counts.x,
                vertex_index_counts.y);

    containers::vector<TerrainVertex> vertices{ size_t{ vertex_index_counts.x }, *ctx.temp };
    containers::vector<uint32_t> indices{ size_t{ vertex_index_counts.y }, *ctx.temp };

    lz::chain(lz::range(uint32_t{}, params.lods ? params.lods : 1)).forEach([&](const uint32_t lod) {
        make_terrain_grid(params,
                          lod,
                          std::span{ vertices.data() + lod_offsets[lod].x, lod_levels[lod].vertices },
                          std::span{ indices.data() + lod_offsets[lod].y, lod_levels[lod].indices });
    });

    // make_terrain_grid(params, 1, vertices, indices);

    containers::vector<float> heightmap_data{ *ctx.temp };
    containers::vector<vec4ui8> colormap_data{ *ctx.temp };

    xray::base::unique_pointer<NoiseGen> noise_gen{ xray::base::make_unique<NoiseGen>() };
    make_terrain_heightmap_colormap(params, *noise_gen, heightmap_data, colormap_data);

    // for (uint32_t z = 0; z < params.height; ++z) {
    //     for (uint32_t x = 0; x < params.height; ++x) {
    //         vertices[z * params.width + x].pos.y = noise_gen->heightmap.GetValue(x, z);
    //     }
    // }

    auto terrain_buffers_job = ctx.renderer->create_job(QueueType::Transfer);
    XR_VK_PROPAGATE_ERROR(terrain_buffers_job);

    auto vertex_buffer = VulkanBuffer::create(*ctx.renderer,
                                              VulkanBufferCreateInfo{
                                                  .name_tag = "Terrain VB",
                                                  .job_cmd_buf = terrain_buffers_job->buffer,
                                                  .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                  .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                  .bytes = container_bytes_size(vertices),
                                                  .frames = ctx.renderer->max_inflight_frames(),
                                                  .initial_data = { to_bytes_span(vertices) },
                                              });
    XR_VK_PROPAGATE_ERROR(vertex_buffer);

    auto index_buffer = VulkanBuffer::create(*ctx.renderer,
                                             VulkanBufferCreateInfo{
                                                 .name_tag = "Terrain IB",
                                                 .job_cmd_buf = terrain_buffers_job->buffer,
                                                 .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                 .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                 .bytes = container_bytes_size(indices),
                                                 .frames = ctx.renderer->max_inflight_frames(),
                                                 .initial_data = { to_bytes_span(indices) },
                                             });
    XR_VK_PROPAGATE_ERROR(index_buffer);
    auto wait_token_buffers = ctx.renderer->submit_job(std::move(*terrain_buffers_job));
    XR_VK_PROPAGATE_ERROR(wait_token_buffers);

    auto terrain_images_job = ctx.renderer->create_job(QueueType::Transfer);
    XR_VK_PROPAGATE_ERROR(terrain_images_job);

    auto heightmap_texture = VulkanImage::from_memory(*ctx.renderer,
                                                      VulkanImageCreateInfo{
                                                          .tag_name = "Terrain Heightmap",
                                                          .wpkg = terrain_images_job->buffer,
                                                          .type = VK_IMAGE_TYPE_2D,
                                                          .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                                                          .memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                          .format = VK_FORMAT_R32_SFLOAT,
                                                          .width = params.size,
                                                          .height = params.size,
                                                          .layers = 1,
                                                          .pixels = { to_bytes_span(heightmap_data) },
                                                      });
    XR_VK_PROPAGATE_ERROR(heightmap_texture);

    auto colormap_texture = VulkanImage::from_memory(*ctx.renderer,
                                                     VulkanImageCreateInfo{
                                                         .tag_name = "Terrain Colormap",
                                                         .wpkg = terrain_images_job->buffer,
                                                         .type = VK_IMAGE_TYPE_2D,
                                                         .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                                                         .memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                         .format = VK_FORMAT_R8G8B8A8_UNORM,
                                                         .width = params.size,
                                                         .height = params.size,
                                                         .layers = 1,
                                                         .pixels = { to_bytes_span(colormap_data) },
                                                     });
    XR_VK_PROPAGATE_ERROR(colormap_texture);
    auto wait_token_images = ctx.renderer->submit_job(std::move(*terrain_images_job));
    XR_VK_PROPAGATE_ERROR(wait_token_images);

    auto sampler = ctx.renderer->bindless_sys().get_sampler(
        VkSamplerCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
            .mipLodBias = 0.0f,
            .anisotropyEnable = false,
            .maxAnisotropy = 1.0f,
            .compareEnable = false,
            .compareOp = VK_COMPARE_OP_NEVER,
            .minLod = 0.0f,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = false,
        },
        *ctx.renderer);

    BindlessImageResourceHandleEntryPair heightmap =
        ctx.renderer->bindless_sys().add_image(std::move(*heightmap_texture), *sampler, tl::nullopt);
    BindlessImageResourceHandleEntryPair colormap =
        ctx.renderer->bindless_sys().add_image(std::move(*colormap_texture), *sampler, tl::nullopt);

    ctx.renderer->queue_image_ownership_transfer(heightmap.first);
    ctx.renderer->queue_image_ownership_transfer(colormap.first);

    auto terrain_instances_buffer = VulkanBuffer::create(*ctx.renderer,
                                                         VulkanBufferCreateInfo{
                                                             .name_tag = "Terrain Instances",
                                                             .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                             .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                                             .bytes = sizeof(TerrainInstanceData) * 8,
                                                             .frames = ctx.renderer->max_inflight_frames(),
                                                         });
    XR_VK_PROPAGATE_ERROR(terrain_instances_buffer);

    BindlessStorageBufferResourceHandleEntryPair instances = ctx.renderer->bindless_sys().add_chunked_storage_buffer(
        std::move(*terrain_instances_buffer), ctx.renderer->max_inflight_frames(), tl::nullopt);

    containers::vector<TerrainLodLevel> lod_lvls =
        lz::chain(lz::zip(lod_levels, lod_offsets))
            .map([](tuple<const TerrainDetails&, const vec2ui32&> p) {
                const auto& [lvl, off] = p;
                return TerrainLodLevel{
                    .offset_vertex = off.x,
                    .offset_index = off.y,
                    .vertex_count = lvl.vertices,
                    .index_count = lvl.indices,
                };
            })
            .toVector(MemoryArenaAllocator<TerrainLodLevel>(*ctx.perm), std::execution::seq);

    return tl::expected<Terrain, VulkanError>{
        tl::in_place,
        PrivateConstructionToken{},
        std::move(*vertex_buffer),
        std::move(*index_buffer),
        instances,
        heightmap,
        colormap,
        std::move(lod_lvls),
        std::move(noise_gen),
        params,
    };
}

void
B5::Terrain::loop_event(const RenderEvent& re)
{
    UniqueMemoryMapping::map_memory(re.renderer->device(),
                                    _renderstate.instances.second.memory,
                                    _renderstate.instances.second.aligned_chunk_size * re.frame_data->id,
                                    _renderstate.instances.second.aligned_chunk_size)
        .map([this, &re](UniqueMemoryMapping mem_map) {
            TerrainInstanceData* inst = mem_map.as<TerrainInstanceData>();
            inst->colormap = destructure_bindless_resource_handle(_renderstate.colormap.first).first;
            inst->heightmap = destructure_bindless_resource_handle(_renderstate.heightmap.first).first;
            inst->world_view_proj = re.g_ubo_data->world_view_proj; // use identity for terrain model transform
        });

    vkCmdBindPipeline(re.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, re.sres->pipelines.p_terrain.handle());
    const VkDeviceSize vb_offsets[] = { 0 };
    const VkBuffer vertexbuffers[] = { _renderstate.vertexbuffer.buffer_handle() };

    //
    // 0 as instance since there's just 1 terrain slab for now
    const PackedU32PushConstant push_const{ _renderstate.instances.first, 0, re.frame_data->id };

    vkCmdBindVertexBuffers(re.frame_data->cmd_buf, 0, 1, vertexbuffers, vb_offsets);
    vkCmdBindIndexBuffer(re.frame_data->cmd_buf, _renderstate.indexbuffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(re.frame_data->cmd_buf,
                       re.sres->pipelines.p_terrain.layout(),
                       VK_SHADER_STAGE_ALL,
                       0,
                       push_const.size(),
                       push_const.as_bytes().data());

    const TerrainLodLevel& lod_lvl = _renderstate.lod_levels[_uistate.lod_level];
    vkCmdDrawIndexed(re.frame_data->cmd_buf,
                     lod_lvl.index_count,
                     1,
                     lod_lvl.offset_index,
                     static_cast<int32_t>(lod_lvl.offset_vertex),
                     0);
}

void
B5::Terrain::user_interface(xray::ui::user_interface* ui, const RenderEvent& re)
{
    if (ImGui::CollapsingHeader("Terrain")) {
        const TerrainLodLevel& current_lod = _renderstate.lod_levels[_uistate.lod_level];

        ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f },
                           "Size: (%u x %u)\nLod level: %u (vertex count: %u, index count: %u)",
                           _terrain_params.size,
                           _terrain_params.size,
                           _uistate.lod_level,
                           current_lod.vertex_count,
                           current_lod.index_count);

        auto clamped_rangle_slider_fn =
            [](const uint32_t value, const uint32_t min, const uint32_t max, const char* txt) {
                int32_t int_val = static_cast<int32_t>(value);
                const bool result =
                    ImGui::DragInt(txt, &int_val, 1.0f, static_cast<int32_t>(min), static_cast<int32_t>(max));
                return pair{ result, static_cast<uint32_t>(int_val) };
            };

        if (const auto [changed, new_lod_lvl] = clamped_rangle_slider_fn(
                _uistate.lod_level, 0, static_cast<uint32_t>(_renderstate.lod_levels.size() - 1), "LOD:");
            changed) {
            _uistate.lod_level = new_lod_lvl;
        }

        // struct TerrainParams
        // {
        //     uint32_t width{ 512 };
        //     uint32_t height{ 512 };
        //     float scale{ 2.0f };
        //     float bias{ 1.0f };
        //     uint32_t octaves{ 6 };
        //     float bounds_x[2]{ -1.0f, 1.0f };
        //     float bounds_z[2]{ -1.0f, 1.0f };
        // };
    }
}
