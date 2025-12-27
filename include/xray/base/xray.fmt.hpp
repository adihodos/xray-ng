#pragma once

#include <span>
#include <fmt/format.h>
#include "xray/base/containers/arena.string.hpp"

namespace xray::base {

template <typename... Fargs>
auto format_to_n(std::span<char> out, fmt::string_view fmt, const Fargs&... args) {
	if (!out.empty()) {
		auto result = fmt::vformat_to_n(out.begin(), out.size() - 1, fmt, fmt::make_format_args(args...));
		*result.out = 0;
	}
}

template <size_t N, typename... Fargs>
inline auto format_to_n(char (&array_out)[N], fmt::string_view fmt_spec, const Fargs&... args) {
	auto fmt_result = fmt::vformat_to(array_out, fmt_spec, fmt::make_format_args(args...));
	*fmt_result.out = 0;
}

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
