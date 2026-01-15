#pragma once

#include <span>
#include <fmt/format.h>
#include "xray/base/containers/arena.string.hpp"

namespace xray::base {

using arena_memory_buffer = fmt::basic_memory_buffer<char, fmt::inline_buffer_size, MemoryArenaAllocator<char>>;

auto inline vformat(MemoryArenaAllocator<char> alloc, fmt::string_view fmt, fmt::format_args args)
	-> containers::string {
	auto buf = arena_memory_buffer(alloc);
	fmt::vformat_to(std::back_inserter(buf), fmt, args);
	return containers::string(buf.data(), buf.size(), alloc);
}

template <typename... Args>
auto format(MemoryArena& arena, fmt::string_view fmt, const Args&... args) -> containers::string {
	return vformat(MemoryArenaAllocator<char>{arena}, fmt, fmt::make_format_args(args...));
}

}  // namespace xray::base
