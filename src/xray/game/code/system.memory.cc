#include "system.memory.hpp"

#include "xray/base/array_dimension.hpp"

namespace B5 {

namespace GlobalMemoryBuffers {
xray::U8 SmallArenas[16][GlobalMemoryConfig::SMALL_ARENA_SIZE];
xray::U8 MediumArenas[8][GlobalMemoryConfig::MEDIUM_ARENA_SIZE];
xray::U8 LargeArenas[4][GlobalMemoryConfig::LARGE_ARENA_SIZE];
};	// namespace GlobalMemoryBuffers

GlobalMemorySystem::GlobalMemorySystem() noexcept {
	for (xray::ISIZE idx = 0; idx < xray::base::xr_array_size(GlobalMemoryBuffers::SmallArenas); ++idx) {
		free_small.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::SmallArenas[idx]));
	}

	for (xray::ISIZE idx = 0; idx < xray::base::xr_array_size(GlobalMemoryBuffers::MediumArenas); ++idx) {
		free_medium.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::MediumArenas[idx]));
	}

	for (xray::ISIZE idx = 0; idx < xray::base::xr_array_size(GlobalMemoryBuffers::LargeArenas); ++idx) {
		free_large.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::LargeArenas[idx]));
	}
}

}  // namespace B5
