#include "xray/base/memory.pool.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

#include "xray/base/memory.os.hpp"
#include "xray/base/memory.arena.hpp"
#include "xray/base/xray.misc.hpp"
#include "xray/base/logger.hpp"

namespace xray::base {

struct PoolBlockConfig {
	uint32_t alloc_size{};
	uint32_t num_blocks{};
};

inline constexpr PoolBlockConfig PoolConfigTable[BasicMemoryPool::kPoolsCount] = {
	PoolBlockConfig{.alloc_size = 16, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 24, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 32, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 64, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 128, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 256, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 348, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 512, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 1024, .num_blocks = 4096},
	PoolBlockConfig{.alloc_size = 2048, .num_blocks = 2048},
	PoolBlockConfig{.alloc_size = 4096, .num_blocks = 2048},
	PoolBlockConfig{.alloc_size = 8192, .num_blocks = 2048},
	PoolBlockConfig{.alloc_size = 16384, .num_blocks = 2048},
	PoolBlockConfig{.alloc_size = 32768, .num_blocks = 1024},
	PoolBlockConfig{.alloc_size = 65536, .num_blocks = 512},
	PoolBlockConfig{.alloc_size = 131072, .num_blocks = 256},
};

struct PoolBlockHeaderTag {
	static constexpr uint32_t kMaxAllocSize	  = 1 << 24;
	static constexpr uint32_t kMaxAlign		  = 1 << 8;
	static constexpr uint32_t kMaxBlockOffset = 1 << 24;

	uintptr_t next_free;
	struct {
		uint32_t block_offset : 24;
		uint32_t pool_offset  : 8;
	} pool;
	struct {
		uint32_t size  : 24;
		uint32_t align : 8;
	} alloc;
};

static_assert(sizeof(PoolBlockHeaderTag) == sizeof(uintptr_t) * 2, "MonkaHMMM");

void BasicMemoryPool::init_blocks() {
	constexpr size_t required_size = []() {
		size_t blocks_size{};
		for (const PoolBlockConfig& block_cfg : PoolConfigTable) {
			blocks_size += block_cfg.num_blocks * (block_cfg.alloc_size + sizeof(PoolBlockHeaderTag));
		}
		return blocks_size;
	}();

	_main_block = static_cast<std::byte*>(os_reserve_mem(required_size * 2));

	size_t main_block_offset = 0;
	uintptr_t end_addr		 = reinterpret_cast<uintptr_t>(_main_block);

	for (size_t idx = 0; idx < std::size(PoolConfigTable); ++idx) {
		end_addr = round_up(end_addr, alignof(PoolBlockHeaderTag));

		_pools[idx] = MemPoolBlock{
			._parent_block = _main_block,
			._offset	   = end_addr - reinterpret_cast<uintptr_t>(_main_block),
			._alloc_size   = PoolConfigTable[idx].alloc_size,
			._aligned_size = 0,
			._num_blocks   = PoolConfigTable[idx].num_blocks,
			._free_idx	   = 0,
		};

		const uintptr_t blk_entry_addr =
			round_up(end_addr + sizeof(PoolBlockHeaderTag) + _pools[idx]._alloc_size, alignof(PoolBlockHeaderTag));
		_pools[idx]._aligned_size = blk_entry_addr - end_addr;

		end_addr += _pools[idx]._aligned_size * _pools[idx]._num_blocks;
	}

	const ptrdiff_t real_size = end_addr - reinterpret_cast<uintptr_t>(_main_block);
	assert(real_size < required_size * 2);
	void* reserved = os_commit_mem(_main_block, real_size);

	for (size_t idx = 0; idx < std::size(PoolConfigTable); ++idx) {
		const MemPoolBlock* pool = &_pools[idx];

		uintptr_t hdr_addr = reinterpret_cast<uintptr_t>(_main_block) + pool->_offset;

		for (size_t block_idx = 0; block_idx < pool->_num_blocks; ++block_idx) {
			assert(hdr_addr % alignof(PoolBlockHeaderTag) == 0);

			PoolBlockHeaderTag* hdr	 = reinterpret_cast<PoolBlockHeaderTag*>(hdr_addr);
			const bool is_last_block = (block_idx + 1) == pool->_num_blocks;
			hdr->next_free = (block_idx + 1) * !is_last_block + static_cast<uint32_t>(kNullBlockId * is_last_block);

			hdr_addr += pool->_aligned_size;
		}
	}

	xray::base::details::poison_memory_region(_main_block, real_size);
}

[[nodiscard]] void* BasicMemoryPool::alloc_mem(const size_t size, const size_t alignment) {
	const size_t requested_size = round_up((size + sizeof(PoolBlockHeaderTag)), alignment);
	auto block_itr				= std::upper_bound(
		 std::begin(_pools),
		 std::end(_pools),
		 requested_size,
		 [](const size_t requested_size, const MemPoolBlock& pool_block) {
			 return requested_size <= pool_block._alloc_size;
		 }
	 );

	uintptr_t hdr_addr_start{};
	uintptr_t data_addr_start{};
	for (;;) {
		if (block_itr == std::end(_pools)) {
			XR_LOG_ERR("No block can serve the requested size {} (align {})", size, alignment);
			print_report();
			return nullptr;
		}

		if (block_itr->_free_idx == kNullBlockId) {
			//
			// TODO: handle this later
			XR_LOG_ERR(
				"Pool for objects of size {} is exhausted. Download more RAM I guess ...", block_itr->_alloc_size
			);
			print_report();
			return nullptr;
		}

		hdr_addr_start = reinterpret_cast<uintptr_t>(block_itr->_parent_block) + block_itr->_offset +
						 block_itr->_free_idx * block_itr->_aligned_size;
		assert(hdr_addr_start % alignof(PoolBlockHeaderTag) == 0);

		const uintptr_t addr_start_data = hdr_addr_start + sizeof(PoolBlockHeaderTag);
		data_addr_start					= round_up(addr_start_data, alignment);
		const ptrdiff_t used_size		= (data_addr_start + size) - hdr_addr_start;

		if (used_size > block_itr->_alloc_size) {
			//
			// doesn’t fit due to alignment requirements
			std::advance(block_itr, 1);
		} else {
			break;
		}
	}

	const size_t block_idx = block_itr->_free_idx;

	xray::base::details::unpoison_memory_region(reinterpret_cast<void*>(hdr_addr_start), block_itr->_aligned_size);

	PoolBlockHeaderTag* hdr = reinterpret_cast<PoolBlockHeaderTag*>(hdr_addr_start);
	block_itr->_free_idx	= hdr->next_free;

	assert(block_idx <= PoolBlockHeaderTag::kMaxBlockOffset);
	assert(alignment <= PoolBlockHeaderTag::kMaxAlign);
	assert(size <= PoolBlockHeaderTag::kMaxAllocSize);

	hdr->pool.block_offset = static_cast<uint32_t>(block_idx);
	hdr->pool.pool_offset  = static_cast<uint32_t>(std::distance(std::begin(_pools), block_itr));
	hdr->alloc.align	   = alignment;
	hdr->alloc.size		   = size;
	hdr->next_free		   = kNullBlockId;

	std::byte* returned_alloc = reinterpret_cast<std::byte*>(data_addr_start);
	assert((data_addr_start - (hdr_addr_start + sizeof(PoolBlockHeaderTag))) == 0);

	block_itr->_allocations += 1;

	return returned_alloc;
}

[[nodiscard]] BlockInfo BasicMemoryPool::block_info(const void* block_addr) const noexcept {
	assert(block_addr != nullptr);

	PoolBlockHeaderTag* hdr =
		reinterpret_cast<PoolBlockHeaderTag*>(reinterpret_cast<uintptr_t>(block_addr) - sizeof(PoolBlockHeaderTag));

	return BlockInfo{
		.size	   = hdr->alloc.size,
		.alignment = hdr->alloc.align,
	};
}

void BasicMemoryPool::free_mem(void* ptr, const size_t size) {
	if (!ptr) {
		return;
	}

	PoolBlockHeaderTag* hdr =
		reinterpret_cast<PoolBlockHeaderTag*>(reinterpret_cast<uintptr_t>(ptr) - sizeof(PoolBlockHeaderTag));

	auto block_itr = std::begin(_pools);
	std::advance(block_itr, hdr->pool.pool_offset);

	const uintptr_t block_start = reinterpret_cast<uintptr_t>(block_itr->_parent_block) + block_itr->_offset;
	const uintptr_t block_end	= block_start + block_itr->_num_blocks * block_itr->_aligned_size;

	assert(reinterpret_cast<uintptr_t>(ptr) >= block_start);
	assert(reinterpret_cast<uintptr_t>(ptr) <= block_end);

	if (block_itr->_free_idx == kNullBlockId) {
		block_itr->_free_idx = hdr->pool.block_offset;
		hdr->next_free		 = kNullBlockId;
	} else {
		const uintptr_t prev_free_idx = block_itr->_free_idx;
		block_itr->_free_idx		  = hdr->pool.block_offset;
		hdr->next_free				  = prev_free_idx;
	}

	hdr->alloc.align = 0;
	hdr->alloc.size	 = 0;

	block_itr->_deallocations += 1;

	xray::base::details::poison_memory_region(
		block_itr->_parent_block + block_itr->_offset + hdr->pool.block_offset * block_itr->_aligned_size,
		block_itr->_aligned_size
	);
}

void BasicMemoryPool::print_report() const noexcept {
	struct BlockStats {
		uint32_t alloc_size;
		uint32_t alloc_count;
		uint32_t dealloc_count;
	};

	struct PoolStats {
		size_t alloc_count{};
		size_t free_count{};
		BlockStats block_stats[kPoolsCount]{};
	} pool_stats{};

	for (size_t idx = 0; idx < std::size(_pools); ++idx) {
		const MemPoolBlock& blk = _pools[idx];

		pool_stats.alloc_count += blk._allocations;
		pool_stats.free_count += blk._deallocations;
		pool_stats.block_stats[idx] = {
			PoolConfigTable[idx].alloc_size,
			blk._allocations,
			blk._deallocations,
		};
	}

	std::ranges::sort(pool_stats.block_stats, [](const BlockStats& a, const BlockStats& b) {
		return a.alloc_count > b.alloc_count;
	});

	XR_LOG_ERR(
		"VulkanMemoryAllocator:\nTotal allocations: {}\nTotal deallocations: {}",
		pool_stats.alloc_count,
		pool_stats.free_count
	);

	XR_LOG_ERR("Pools by allocations:");
	for (const BlockStats& b : pool_stats.block_stats) {
		XR_LOG_ERR("pool size: {} :: {} / {}", b.alloc_size, b.alloc_count, b.dealloc_count);
	}
}

}  // namespace xray::base
