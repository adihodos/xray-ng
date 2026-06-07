#pragma once

#include <cstddef>
#include <cstdint>

namespace xray::base {

struct MemPoolBlock {
	std::byte* _parent_block{};
	size_t _offset{};
	size_t _alloc_size{};
	size_t _aligned_size{};
	size_t _num_blocks{};
	size_t _free_idx{};

	uint32_t _allocations{};
	uint32_t _deallocations{};
};

struct BlockInfo {
	uint32_t size{};
	uint32_t alignment{};
};

class BasicMemoryPool {
	static constexpr size_t kNullBlockId = 0xFFFFFFFFFFFFFFFFu;

public:
	static constexpr size_t kPoolsCount = 16;

	void init_blocks();

	[[nodiscard]] void* alloc_mem(const size_t size, const size_t alignment);
	void free_mem(void* ptr, const size_t size);
	[[nodiscard]] BlockInfo block_info(const void* block_addr) const noexcept;

	void print_report() const noexcept;

private:
	std::byte* _main_block;
	MemPoolBlock _pools[kPoolsCount];
};

}  // namespace xray::base
