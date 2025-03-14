#include "xray/rendering/sprite.system/sprite.system.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/serialization/rfl.libconfig/config.load.hpp"
#include "xray/rendering/sprite.system/sprite.serialization.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/serialization/parser.scalar2.hpp"
#include "xray/base/app_config.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.packed.pushconst.hpp"
#include "xray/rendering/sprite.system/sprite.defs.hpp"
#include "xray/scene/scene.definition.hpp"
#include "xray/rendering/colors/color_palettes.hpp"

namespace xray::rendering {

struct TextureAtlasData
{
    // std::unordered_map<uint64_t, TextureRegion> frames;
    // TODO: fix bug in libconfig serialize with unordered_map
    std::vector<SpriteAtlasEntry> frames;
    std::filesystem::path texture_file;
};

SpriteSystem::SpriteSystem(SpriteSystem::PrivateConstructionToken,
                           VulkanBuffer&& vertex_buffer,
                           VulkanBuffer&& index_buffer,
                           BindlessImageResourceHandleEntryPair atlas_res,
                           std::unordered_map<SpriteHandleType, SpriteAtlasEntry> frames,
                           SpriteVertex* sprite_vtx_ptr,
                           uint16_t* sprite_idx_ptr)
    : _vertex_buffer{ std::move(vertex_buffer) }
    , _index_buffer{ std::move(index_buffer) }
    , _atlas{ atlas_res }
    , _sprites_table{ std::move(frames) }
    , _mapped_vb{ sprite_vtx_ptr }
    , _mapped_ib{ sprite_idx_ptr }
{
}

tl::expected<SpriteSystem, SpriteAtlasCreationError>
SpriteSystem::from_file(const std::filesystem::path& path,
                        const xray::base::ConfigSystem& cfg_sys,
                        VulkanRenderer& renderer)
{
    auto vertex_buffer = VulkanBuffer::create(
        renderer,
        VulkanBufferCreateInfo{
            .name_tag = "sprite.vb",
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .bytes = sizeof(SpriteVertex) * MAX_SPRITES,
            .frames = renderer.max_inflight_frames(),
        });
    XR_VK_PROPAGATE_ERROR(vertex_buffer);

    auto index_buffer = VulkanBuffer::create(
        renderer,
        VulkanBufferCreateInfo{
            .name_tag = "sprite.ib",
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            .bytes = sizeof(uint16_t) * MAX_INDICES,
            .frames = renderer.max_inflight_frames(),
        });

    XR_VK_PROPAGATE_ERROR(index_buffer);

    rfl::Result<TextureAtlasData> atlas_data = rfl::libconfig::read<TextureAtlasData>(path);
    if (!atlas_data) {
        return tl::make_unexpected(SpriteAtlasError{ .what = atlas_data.error().what() });
    }

    auto job = renderer.create_job(QueueType::Transfer);
    XR_VK_PROPAGATE_ERROR(job);

    auto atlas_image = VulkanImage::from_file(renderer,
                                              VulkanImageLoadInfo{
                                                  .tag_name = "sprite.atlas",
                                                  .cmd_buf = job->buffer,
                                                  .path = cfg_sys.texture_path(atlas_data->texture_file),
                                                  .usage_flags = VK_IMAGE_USAGE_SAMPLED_BIT,
                                                  .final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                  .tiling = VK_IMAGE_TILING_OPTIMAL,
                                              });
    XR_VK_PROPAGATE_ERROR(atlas_image);
    [[maybe_unused]] auto wait_token = renderer.submit_job(std::move(*job));

    //
    // once the libconfig bug is fixed this should not be needed anymore
    std::unordered_map<SpriteHandleType, SpriteAtlasEntry> sprites;
    for (const SpriteAtlasEntry& atlas_entry : atlas_data->frames) {
        [[maybe_unused]] const auto [itr, was_inserted] = sprites.emplace(atlas_entry.hashed_name, atlas_entry);
    }

    auto bindless_atlas_image = renderer.bindless_sys().add_image(std::move(*atlas_image), nullptr, tl::nullopt);
    renderer.queue_image_ownership_transfer(bindless_atlas_image.first);

    auto map_buffer_fn = [device = renderer.device()](const VulkanBuffer& vb) noexcept {
        void* mapped_addr{};
        const VkResult map_result =
            WRAP_VULKAN_FUNC(vkMapMemory, device, vb.memory_handle(), 0, VK_WHOLE_SIZE, 0, &mapped_addr);
        return std::pair{ mapped_addr, map_result };
    };

    auto [map_addr_vb, map_res_vb] = map_buffer_fn(*vertex_buffer);
    if (map_res_vb != VK_SUCCESS) {
        return XR_MAKE_VULKAN_ERROR(map_res_vb);
    }

    auto [map_addr_ib, map_res_ib] = map_buffer_fn(*index_buffer);
    if (map_res_ib != VK_SUCCESS) {
        return XR_MAKE_VULKAN_ERROR(map_res_ib);
    }

    return tl::expected<SpriteSystem, SpriteAtlasCreationError>{
        tl::in_place,
        PrivateConstructionToken{},
        std::move(*vertex_buffer),
        std::move(*index_buffer),
        bindless_atlas_image,
        std::move(sprites),
        static_cast<SpriteVertex*>(map_addr_vb),
        static_cast<uint16_t*>(map_addr_ib),
    };
}

tl::optional<SpriteAtlasEntry>
SpriteSystem::get_sprite_by_name(const std::string_view sprite) const noexcept
{
    const uint64_t hashed_name = FNV::fnv1a(sprite.data());
    return get_sprite_by_id(SpriteHandleType{ hashed_name });
}

tl::optional<SpriteAtlasEntry>
SpriteSystem::get_sprite_by_id(const SpriteHandleType id) const noexcept
{
    auto itr_entry = _sprites_table.find(id);
    return itr_entry == std::cend(_sprites_table) ? tl::nullopt : tl::optional{ itr_entry->second };
}

void
SpriteSystem::draw(const float x,
                   const float y,
                   const float width,
                   const float height,
                   const SpriteHandleType hashed_name,
                   const uint32_t color)
{
    const auto& itr_entry = _sprites_table.find(hashed_name);
    if (itr_entry == std::cend(_sprites_table)) {
        return;
    }

    using namespace xray::math;

    const SpriteVertex sprite_vertices[] = {
        SpriteVertex{
            .pos = vec2f32{ x, y + height },
            .uv = itr_entry->second.bottom_left,
            .texid = itr_entry->second.layer,
            .color = color,
        },

        SpriteVertex{
            .pos = vec2f32{ x + width, y + height },
            .uv = itr_entry->second.bottom_right,
            .texid = itr_entry->second.layer,
            .color = color,
        },

        SpriteVertex{
            .pos = vec2f32{ x + width, y },
            .uv = itr_entry->second.top_right,
            .texid = itr_entry->second.layer,
            .color = color,
        },

        SpriteVertex{
            .pos = vec2f32{ x, y },
            .uv = itr_entry->second.top_left,
            .texid = itr_entry->second.layer,
            .color = color,
        },
    };

    add_sprite_quad(sprite_vertices);
}

void
SpriteSystem::draw_scaled_rotated_with_origin(const float ox,
                                              const float oy,
                                              const float width,
                                              const float height,
                                              const float scale,
                                              const float rotation,
                                              const SpriteHandleType sprite_handle,
                                              const uint32_t color)
{
    const auto& itr_entry = _sprites_table.find(sprite_handle);
    if (itr_entry == std::cend(_sprites_table)) {
        return;
    }

    using namespace xray::math;

    const vec2f32 t = vec2f32{ ox, oy };
    const float hw = width * 0.5f;
    const float hh = height * 0.5f;

    //
    // generate vertices from origin @ (0, 0)
    vec2f32 v0 = vec2f32{ -hw, hh };
    vec2f32 v1 = vec2f32{ hw, hh };
    vec2f32 v2 = vec2f32{ hw, -hh };
    vec2f32 v3 = vec2f32{ -hw, -hh };

    //
    // scale
    v0 = v0 * scale;
    v1 = v1 * scale;
    v2 = v2 * scale;
    v3 = v3 * scale;

    //
    // rotate
    const float sin_theta = std::sin(rotation);
    const float cos_theta = std::cos(rotation);

    v0 = vec2f32{ v0.x * cos_theta - v0.y * sin_theta, v0.x * sin_theta + v0.y * cos_theta };
    v1 = vec2f32{ v1.x * cos_theta - v1.y * sin_theta, v1.x * sin_theta + v1.y * cos_theta };
    v2 = vec2f32{ v2.x * cos_theta - v2.y * sin_theta, v2.x * sin_theta + v2.y * cos_theta };
    v3 = vec2f32{ v3.x * cos_theta - v3.y * sin_theta, v3.x * sin_theta + v3.y * cos_theta };

    //
    // translate back to origin
    v0 = v0 + t;
    v1 = v1 + t;
    v2 = v2 + t;
    v3 = v3 + t;

    const SpriteVertex sprite_vertices[] = {
        SpriteVertex{
            .pos = v0,
            .uv = itr_entry->second.bottom_left,
            .texid = itr_entry->second.layer,
            .color = color,
        },

        SpriteVertex{
            .pos = v1,
            .uv = itr_entry->second.bottom_right,
            .texid = itr_entry->second.layer,
            .color = color,
        },

        SpriteVertex{
            .pos = v2,
            .uv = itr_entry->second.top_right,
            .texid = itr_entry->second.layer,
            .color = color,
        },

        SpriteVertex{
            .pos = v3,
            .uv = itr_entry->second.top_left,
            .texid = itr_entry->second.layer,
            .color = color,
        },
    };

    add_sprite_quad(sprite_vertices);
}

void
SpriteSystem::render(const SpriteSystemRenderContext& rctx)
{
    if (_cursor.index != 0) {
        [[maybe_unused]] const auto marker = rctx.renderer->dbg_marker_begin(
            rctx.frame_data->cmd_buf, "Draw sprites", xray::rendering::color_palette::flat::orange100);

        vkCmdBindPipeline(
            rctx.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, rctx.sres->pipelines.p_sprites.handle());

        const VkBuffer vertex_buffers[] = { _vertex_buffer.buffer_handle() };
        const VkDeviceSize vertex_offsets[] = { 0 };

        vkCmdBindVertexBuffers(rctx.frame_data->cmd_buf, 0, 1, vertex_buffers, vertex_offsets);
        vkCmdBindIndexBuffer(rctx.frame_data->cmd_buf, _index_buffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT16);

        const xray::rendering::PackedU32PushConstant push_const{ _atlas.first, 0, rctx.frame_data->id };
        vkCmdPushConstants(rctx.frame_data->cmd_buf,
                           rctx.sres->pipelines.p_sprites.layout(),
                           VK_SHADER_STAGE_ALL,
                           0,
                           push_const.size(),
                           push_const.as_bytes().data());
        vkCmdDrawIndexed(rctx.frame_data->cmd_buf, static_cast<uint32_t>(_cursor.index), 1, 0, 0, 0);
    }

    const uint32_t max_frames = rctx.renderer->max_inflight_frames();
    _cursor.frame = static_cast<uint8_t>((rctx.frame_data->id + 1) % max_frames);
    _cursor.vertex = _cursor.index = 0;
}

void
SpriteSystem::add_sprite_quad(const std::span<const SpriteVertex> quad)
{
    assert(quad.size() == 4);

    memcpy(vertex_ptr(), quad.data(), quad.size_bytes());

    const uint16_t indices[] = {
        //
        // 1st quad
        static_cast<uint16_t>(_cursor.vertex + 0),
        static_cast<uint16_t>(_cursor.vertex + 1),
        static_cast<uint16_t>(_cursor.vertex + 2),

        //
        // 2nd quad
        static_cast<uint16_t>(_cursor.vertex + 0),
        static_cast<uint16_t>(_cursor.vertex + 2),
        static_cast<uint16_t>(_cursor.vertex + 3),
    };

    memcpy(index_ptr(), indices, sizeof(indices));

    _cursor.vertex += 4;
    _cursor.index += 6;
}
}
