#include "terrain.hpp"

#include <utility>
#include <imgui/imgui.h>
#include <imgui/IconsFontAwesome.h>

#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/memory.arena.unique.ptr.hpp"
#include "xray/math/scalar3.hpp"
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
                     xray::rendering::VulkanBuffer vertexbuffer,
                     xray::rendering::VulkanBuffer indexbuffer,
                     xray::rendering::BindlessStorageBufferResourceHandleEntryPair instances,
                     xray::rendering::BindlessImageResourceHandleEntryPair hmap,
                     xray::rendering::BindlessImageResourceHandleEntryPair cmap,
                     uint32_t index_count,
                     xray::base::unique_pointer<NoiseGen> noise_gen,
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
            .index_count = index_count,
        },
    }
{
}

void
make_terrain_grid(const uint32_t width,
                  const uint32_t height,
                  containers::vector<TerrainVertex>& vertices,
                  containers::vector<uint32_t>& indices)
{
    const size_t vertex_count = width * height;
    const size_t faces = (width - 1) * (height - 1) * 2;
    const size_t index_count = faces * 3;

    const float hx = static_cast<float>(width - 1) * 0.5f;
    const float hz = static_cast<float>(height - 1) * 0.5f;
    const float du = 1.0f / (static_cast<float>(width));
    const float dz = 1.0f / (static_cast<float>(height));

    vertices.resize(vertex_count);

    for (size_t z = 0; z < height; ++z) {
        for (size_t x = 0; x < width; ++x) {
            vertices[z * width + x].pos = vec3f{ static_cast<float>(x) - hx, .0f, static_cast<float>(z) - hz };
            vertices[z * width + x].uv = vec2f{ static_cast<float>(x) * du, 1.0f - static_cast<float>(z) * dz };
            // vertices[z * width + x].xy = { x, z };
        }
    }

    indices.resize(index_count, uint32_t{});
    uint32_t* idx = indices.data();
    for (size_t z = 0; z < height - 1; ++z) {
        for (size_t x = 0; x < width - 1; ++x) {

            *idx++ = static_cast<uint32_t>(z * width + x);
            *idx++ = static_cast<uint32_t>(z * width + x + 1);
            *idx++ = static_cast<uint32_t>((z + 1) * width + x);

            *idx++ = static_cast<uint32_t>(z * width + x + 1);
            *idx++ = static_cast<uint32_t>((z + 1) * width + x + 1);
            *idx++ = static_cast<uint32_t>((z + 1) * width + x);
        }
    }
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
    noise_gen.heightmap_builder.SetDestSize(params.width, params.height);
    noise_gen.heightmap_builder.SetBounds(params.xmin, params.xmax, params.zmin, params.zmax);
    noise_gen.heightmap_builder.EnableSeamless();
    noise_gen.heightmap_builder.Build();

    heightmap.resize(params.width * params.height);
    for (size_t z = 0; z < params.height; ++z) {
        for (size_t x = 0; x < params.width; ++x) {
            heightmap[z * params.width + x] = noise_gen.heightmap.GetValue(x, z);
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

    colormap.resize(params.width * params.height);
    for (size_t z = 0; z < params.height; ++z) {
        for (size_t x = 0; x < params.width; ++x) {
            const auto color = height_map_image.GetValue(x, z);
            colormap[z * params.width + x] = vec4ui8{ color.red, color.green, color.blue, color.alpha };
        }
    }
}

tl::expected<B5::Terrain, VulkanError>
B5::Terrain::create(const InitContext& ctx)
{
    TerrainParams params{ ctx.scene_def->terrain_params };

    containers::vector<TerrainVertex> vertices{ *ctx.temp };
    containers::vector<uint32_t> indices{ *ctx.temp };

    make_terrain_grid(params.width, params.height, vertices, indices);

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
                                                          .width = params.width,
                                                          .height = params.height,
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
                                                         .width = params.width,
                                                         .height = params.height,
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

    return tl::expected<Terrain, VulkanError>{
        tl::in_place,
        PrivateConstructionToken{},
        std::move(*vertex_buffer),
        std::move(*index_buffer),
        instances,
        heightmap,
        colormap,
        static_cast<uint32_t>(indices.size()),
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
    vkCmdDrawIndexed(re.frame_data->cmd_buf, _renderstate.index_count, 1, 0, 0, 0);
}

void
B5::Terrain::user_interface(xray::ui::user_interface* ui, const RenderEvent& re)
{
    if (ImGui::CollapsingHeader("Terrain")) {
        ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f },
                           "Size: (%u x %u), indices %u",
                           _terrain_params.width,
                           _terrain_params.height,
                           _renderstate.index_count);

        auto clamped_rangle_slider_fn =
            [](const uint32_t value, const uint32_t min, const uint32_t max, const char* txt) {
                int32_t int_val = static_cast<int32_t>(value);
                const bool result =
                    ImGui::DragInt(txt, &int_val, 1.0f, static_cast<int32_t>(min), static_cast<int32_t>(max));
                return result;
            };

        if (clamped_rangle_slider_fn(_terrain_params.width, 16, 1024, "width:")) {
        }

        if (clamped_rangle_slider_fn(_terrain_params.height, 16, 1024, "height:")) {
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
