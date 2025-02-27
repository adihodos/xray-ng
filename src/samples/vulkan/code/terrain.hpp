#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <bitset>

#include <tl/expected.hpp>
#include <noise/noise.h>
#include <noise/noiseutils.h>

#include "xray/base/memory.arena.hpp"
#include "xray/base/containers/arena.vector.hpp"
// #include "xray/base/containers/arena.unorderered_map.hpp"
#include "xray/base/unique_pointer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/geometry/procedural.terrain.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar2.hash.hpp"

namespace xray::ui {
class user_interface;
};

namespace B5 {

struct RenderEvent;
struct InitContext;

struct TerrainLodLevel
{
    uint32_t offset_vertex;
    uint32_t offset_index;
    uint32_t vertex_count;
    uint32_t index_count;
};

struct NoiseGen
{
    noise::module::RidgedMulti ridged;
    noise::module::Billow base_flat_terrain;
    noise::module::ScaleBias flat_terrain;
    noise::module::Perlin terrain_type;
    noise::module::Select terrain_selector;
    noise::module::Turbulence final_terrain;
    noise::utils::NoiseMap heightmap;
    noise::utils::NoiseMapBuilderPlane heightmap_builder;
};

class Terrain
{
  public:
    static tl::expected<Terrain, xray::rendering::VulkanError> create(const InitContext& ctx);

    struct SlabRenderResources
    {
        xray::rendering::BindlessImageResourceHandleEntryPair heightmap;
        xray::rendering::BindlessImageResourceHandleEntryPair colormap;
    };

    using SlabResourceTable = std::unordered_map<xray::math::vec2i32, SlabRenderResources>;

  private:
    struct PrivateConstructionToken
    {
        explicit PrivateConstructionToken() = default;
    };

    xray::base::unique_pointer<NoiseGen> _noise_gen;
    xray::rendering::TerrainParams _terrain_params;

    enum DrawOptions
    {
        TerrainWireframeBit = 0,
    };

    struct UIState
    {
        uint32_t lod_level{};
        xray::math::vec2f32 cam_pos_xz_plane{};
        std::bitset<16> draw_opts{};
        std::vector<xray::math::vec2i32> visible_slabs;
        std::vector<xray::math::vec2i32> spawned_slabs;
    } _uistate;

    struct RenderResources
    {
        xray::rendering::VulkanBuffer vertexbuffer;
        xray::rendering::VulkanBuffer indexbuffer;
        xray::rendering::BindlessStorageBufferResourceHandleEntryPair instances;
        xray::base::containers::vector<TerrainLodLevel> lod_levels;
        SlabResourceTable slabs_table;
        std::unordered_set<xray::math::vec2i32> slabs_visible_last_frame;
        std::vector<SlabRenderResources> slabs_freelist;
        xray::math::vec2f32 last_cam_pos;
        xray::math::vec2f32 last_cam_dir;
        float max_view_distance_squared;
    } _renderstate;

  public:
    Terrain(PrivateConstructionToken,
            xray::rendering::VulkanBuffer&& vertexbuffer,
            xray::rendering::VulkanBuffer&& indexbuffer,
            xray::rendering::BindlessStorageBufferResourceHandleEntryPair instances,
            xray::base::containers::vector<TerrainLodLevel>&& lod_levels,
            SlabResourceTable&& chunks,
            xray::base::unique_pointer<NoiseGen>&& noise_gen,
            xray::rendering::TerrainParams terrain_params);

    Terrain(Terrain&&) = default;

    void loop_event(const RenderEvent& re);
    void user_interface(xray::ui::user_interface* ui, const RenderEvent& re);
};

}
