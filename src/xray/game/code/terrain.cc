#include "terrain.hpp"

#include <utility>
#include <random>

#include <imgui/imgui.h>
#include <imgui/IconsFontAwesome.h>
#include <concurrencpp/concurrencpp.h>
#include <stb/stb_image_write.h>

#include <Lz/algorithm/accumulate.hpp>
#include <Lz/algorithm/transform.hpp>
#include <Lz/chunks.hpp>
#include <Lz/exclusive_scan.hpp>
#include <Lz/filter.hpp>
#include <Lz/map.hpp>
#include <Lz/procs/to.hpp>
#include <Lz/range.hpp>
#include <Lz/zip.hpp>
#include <Lz/take.hpp>

#include "xray/xray.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/containers/arena.vector.hpp"
#include "xray/base/containers/arena.unordered_set.hpp"
#include "xray/base/memory.arena.unique.ptr.hpp"

#include "xray/base/xray.fmt.hpp"
#include "xray/math/scalar3.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/math/scalar2_math.hpp"
#include "xray/math/scalar2_string_cast.hpp"
#include "xray/math/axis.aligned.bounding.box.r2.hpp"
#include "xray/math/quadtree.hpp"
#include "xray/math/scalar4x4.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r4.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.renderer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.image.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.dynamic.dispatch.hpp"
#include "xray/scene/scene.definition.hpp"
#include "xray/scene/camera.hpp"
#include "events.hpp"
#include "push.constant.packer.hpp"
#include "bindless.pipeline.config.hpp"
#include "system.memory.hpp"

XR_DISABLE_OPTIMIZATIONS()

using namespace std;
using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;

constexpr const vec2i32 TERRAIN_CHUNK_OFFSETS[] = {
	{-1, -1},
	{0, -1},
	{1, -1},
	{-1, 0},
	{0, 0},
	{1, 0},
	{-1, 1},
	{0, 1},
	{1, 1},
};

constexpr const float WORLD_SIZE	  = 65536.0f;
constexpr const float WORLD_HALF_SIZE = 32768.0f;
constexpr const BBoxAA2DF32 WORLD_BOUNDS{vec2f32{-WORLD_HALF_SIZE}, vec2f32{WORLD_HALF_SIZE}};
constexpr const uint32_t MAX_VISIBLE_SLABS = 32;

struct SplitByVisibleBoundingRegion {
	BBoxAA2DF32 visible_region;
	float epsilon;

	SplitByVisibleBoundingRegion(const vec2f32 origin, const float extent, const float eps) noexcept
		: visible_region{OriginWithExtentsTag{}, origin, vec2f32{extent}}, epsilon{eps} {}

	bool operator()(const QuadTreeF32::tree_node_type& node) const noexcept {
		return (node.bbox ^ visible_region)
			.map([e = epsilon](const BBoxAA2DF32& box) { return box.width() > e && box.height() > e; })
			.value_or(false);
	}
};

bool ray_aabb_intersect(
	const vec2f32 ray_org, const vec2f32 ray_dir, const vec2f32 box_min, const vec2f32 box_max
) noexcept {
	float t_min = std::numeric_limits<float>::min();
	float t_max = std::numeric_limits<float>::max();

	const float tx0			= (box_min.x - ray_org.x) / ray_dir.x;
	const float tx1			= (box_max.x - ray_org.x) / ray_dir.x;
	const auto [xmin, xmax] = std::minmax({tx0, tx1});

	t_min = std::min(t_min, xmin);
	t_max = std::max(t_max, xmax);

	const float ty0			= (box_min.y - ray_org.y) / ray_dir.y;
	const float ty1			= (box_max.y - ray_org.y) / ray_dir.y;
	const auto [ymin, ymax] = std::minmax({ty0, ty1});

	t_min = std::min(t_min, ymin);
	t_max = std::max(t_max, ymax);

	return t_min <= t_max && t_max >= 0;
}

struct TerrainVertex {
	vec3f pos;
	vec2f uv;

	static constexpr TerrainVertex identity() noexcept {
		return TerrainVertex{
			.pos = vec3f::stdc::zero,
			.uv	 = vec2f::stdc::zero,
		};
	}
};

struct TerrainGenTaskParams {
	TerrainParams terrain;
	VulkanRenderer* renderer;
	vec2i32 center;
};

struct TerrainSlabTextures {
	VulkanImage colormap;
	VulkanImage heightmap;
	// VulkanImage normalmap;
};

struct TerrainGenTaskResult {
	vector<vec2i32> chunks;
	vector<TerrainSlabTextures> textures;
};

B5::Terrain::Terrain(
	PrivateConstructionToken,
	xray::rendering::VulkanBuffer&& vertexbuffer,
	xray::rendering::VulkanBuffer&& indexbuffer,
	xray::rendering::BindlessStorageBufferResourceHandleEntryPair instances,
	xray::base::containers::vector<TerrainLodLevel>&& lod_levels,
	SlabResourceTable&& chunks,
	xray::base::unique_pointer<NoiseGen>&& noise_gen,
	TerrainParams terrain_params
)
	: _noise_gen{std::move(noise_gen)},
	  _terrain_params{terrain_params},
	  _renderstate{
		  RenderResources{
			  .vertexbuffer				 = std::move(vertexbuffer),
			  .indexbuffer				 = std::move(indexbuffer),
			  .instances				 = instances,
			  .lod_levels				 = std::move(lod_levels),
			  .slabs_table				 = std::move(chunks),
			  .max_view_distance_squared = static_cast<float>(
				  (terrain_params.cells_view_dist * terrain_params.size) *
				  (terrain_params.cells_view_dist * terrain_params.size)
			  ),
		  },
	  } {
	lz::transform(
		_renderstate.slabs_table,
		std::inserter(_renderstate.slabs_visible_last_frame, _renderstate.slabs_visible_last_frame.begin()),
		[](const auto& slab) { return slab.first; }
	);
}

struct TerrainDetails {
	uint32_t lod_factor;
	uint32_t points;
	uint32_t vertices;
	uint32_t indices;

	TerrainDetails& operator+=(const TerrainDetails& rhs) noexcept {
		lod_factor += rhs.lod_factor;
		points += rhs.points;
		vertices += rhs.vertices;
		indices += rhs.indices;

		return *this;
	}
};

inline TerrainDetails operator+(const TerrainDetails& a, const TerrainDetails& b) noexcept {
	TerrainDetails r{a};
	r += b;
	return r;
}

TerrainDetails compute_terrain_details_lod(const TerrainParams& params, const uint32_t lod) noexcept {
	assert(is_power_of_two(params.size));
	assert(lod <= 8);

	const uint32_t lod_factor = lod ? 2 << (lod - 1) : 1;

	const uint32_t points_count = params.size / lod_factor + 1;
	const uint32_t vertex_count = points_count * points_count;
	const uint32_t faces		= (points_count - 1) * (points_count - 1) * 2;
	const uint32_t index_count	= faces * 3;

	return TerrainDetails{
		.lod_factor = lod_factor,
		.points		= points_count,
		.vertices	= vertex_count,
		.indices	= index_count,
	};
}

vec2ui32 make_terrain_grid(
	const TerrainParams& params,
	const uint32_t lod,
	std::span<TerrainVertex> buffer_vertex,
	std::span<uint32_t> buffer_index
) {
	assert(is_power_of_two(params.size));
	assert(lod <= 8);

	const auto [lod_factor, points_count, vertex_count, index_count] = compute_terrain_details_lod(params, lod);

	const float hx = static_cast<float>(params.size) * .5f;
	const float hz = static_cast<float>(params.size) * .5f;
	const float du = 1.0f / (static_cast<float>(points_count - 1));
	const float dz = 1.0f / (static_cast<float>(points_count - 1));

	assert(buffer_vertex.size() == vertex_count);
	assert(buffer_index.size() == index_count);

	for (size_t z = 0; z < points_count; ++z) {
		for (size_t x = 0; x < points_count; ++x) {
			buffer_vertex[z * points_count + x].pos =
				vec3f{static_cast<float>(x * lod_factor) - hx, .0f, static_cast<float>(z * lod_factor) - hz};
			buffer_vertex[z * points_count + x].uv =
				vec2f{static_cast<float>(x) * du, 1.0f - static_cast<float>(z) * dz};
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

	return vec2ui32{vertex_count, index_count};
}

void B5::make_terrain_heightmap_colormap(
	const xray::rendering::TerrainParams& params,
	const xray::math::BBoxAA2DF32& bounds,
	std::span<float> heightmap,
	std::span<vec4ui8> colormap
) {
	XR_LOG_INFO("[[terrain]] slab {}x{}, {}x{}", bounds.min.x, bounds.max.x, bounds.min.y, bounds.max.y);

	const uint32_t slab_size = params.size;
	std::mt19937 rand_eng{params.seed};

	B5::NoiseGen noise_gen{};
	noise_gen.ridged.SetSeed(rand_eng());
	noise_gen.ridged.SetNoiseQuality(noise::NoiseQuality::QUALITY_BEST);

	noise_gen.base_flat_terrain.SetSeed(rand_eng());
	noise_gen.base_flat_terrain.SetNoiseQuality(noise::NoiseQuality::QUALITY_BEST);
	noise_gen.base_flat_terrain.SetFrequency(2.0);
	noise::module::ScaleBias flat_terrain;
	flat_terrain.SetSourceModule(0, noise_gen.base_flat_terrain);
	flat_terrain.SetScale(params.scale);
	flat_terrain.SetBias(params.bias);

	noise_gen.terrain_type.SetSeed(rand_eng());
	noise_gen.terrain_type.SetNoiseQuality(noise::NoiseQuality::QUALITY_BEST);
	noise_gen.terrain_type.SetOctaveCount(params.octaves);
	noise_gen.terrain_type.SetFrequency(0.5);
	noise_gen.terrain_type.SetPersistence(0.25);

	noise_gen.terrain_selector.SetSourceModule(0, flat_terrain);
	noise_gen.terrain_selector.SetSourceModule(1, noise_gen.ridged);
	noise_gen.terrain_selector.SetControlModule(noise_gen.terrain_type);
	noise_gen.terrain_selector.SetBounds(0.0f, 1000.0f);
	noise_gen.terrain_selector.SetEdgeFalloff(0.125);

	noise_gen.final_terrain.SetSeed(rand_eng());
	noise_gen.final_terrain.SetSourceModule(0, noise_gen.terrain_selector);
	noise_gen.final_terrain.SetFrequency(4.0);
	noise_gen.final_terrain.SetPower(0.125);

	noise_gen.rly_final_terrain.SetSourceModule(0, noise_gen.final_terrain);

	noise_gen.heightmap_builder.SetDestNoiseMap(noise_gen.heightmap);
	noise_gen.heightmap_builder.SetSourceModule(noise_gen.rly_final_terrain);
	noise_gen.heightmap_builder.SetDestSize(slab_size, slab_size);
	const float terrain_scale = static_cast<float>(slab_size) / WORLD_SIZE;
	noise_gen.heightmap_builder.SetBounds(
		bounds.min.x * terrain_scale,
		bounds.max.x * terrain_scale,
		bounds.min.y * terrain_scale,
		bounds.max.y * terrain_scale
	);
	// noise_gen.heightmap_builder.EnableSeamless();
	noise_gen.heightmap_builder.Build();
	assert(heightmap.size() == params.size * params.size);

	//
	// libnoise image pixels are laid in memory bottom to top while Vulkan textures
	// are top to bottom
	for (size_t z = 0; z < slab_size; ++z) {
		const float* slab_ptr = noise_gen.heightmap.GetConstSlabPtr(static_cast<int32_t>(slab_size - z - 1));
		memcpy(heightmap.subspan(z * slab_size).data(), slab_ptr, slab_size * sizeof(float));
	}

	utils::Image height_map_image;
	utils::Image normal_map_image;
	utils::RendererImage renderer_img;
	renderer_img.SetSourceNoiseMap(noise_gen.heightmap);
	renderer_img.SetDestImage(height_map_image);
	// renderer_img.EnableWrap();
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

	renderer_img.Render();

	assert(colormap.size() == params.size * params.size);
	//
	// libnoise data is laid out bottom to top in memory
	for (size_t z = 0; z < slab_size; ++z) {
		const utils::Color* slab_ptr = height_map_image.GetConstSlabPtr(static_cast<int32_t>(slab_size - z - 1));
		memcpy(colormap.subspan(z * slab_size).data(), slab_ptr, slab_size * sizeof(*slab_ptr));
	}
}

tl::expected<TerrainSlabTextures, VulkanError> create_terrain_slab_render_resources(
	const TerrainParams terrain_params,
	const vec2i32 coords,
	VulkanRenderer* renderer,
	std::span<float> heightmap,
	std::span<vec4ui8> colormap
) {
	auto terrain_images_job = renderer->create_job(QueueType::Transfer);
	XR_VK_PROPAGATE_ERROR(terrain_images_job);

	char scratch_buffer[256];
	xray::base::format_to_n(scratch_buffer, "heightmap: {},{}", coords.x, coords.y);
	auto heightmap_texture = VulkanImage::from_memory(
		*renderer,
		VulkanImageCreateInfo{
			.tag_name	  = scratch_buffer,
			.wpkg		  = terrain_images_job->buffer,
			.type		  = VK_IMAGE_TYPE_2D,
			.usage_flags  = VK_IMAGE_USAGE_SAMPLED_BIT,
			.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.format		  = VK_FORMAT_R32_SFLOAT,
			.width		  = terrain_params.size,
			.height		  = terrain_params.size,
			.layers		  = 1,
			.pixels		  = {to_bytes_span(heightmap)},
		}
	);
	XR_VK_PROPAGATE_ERROR(heightmap_texture);

	format_to_n(scratch_buffer, "colormap: {},{}", coords.x, coords.y);
	auto colormap_texture = VulkanImage::from_memory(
		*renderer,
		VulkanImageCreateInfo{
			.tag_name	  = scratch_buffer,
			.wpkg		  = terrain_images_job->buffer,
			.type		  = VK_IMAGE_TYPE_2D,
			.usage_flags  = VK_IMAGE_USAGE_SAMPLED_BIT,
			.memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.format		  = VK_FORMAT_R8G8B8A8_UNORM,
			.width		  = terrain_params.size,
			.height		  = terrain_params.size,
			.layers		  = 1,
			.pixels		  = {to_bytes_span(colormap)},
		}
	);
	XR_VK_PROPAGATE_ERROR(colormap_texture);

	auto wait_token_images = renderer->submit_job(std::move(*terrain_images_job));
	XR_VK_PROPAGATE_ERROR(wait_token_images);

	format_to_n(
		scratch_buffer,
		"%s/mt.colormap_%d_%d.png",
		xray::base::ConfigSystem::instance()->FileSys.RootPathAbsolute.generic_string().c_str(),
		coords.x,
		coords.y
	);
	stbi_write_png(
		scratch_buffer,
		terrain_params.size,
		terrain_params.size,
		4,
		colormap.data(),
		terrain_params.size * sizeof(vec4ui8)
	);

	return tl::expected<TerrainSlabTextures, VulkanError>{
		tl::in_place,
		std::move(*colormap_texture),
		std::move(*heightmap_texture),
	};
}

concurrencpp::result<tl::expected<TerrainGenTaskResult, VulkanError>> task_generate_terrain_chunk(
	concurrencpp::executor_tag,
	std::shared_ptr<concurrencpp::thread_pool_executor> thread_pool,
	TerrainGenTaskParams params
) {
	auto temp_arena = B5::GlobalMemorySystem::instance()->grab_medium_arena();

	containers::vector<vec2i32> slabs_to_spawn{temp_arena.arena};
	QuadTreeF32{temp_arena.arena, WORLD_BOUNDS, static_cast<float>(params.terrain.size)}.insert(
		vec2f32{0},
		SplitByVisibleBoundingRegion{
			vec2f32{0},
			static_cast<float>(params.terrain.cells_view_dist * params.terrain.size),
			static_cast<float>(params.terrain.size / 2),
		},
		[&slabs_to_spawn](const QuadTreeF32::tree_node_type& node) {
			slabs_to_spawn.push_back(vec2i32{node.bbox.center()});
		}
	);

	containers::vector<concurrencpp::result<tl::expected<TerrainSlabTextures, VulkanError>>> spawned_tasks{
		temp_arena.arena
	};
	spawned_tasks.reserve(slabs_to_spawn.size());

	const float half_size = static_cast<float>(params.terrain.size) * 0.5f;
	for (const vec2i32 slab_center : slabs_to_spawn) {
		const size_t terrain_items = params.terrain.size * params.terrain.size;

		std::span<float> heightmap{temp_arena.arena.alloc_align<float>(terrain_items), terrain_items};
		std::span<vec4ui8> colormap{temp_arena.arena.alloc_align<vec4ui8>(terrain_items), terrain_items};

		const BBoxAA2DF32 bounds{
			vec2f32{slab_center} - vec2f32{half_size},
			vec2f32{slab_center} + vec2f32{half_size},
		};

		spawned_tasks.push_back(thread_pool->submit([=, terrain_params = params.terrain, renderer = params.renderer]() {
			B5::make_terrain_heightmap_colormap(terrain_params, bounds, heightmap, colormap);
			return create_terrain_slab_render_resources(terrain_params, slab_center, renderer, heightmap, colormap);
		}));
	}

	auto tasks_results = co_await concurrencpp::when_all(thread_pool, spawned_tasks.begin(), spawned_tasks.end());
	vector<TerrainSlabTextures> chunk_textures{};

	for (auto&& task_res : tasks_results) {
		auto&& maybe_image = task_res.get();
		if (!maybe_image) {
			co_return tl::make_unexpected(maybe_image.error());
		}
		chunk_textures.emplace_back(std::move(*maybe_image));
	}

	co_return tl::expected<TerrainGenTaskResult, VulkanError>{
		tl::in_place,
		std::vector<vec2i32>{std::cbegin(slabs_to_spawn), std::cend(slabs_to_spawn)},
		std::move(chunk_textures),
	};
}

tl::expected<B5::Terrain, VulkanError> B5::Terrain::create(const InitContext& ctx) {
	TerrainParams params{ctx.scene_def->terrain_params};

	concurrencpp::result<tl::expected<TerrainGenTaskResult, VulkanError>> img_gen_task_res =
		task_generate_terrain_chunk(
			concurrencpp::executor_tag{},
			ctx.co_runtime->thread_pool_executor(),
			TerrainGenTaskParams{
				.terrain  = params,
				.renderer = ctx.renderer,
				.center	  = vec2i32::stdc::zero,
			}
		);

	containers::vector<TerrainDetails> lod_levels =
		lz::range(uint32_t{}, params.lods ? params.lods : 1) |
		lz::map([&params](uint32_t lod) { return compute_terrain_details_lod(params, lod); }) |
		lz::to<containers::vector<TerrainDetails>>(MemoryArenaAllocator<TerrainDetails>{*ctx.temp});

	for (const TerrainDetails& td : lod_levels) {
		XR_LOG_INFO("Lod vtx: {} idx: {}", td.vertices, td.indices);
	}

	const containers::vector<vec2ui32> lod_offsets =
		lod_levels | lz::map([](const TerrainDetails& td) { return vec2ui32{td.vertices, td.indices}; }) |
		lz::exclusive_scan(vec2ui32::stdc::zero, std::plus<vec2ui32>{}) |
		lz::to<containers::vector<vec2ui32>>(MemoryArenaAllocator<vec2ui32>(*ctx.temp));

	const vec2ui32 vertex_index_counts = lz::accumulate(
		lod_levels | lz::map([](const TerrainDetails& td) { return vec2ui32{td.vertices, td.indices}; }),
		vec2ui32::stdc::zero
	);

	XR_LOG_INFO(
		"Terrain generator: lod levels: {}, vertices:{}, indices: {}",
		params.lods,
		vertex_index_counts.x,
		vertex_index_counts.y
	);

	containers::vector<TerrainVertex> vertices{size_t{vertex_index_counts.x}, *ctx.temp};
	containers::vector<uint32_t> indices{size_t{vertex_index_counts.y}, *ctx.temp};

	for (uint32_t lod_level = 0; lod_level < std::max(params.lods, 1u); ++lod_level) {
		make_terrain_grid(
			params,
			lod_level,
			std::span{vertices.data() + lod_offsets[lod_level].x, lod_levels[lod_level].vertices},
			std::span{indices.data() + lod_offsets[lod_level].y, lod_levels[lod_level].indices}
		);
	}

	xray::base::unique_pointer<NoiseGen> noise_gen{xray::base::make_unique<NoiseGen>()};

	auto terrain_buffers_job = ctx.renderer->create_job(QueueType::Transfer);
	XR_VK_PROPAGATE_ERROR(terrain_buffers_job);

	auto vertex_buffer = VulkanBuffer::create(
		*ctx.renderer,
		VulkanBufferCreateInfo{
			.name_tag		   = "Terrain VB",
			.job_cmd_buf	   = terrain_buffers_job->buffer,
			.usage			   = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.bytes			   = container_bytes_size(vertices),
			.initial_data	   = {to_bytes_span(vertices)},
		}
	);
	XR_VK_PROPAGATE_ERROR(vertex_buffer);

	auto index_buffer = VulkanBuffer::create(
		*ctx.renderer,
		VulkanBufferCreateInfo{
			.name_tag		   = "Terrain IB",
			.job_cmd_buf	   = terrain_buffers_job->buffer,
			.usage			   = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			.memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			.bytes			   = container_bytes_size(indices),
			.initial_data	   = {to_bytes_span(indices)},
		}
	);
	XR_VK_PROPAGATE_ERROR(index_buffer);
	auto wait_token_buffers = ctx.renderer->submit_job(std::move(*terrain_buffers_job));
	XR_VK_PROPAGATE_ERROR(wait_token_buffers);

	auto terrain_images_job = ctx.renderer->create_job(QueueType::Transfer);
	XR_VK_PROPAGATE_ERROR(terrain_images_job);

	auto sampler = ctx.renderer->bindless_sys().default_sampler(*ctx.renderer);

	auto terrain_images = img_gen_task_res.get();
	XR_VK_PROPAGATE_ERROR(terrain_images);

	SlabResourceTable terrain_chunks{};
	for (auto&& [coords, images] : lz::zip(terrain_images->chunks, terrain_images->textures)) {
		BindlessImageResourceHandleEntryPair heightmap =
			ctx.renderer->bindless_sys().add_image(std::move(images.heightmap), *sampler, tl::nullopt);
		BindlessImageResourceHandleEntryPair colormap =
			ctx.renderer->bindless_sys().add_image(std::move(images.colormap), *sampler, tl::nullopt);

		ctx.renderer->queue_image_ownership_transfer(heightmap.first);
		ctx.renderer->queue_image_ownership_transfer(colormap.first);

		terrain_chunks.try_emplace(coords, heightmap, colormap);
	}

	auto terrain_instances_buffer = VulkanBuffer::create(
		*ctx.renderer,
		VulkanBufferCreateInfo{
			.name_tag		   = "Terrain Instances",
			.usage			   = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			.bytes			   = sizeof(TerrainInstanceData) * MAX_VISIBLE_SLABS,
			.frames			   = ctx.renderer->max_inflight_frames(),
		}
	);
	XR_VK_PROPAGATE_ERROR(terrain_instances_buffer);

	BindlessStorageBufferResourceHandleEntryPair instances = ctx.renderer->bindless_sys().add_chunked_storage_buffer(
		std::move(*terrain_instances_buffer), ctx.renderer->max_inflight_frames(), tl::nullopt
	);

	containers::vector<TerrainLodLevel> lod_lvls =
		lz::zip(lod_levels, lod_offsets) | lz::map([](tuple<const TerrainDetails&, const vec2ui32&> p) {
			const auto& [lvl, off] = p;
			return TerrainLodLevel{
				.offset_vertex = off.x,
				.offset_index  = off.y,
				.vertex_count  = lvl.vertices,
				.index_count   = lvl.indices,
			};
		}) |
		lz::to<containers::vector<TerrainLodLevel>>(MemoryArenaAllocator<TerrainLodLevel>{*ctx.perm});

	return tl::expected<Terrain, VulkanError>{
		tl::in_place,
		PrivateConstructionToken{},
		std::move(*vertex_buffer),
		std::move(*index_buffer),
		instances,
		std::move(lod_lvls),
		std::move(terrain_chunks),
		std::move(noise_gen),
		params,
	};
}

void copy_render_resources(
	xray::rendering::VulkanRenderer* renderer,
	xray::rendering::QueuedJob& queued_job,
	std::span<float> heightmap,
	std::span<vec4ui8> color_map,
	const B5::Terrain::SlabRenderResources& textures,
	const TerrainParams& params
) {
	const uintptr_t staging_mem =
		renderer->reserve_staging_buffer_memory(heightmap.size_bytes() + color_map.size_bytes());
	uintptr_t staging_buffer_ptr = renderer->staging_buffer_memory();

	const std::tuple<uintptr_t, uintptr_t, uintptr_t> copies[] = {
		{staging_buffer_ptr + staging_mem, heightmap.size_bytes(), reinterpret_cast<uintptr_t>(heightmap.data())},
		{staging_buffer_ptr + staging_mem + heightmap.size_bytes(),
		 color_map.size_bytes(),
		 reinterpret_cast<uintptr_t>(color_map.data())}
	};

	std::array<VkBufferImageCopy, 2> buffer_image_copies{};
	std::array<VkImageMemoryBarrier2, 4> memory_barriers;

	const VkImageSubresourceLayers subresource_layers{
		.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
		.mipLevel		= 0,
		.baseArrayLayer = 0,
		.layerCount		= 1,
	};

	const VkImageSubresourceRange subresource_range{
		.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel	= 0,
		.levelCount		= 1,
		.baseArrayLayer = 0,
		.layerCount		= 1,
	};

	// const auto [queue_idx_graphics, queue_idx_transfer] = renderer->queue_family_indices();

	auto mem_barrier_chunks = lz::chunks(memory_barriers, 2);
	auto texture_list =
		std::initializer_list<VkImage>{textures.heightmap.second.handle, textures.colormap.second.handle};

	for (auto&& [copy_data, buffer_copy, mem_barriers, texture] :
		 lz::zip(copies, buffer_image_copies, mem_barrier_chunks, texture_list)) {
		auto [copy_dst, copy_bytes, copy_src] = copy_data;
		memcpy(reinterpret_cast<void*>(copy_dst), reinterpret_cast<const void*>(copy_src), copy_bytes);

		buffer_copy = VkBufferImageCopy{
			.bufferOffset	   = copy_dst - staging_buffer_ptr,
			.bufferRowLength   = 0,
			.bufferImageHeight = 0,
			.imageSubresource  = subresource_layers,
			.imageOffset	   = {},
			.imageExtent	   = VkExtent3D{params.size, params.size, 1},
		};

		auto&& [color_map, height_map] = textures;

		const VkImageMemoryBarrier2 pre_copy_barrier = {
			.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext				 = nullptr,
			.srcStageMask		 = VK_PIPELINE_STAGE_2_NONE,
			.srcAccessMask		 = VK_ACCESS_2_NONE,
			.dstStageMask		 = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.dstAccessMask		 = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.oldLayout			 = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout			 = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image				 = texture,
			.subresourceRange	 = subresource_range,
		};

		VkDependencyInfo dependency_info{
			.sType					  = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext					  = nullptr,
			.dependencyFlags		  = VK_DEPENDENCY_BY_REGION_BIT,
			.memoryBarrierCount		  = 0,
			.pMemoryBarriers		  = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers	  = nullptr,
			.imageMemoryBarrierCount  = 1,
			.pImageMemoryBarriers	  = &pre_copy_barrier,
		};

		vkCmdPipelineBarrier2(queued_job.buffer, &dependency_info);
		vkCmdCopyBufferToImage(
			queued_job.buffer,
			renderer->staging_buffer(),
			texture,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&buffer_copy
		);

		const VkImageMemoryBarrier2 post_copy_barrier = {
			.sType				 = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
			.pNext				 = nullptr,
			.srcStageMask		 = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.srcAccessMask		 = VK_ACCESS_2_TRANSFER_WRITE_BIT,
			.dstStageMask		 = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			.dstAccessMask		 = VK_ACCESS_2_SHADER_READ_BIT,
			.oldLayout			 = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout			 = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image				 = texture,
			.subresourceRange	 = subresource_range,
		};

		dependency_info.pImageMemoryBarriers = &post_copy_barrier;
		vkCmdPipelineBarrier2(queued_job.buffer, &dependency_info);
	}
}

void B5::Terrain::loop_event(const RenderEvent& re) {
	//
	// pending spawn tasks

	const bool looking_up = are_parallel(re.cam->direction(), vec3f::stdc::unit_y) &&
							(dot(re.cam->direction(), vec3f::stdc::unit_y) > 0.0f);
	if (looking_up) return;

	const float offset = static_cast<float>(_terrain_params.size) * 0.5f;
	const vec2f32 cam_pos_xz_plane{re.g_ubo_data->eye_pos.x, re.g_ubo_data->eye_pos.z};
	_uistate.cam_pos_xz_plane = cam_pos_xz_plane;
	_renderstate.last_cam_pos = cam_pos_xz_plane;

	const auto [view_dir, ray_dir] = [&re]() {
		if (are_parallel(re.cam->direction(), vec3f::stdc::unit_y)) {
			//
			// looking straight down
			return pair{vec2f32{re.cam->up().x, re.cam->up().z}, vec2f32{-re.cam->up().z, re.cam->up().x}};
		}
		return pair{
			vec2f32{re.cam->direction().x, re.cam->direction().z},
			vec2f32{-re.cam->direction().z, re.cam->direction().x},
		};
	}();

	_renderstate.last_cam_dir = view_dir;

	ScratchPadArena scratchpad{re.arena_temp};

	containers::unordered_set<vec2i32> slabs_current_frame{*scratchpad.arena};
	QuadTreeF32{*re.arena_temp, WORLD_BOUNDS, static_cast<float>(_terrain_params.size)}.insert(
		cam_pos_xz_plane,
		SplitByVisibleBoundingRegion{
			cam_pos_xz_plane,
			static_cast<float>(_terrain_params.size * _terrain_params.cells_view_dist),
			static_cast<float>(_terrain_params.size / 2),
		},
		[&slabs_current_frame](const QuadTreeF32::tree_node_type& n) {
			slabs_current_frame.insert(vec2i32{n.bbox.center()});
		}
	);

	auto slabs_to_spawn = slabs_current_frame | lz::filter([this](const vec2i32 curr_frame_slab) {
							  return !_renderstate.slabs_visible_last_frame.contains(curr_frame_slab);
						  }) |
						  lz::to<containers::unordered_set<vec2i32>>(MemoryArenaAllocator<vec2i32>{*re.arena_temp});

	for (const vec2i32 spawned : slabs_to_spawn) {
		XR_LOG_INFO("Spawned: {}", spawned);
	}

	auto slabs_to_despawn = _renderstate.slabs_visible_last_frame |
							lz::filter([&slabs_current_frame](const vec2i32 last_frame_slab) {
								return !slabs_current_frame.contains(last_frame_slab);
							}) |
							lz::to<containers::unordered_set<vec2i32>>(MemoryArenaAllocator<vec2i32>{*re.arena_temp});

	for (const vec2i32 despawned : slabs_to_despawn) {
		XR_LOG_INFO("Despawning {}", despawned);
	}

	//
	// handle slabs for which we can reuse existing render resources
	// just generate a new terrain chunk and copy the heightmap + colormap
	// to a texture
	const size_t terrain_items = _terrain_params.size * _terrain_params.size;
	tl::optional<concurrencpp::result<size_t>> task_gen_recycle;
	containers::vector<std::pair<vec2i32, SlabRenderResources>> recycled_slab_resources{*scratchpad.arena};

	if (!slabs_to_spawn.empty() && (!slabs_to_despawn.empty() || !_renderstate.slabs_freelist.empty())) {
		//
		// recycle from this frame
		while (!slabs_to_despawn.empty() && !slabs_to_spawn.empty()) {
			auto despawned						 = slabs_to_despawn.extract(slabs_to_despawn.begin());
			auto spawned						 = slabs_to_spawn.extract(slabs_to_spawn.begin());
			auto despawned_slab_render_resources = _renderstate.slabs_table.extract(despawned.value());
			recycled_slab_resources.emplace_back(spawned.value(), despawned_slab_render_resources.mapped());
		}

		//
		// recycle from the free-list
		while (!slabs_to_spawn.empty() && !_renderstate.slabs_freelist.empty()) {
			auto spawned   = slabs_to_spawn.extract(slabs_to_spawn.begin());
			auto free_slab = _renderstate.slabs_freelist.back();
			_renderstate.slabs_freelist.pop_back();
			recycled_slab_resources.emplace_back(spawned.value(), free_slab);
		}

		task_gen_recycle = re.co_runtime->thread_pool_executor()->submit([&recycled_slab_resources,
																		  params   = &_terrain_params,
																		  renderer = re.renderer,
																		  terrain_items]() {
			ScopedSmallArenaType scratch = GlobalMemorySystem::instance()->grab_small_arena();
			const vec2f32 slab_half_size = vec2f32{params->size / 2};
			auto queued_job				 = renderer->create_job(QueueType::Graphics);

			for (auto&& [slab_center, slab_render_res] : recycled_slab_resources) {
				std::span<float> heightmap_data =
					std::span{scratch.arena.alloc_align<float>(terrain_items), terrain_items};
				std::span<vec4ui8> colormap_data =
					std::span{scratch.arena.alloc_align<vec4ui8>(terrain_items), terrain_items};

				make_terrain_heightmap_colormap(
					*params,
					BBoxAA2DF32{
						vec2f32{slab_center} - slab_half_size,
						vec2f32{slab_center} + slab_half_size,
					},
					heightmap_data,
					colormap_data
				);

				copy_render_resources(renderer, *queued_job, heightmap_data, colormap_data, slab_render_res, *params);
			}

			[[maybe_unused]] auto wait_token = renderer->submit_job(std::move(*queued_job));
			return size_t{1};
		});
	}

	//
	// move remaining slabs marked for despawning to the free list
	lz::transform(slabs_to_despawn, std::back_inserter(_renderstate.slabs_freelist), [this](const vec2i32 slab) {
		auto node_handle = _renderstate.slabs_table.extract(slab);
		return node_handle.mapped();
	});

	//
	// handle slabs that need to have render resources created
	// generate terrain slab and new textures
	using CreateTerrainSlabResult	  = tl::expected<TerrainSlabTextures, VulkanError>;
	using TaskCreateTerrainSlabResult = concurrencpp::result<CreateTerrainSlabResult>;

	containers::vector<TaskCreateTerrainSlabResult> tasks_create_slabs_results =
		slabs_to_spawn | lz::map([&, this](vec2i32 slab_center) {
			const size_t terrain_items = _terrain_params.size * _terrain_params.size;

			std::span<float> heightmap{scratchpad.arena->alloc_align<float>(terrain_items), terrain_items};
			std::span<vec4ui8> colormap{scratchpad.arena->alloc_align<vec4ui8>(terrain_items), terrain_items};

			const vec2f32 slab_half_size = vec2f32{_terrain_params.size / 2};

			const BBoxAA2DF32 bounds{
				vec2f32{slab_center} - slab_half_size,
				vec2f32{slab_center} + slab_half_size,
			};

			return re.co_runtime->thread_executor()->submit([terrain_params = &_terrain_params,
															 heightmap,
															 bounds,
															 colormap,
															 coords	  = slab_center,
															 renderer = re.renderer]() {
				make_terrain_heightmap_colormap(*terrain_params, bounds, heightmap, colormap);
				return create_terrain_slab_render_resources(*terrain_params, coords, renderer, heightmap, colormap);
			});
		}) |
		lz::to<containers::vector<TaskCreateTerrainSlabResult>>(
			MemoryArenaAllocator<TaskCreateTerrainSlabResult>{*scratchpad.arena}
		);
	//
	// perform slab visibility check

	containers::vector<vec2i32> slabs_visible_current_frame =
		slabs_current_frame |
		lz::filter([half_size = static_cast<float>(_terrain_params.size),
					ray_dir,
					ray_origin = cam_pos_xz_plane](vec2i32 slab_center) {
			const vec2f32 slab_box_min = vec2f32{slab_center} - vec2f32{half_size};
			const vec2f32 slab_box_max = vec2f32{slab_center} + vec2f32{half_size};
			return ray_aabb_intersect(ray_origin, ray_dir, slab_box_min, slab_box_max);
		}) |
		lz::to<containers::vector<vec2i32>>(MemoryArenaAllocator<vec2i32>{*scratchpad.arena});

	//
	// wait for any tasks to complete
	task_gen_recycle.map([&, this](concurrencpp::result<size_t>& task_result) {
		[[maybe_unused]] size_t res = task_result.get();
		_renderstate.slabs_table.insert(std::begin(recycled_slab_resources), std::end(recycled_slab_resources));
	});

	for (auto&& [task_slab, slab_coords] : lz::zip(tasks_create_slabs_results, slabs_to_spawn)) {
		CreateTerrainSlabResult create_terrain_res = task_slab.get();
		assert(create_terrain_res);

		BindlessImageResourceHandleEntryPair heightmap =
			re.renderer->bindless_sys().add_image(std::move(create_terrain_res->heightmap), nullptr, tl::nullopt);
		BindlessImageResourceHandleEntryPair colormap =
			re.renderer->bindless_sys().add_image(std::move(create_terrain_res->colormap), nullptr, tl::nullopt);

		re.renderer->queue_image_ownership_transfer(heightmap.first);
		re.renderer->queue_image_ownership_transfer(colormap.first);

		assert(!_renderstate.slabs_table.contains(slab_coords));
		[[maybe_unused]] auto [itr, was_inserted] =
			_renderstate.slabs_table.try_emplace(slab_coords, heightmap, colormap);
		assert(was_inserted);
	}

	_renderstate.slabs_visible_last_frame.clear();
	if (slabs_visible_current_frame.empty()) {
		XR_LOG_INFO("No slabs visible");
		return;
	}

	_renderstate.slabs_visible_last_frame.insert(
		std::cbegin(slabs_visible_current_frame), std::cend(slabs_visible_current_frame)
	);
	assert(slabs_visible_current_frame.size() <= MAX_VISIBLE_SLABS);

	vkCmdBindPipeline(re.frame_data->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, re.sres->pipelines.p_terrain.handle());
	vkfn::CmdSetPolygonModeEXT(
		re.frame_data->cmd_buf,
		_uistate.draw_opts[DrawOptions::TerrainWireframeBit] ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL
	);
	const VkDeviceSize vb_offsets[] = {0};
	const VkBuffer vertexbuffers[]	= {_renderstate.vertexbuffer.buffer_handle()};

	const PackedU32PushConstant push_const{_renderstate.instances.first, 0, re.frame_data->id};

	vkCmdBindVertexBuffers(re.frame_data->cmd_buf, 0, 1, vertexbuffers, vb_offsets);
	vkCmdBindIndexBuffer(re.frame_data->cmd_buf, _renderstate.indexbuffer.buffer_handle(), 0, VK_INDEX_TYPE_UINT32);
	vkCmdPushConstants(
		re.frame_data->cmd_buf,
		re.sres->pipelines.p_terrain.layout(),
		VK_SHADER_STAGE_ALL,
		0,
		push_const.size(),
		push_const.as_bytes().data()
	);

	const TerrainLodLevel& lod_lvl = _renderstate.lod_levels[_uistate.lod_level];
	vkCmdDrawIndexed(
		re.frame_data->cmd_buf,
		lod_lvl.index_count,
		std::min<uint32_t>(slabs_visible_current_frame.size(), MAX_VISIBLE_SLABS),
		lod_lvl.offset_index,
		static_cast<int32_t>(lod_lvl.offset_vertex),
		0
	);

	[[maybe_unused]] const auto map_result =
		UniqueMemoryMapping::map_memory(
			re.renderer->device(),
			_renderstate.instances.second.memory,
			_renderstate.instances.second.aligned_chunk_size * re.frame_data->id,
			_renderstate.instances.second.aligned_chunk_size
		)
			.map([this,
				  &re,
				  visible_range = slabs_visible_current_frame |
								  lz::take(std::min<uint32_t>(slabs_visible_current_frame.size(), MAX_VISIBLE_SLABS))](
					 UniqueMemoryMapping mem_map
				 ) {
				TerrainInstanceData* instance = mem_map.as<TerrainInstanceData>();

				for (vec2i32 slab_center : visible_range) {
					auto slab_entry = _renderstate.slabs_table.find(slab_center);
					assert(slab_entry != std::cend(_renderstate.slabs_table));
					instance->colormap = destructure_bindless_resource_handle(slab_entry->second.colormap.first).first;
					instance->heightmap =
						destructure_bindless_resource_handle(slab_entry->second.heightmap.first).first;

					const vec2f xz_translation = vec2f32{slab_center};

					// XR_LOG_INFO("chunk@ {} -> Mat {}", coords, xz_translation);
					instance->world_view_proj =
						// re.g_ubo_data->world_view_proj *
						R4::translate(xz_translation.x, 0.0f, xz_translation.y);

					++instance;
				}
			});
}

void B5::Terrain::user_interface(xray::ui::user_interface* ui, const RenderEvent& re) {
	if (ImGui::CollapsingHeader("Terrain")) {
		char scratch_buffer[1024];
		format_to_n(
			scratch_buffer, "{} pos: {}", xray::ui::fonts::awesome::ICON_FA_CAMERA_RETRO, _uistate.cam_pos_xz_plane
		);

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->ChannelsSplit(2);

		draw_list->ChannelsSetCurrent(1);
		ImGui::TextColored({0.0f, 1.0f, 0.0f, 1.0f}, "%s", scratch_buffer);

		const TerrainLodLevel& current_lod = _renderstate.lod_levels[_uistate.lod_level];

		ImGui::TextColored(
			{0.0f, 1.0f, 0.0f, 1.0f},
			"Size: (%u x %u)\nLod level: %u (vertex count: %u, index count: %u)",
			_terrain_params.size,
			_terrain_params.size,
			_uistate.lod_level,
			current_lod.vertex_count,
			current_lod.index_count
		);

		auto clamped_rangle_slider_fn =
			[](const uint32_t value, const uint32_t min, const uint32_t max, const char* txt) {
				int32_t int_val = static_cast<int32_t>(value);
				const bool result =
					ImGui::DragInt(txt, &int_val, 1.0f, static_cast<int32_t>(min), static_cast<int32_t>(max));
				return pair{result, static_cast<uint32_t>(int_val)};
			};

		if (const auto [changed, new_lod_lvl] = clamped_rangle_slider_fn(
				_uistate.lod_level, 0, static_cast<uint32_t>(_renderstate.lod_levels.size() - 1), "LOD:"
			);
			changed) {
			_uistate.lod_level = new_lod_lvl;
		}

		bool terrain_yreframe = _uistate.draw_opts[DrawOptions::TerrainWireframeBit];
		ImGui::Checkbox("Terrain wireframe", &terrain_yreframe);
		_uistate.draw_opts[DrawOptions::TerrainWireframeBit] = terrain_yreframe;

		ImGui::SeparatorText("Slabs state:");

		const ImVec2 cursor = ImGui::GetCursorScreenPos();
		const ImVec2 rmax	= ImGui::GetWindowContentRegionMax();
		const ImVec2 rmin	= ImGui::GetWindowContentRegionMin();

		constexpr const uint32_t GRID_SIZE = 16;
		const float viewport_width		   = rmax.x - rmin.x;
		const float cell_size			   = viewport_width / static_cast<float>(GRID_SIZE);
		const float WORLD_SECTION_SIZE	   = static_cast<float>(_terrain_params.size * GRID_SIZE) * 0.5f;

		draw_list->AddRectFilled(
			cursor, ImVec2{cursor.x + viewport_width, cursor.y + viewport_width}, IM_COL32(128, 128, 128, 255)
		);

		for (size_t y = 0; y < GRID_SIZE; ++y) {
			for (size_t x = 0; x < GRID_SIZE; ++x) {
				const float xmin = cursor.x + x * cell_size;
				const float xmax = xmin + cell_size;
				const float ymin = cursor.y + y * cell_size;
				const float ymax = ymin + cell_size;
				draw_list->AddRect({xmin, ymin}, {xmax, ymax}, IM_COL32(0, 255, 0, 255));
			}
		}

		auto viewport_transform_fn = [](const vec2f32 pt,
										const float vx,
										const float vy,
										const float viewport_width,
										const float viewport_height) {
			return vec2f32{
				viewport_width * 0.5f * pt.x + (vx + viewport_width * 0.5f),
				viewport_height * 0.5f * pt.y + (vy + viewport_height * 0.5f),
			};
		};

		for (auto&& [slab_center, dontcare] : _renderstate.slabs_table) {
			vec2f32 c{slab_center};
			c /= WORLD_SECTION_SIZE;
			c = viewport_transform_fn(c, cursor.x, cursor.y, viewport_width, viewport_width);
			draw_list->AddRectFilled(
				ImVec2{c.x - cell_size * 0.5f, c.y - cell_size * 0.5f},
				ImVec2{c.x + cell_size * 0.5f, c.y + cell_size * 0.5f},
				IM_COL32(255, 255, 0, 255)
			);
		}

		const vec2f32 cam_pos = viewport_transform_fn(
			_renderstate.last_cam_pos / WORLD_SECTION_SIZE, cursor.x, cursor.y, viewport_width, viewport_width
		);
		draw_list->AddCircleFilled({cam_pos.x, cam_pos.y}, 16.0f, IM_COL32(0, 32, 255, 255));

		const vec2f32 dir_endpt = cam_pos + _renderstate.last_cam_dir * 64.0f;
		draw_list->AddLine({cam_pos.x, cam_pos.y}, {dir_endpt.x, dir_endpt.y}, IM_COL32(0, 32, 255, 255), 2.0f);
		const vec2f32 perp_vec{-_renderstate.last_cam_dir.y, _renderstate.last_cam_dir.x};

		const vec2f32 tri = cam_pos + _renderstate.last_cam_dir * 48.0f;
		const vec2f32 v0  = tri - perp_vec * 8.0f;
		const vec2f32 v1  = tri + perp_vec * 8.0f;

		draw_list->AddTriangleFilled({v0.x, v0.y}, {v1.x, v1.y}, {dir_endpt.x, dir_endpt.y}, IM_COL32(0, 32, 255, 255));

		draw_list->ChannelsMerge();
		ImGui::Dummy(ImVec2(viewport_width, viewport_width));

		ImGui::Text(
			"Slabs: in use: %zu, free: %zu", _renderstate.slabs_table.size(), _renderstate.slabs_freelist.size()
		);
		for (const auto& [scenter, dontcare] : _renderstate.slabs_table) {
			ImGui::Text("Slab @ %d, %d", scenter.x, scenter.y);
		}
	}
}
