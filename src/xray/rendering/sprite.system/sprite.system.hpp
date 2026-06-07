#pragma once

#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <span>

#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <swl/variant.hpp>

#include "xray/xray.hpp"
#include "xray/rendering/sprite.system/sprite.defs.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.error.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.buffer.hpp"
#include "xray/rendering/vulkan.renderer/vulkan.bindless.defs.hpp"

namespace xray::base {
class ConfigSystem;
struct MemoryArena;
}  // namespace xray::base

namespace xray::scene {
class SceneResources;
}

namespace xray::rendering {

struct SpriteAtlasError {
	std::string what;
};

class VulkanRenderer;
struct FrameRenderData;
struct SpriteVertex;

using SpriteAtlasCreationError = swl::variant<SpriteAtlasError, VulkanError>;

struct SpriteVertex {
	math::vec2f32 pos;
	math::vec2f32 uv;
	uint32_t texid;
	uint32_t color;
};

struct SpriteSystemRenderContext {
	VulkanRenderer* renderer;
	const FrameRenderData* frame_data;
	const scene::SceneResources* sres;
};

class SpriteSystem {
private:
	struct PrivateConstructionToken {
		explicit PrivateConstructionToken() noexcept = default;
	};

	struct BufferCursor {
		uint64_t frame	: 8;
		uint64_t index	: 16;
		uint64_t vertex : 40;

		BufferCursor() noexcept : frame{0}, index{0}, vertex{0} {}
	};

	static_assert(sizeof(BufferCursor) == 8, "MonkaS");

	static constexpr const uint32_t MAX_SPRITES = 2048;
	static constexpr const uint32_t MAX_INDICES = MAX_SPRITES * 6;

	VulkanBuffer _vertex_buffer;
	VulkanBuffer _index_buffer;
	BindlessImageResourceHandleEntryPair _atlas;
	std::unordered_map<SpriteHandleType, SpriteAtlasEntry> _sprites_table;
	SpriteVertex* _mapped_vb{};
	uint16_t* _mapped_ib{};
	BufferCursor _cursor{};

public:
	SpriteSystem(SpriteSystem&&) noexcept = default;

	SpriteSystem(
		PrivateConstructionToken,
		VulkanBuffer&& vertex_buffer,
		VulkanBuffer&& index_buffer,
		BindlessImageResourceHandleEntryPair atlas_res,
		std::unordered_map<SpriteHandleType, SpriteAtlasEntry> frames,
		SpriteVertex* sprite_vtx_ptr,
		uint16_t* sprite_idx_ptr
	);

	static tl::expected<SpriteSystem, SpriteAtlasCreationError> from_file(
		xray::base::MemoryArena& arena,
		const std::filesystem::path& path,
		const xray::base::ConfigSystem& cfg_sys,
		VulkanRenderer& renderer
	);

	tl::optional<SpriteAtlasEntry> get_sprite_by_name(const std::string_view sprite) const noexcept;
	tl::optional<SpriteAtlasEntry> get_sprite_by_id(const SpriteHandleType id) const noexcept;

	void draw(const SpriteEntry& sprite, const float x, const float y, const uint32_t color) {
		draw_sprite(sprite, x + sprite.F32Size.width * 0.5f, y + sprite.F32Size.height * 0.5f, color);
	}

	void draw_with_origin(const SpriteEntry& sprite, const float ox, const float oy, const uint32_t color) {
		draw_sprite(sprite, ox, oy, color);
	}

	void draw_scaled_rotated(
		const SpriteEntry& sprite,
		const float x,
		const float y,
		const float scale,
		const float rotation,
		const uint32_t color
	) {
		draw_sprite(
			sprite,
			x + sprite.F32Size.width * 0.5f,
			y + sprite.F32Size.height * 0.5f,
			scale,
			std::cos(rotation),
			std::sin(rotation),
			color
		);
	}

	void draw_scaled_rotated_with_origin(
		const SpriteEntry& sprite,
		const float ox,
		const float oy,
		const float scale,
		const float rotation,
		const uint32_t color
	) {
		draw_sprite(sprite, ox, oy, scale, std::cos(rotation), std::sin(rotation), color);
	}

	void draw_scaled_rotated_with_origin(
		const SpriteEntry& sprite,
		const float ox,
		const float oy,
		const float scale,
		const float cos_rotation,
		const float sine_rotation,
		const uint32_t color
	) {
		draw_sprite(sprite, ox, oy, scale, cos_rotation, sine_rotation, color);
	}

	void render(const SpriteSystemRenderContext& rctx);

private:
	void draw_sprite(const SpriteEntry& sprite, const float ox, const float oy, const uint32_t color);
	// void draw_sprite(const SpriteAtlasEntry& sprite, const float ox, const float oy, const uint32_t color);
	// void draw_sprite(const SpriteAtlasEntry& sprite,
	//                  const float ox,
	//                  const float oy,
	//                  const float scale,
	//                  const float cos_rotation,
	//                  const float sine_rotation,
	//                  const uint32_t color);

	void draw_sprite(
		const SpriteEntry& sprite,
		const float ox,
		const float oy,
		const float scale,
		const float cos_rotation,
		const float sine_rotation,
		const uint32_t color
	);

	void add_sprite_quad(const std::span<const SpriteVertex> quad);
	SpriteVertex* vertex_ptr() const noexcept { return _mapped_vb + _cursor.frame * MAX_SPRITES + _cursor.vertex; }
	uint16_t* index_ptr() const noexcept { return _mapped_ib + _cursor.frame * MAX_INDICES + _cursor.index; }
};

}  // namespace xray::rendering
