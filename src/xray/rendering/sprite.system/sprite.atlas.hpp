#pragma once

#include "xray/xray.hpp"

#include <filesystem>
#include <vector>

#include "xray/rendering/sprite.system/sprite.defs.hpp"

namespace xray::rendering {
struct TextureAtlasData {
	// std::unordered_map<uint64_t, TextureRegion> frames;
	// TODO: fix bug in libconfig serialize with unordered_map
	std::vector<SpriteAtlasEntry> frames;
	std::filesystem::path texture_file;
};
}  // namespace xray::rendering
