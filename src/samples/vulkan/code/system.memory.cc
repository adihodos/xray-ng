#include "system.memory.hpp"

namespace B5 {

namespace GlobalMemoryBuffers {
std::byte SmallArenas[16][GlobalMemoryConfig::SMALL_ARENA_SIZE];
std::byte MediumArenas[8][GlobalMemoryConfig::MEDIUM_ARENA_SIZE];
std::byte LargeArenas[4][GlobalMemoryConfig::LARGE_ARENA_SIZE];
};

GlobalMemorySystem::GlobalMemorySystem() noexcept
{
    for (size_t idx = 0; idx < std::size(GlobalMemoryBuffers::SmallArenas); ++idx) {
        free_small.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::SmallArenas[idx]));
    }

    for (size_t idx = 0; idx < std::size(GlobalMemoryBuffers::MediumArenas); ++idx) {
        free_medium.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::MediumArenas[idx]));
    }

    for (size_t idx = 0; idx < std::size(GlobalMemoryBuffers::LargeArenas); ++idx) {
        free_large.push(reinterpret_cast<void*>(&GlobalMemoryBuffers::LargeArenas[idx]));
    }
}

}
