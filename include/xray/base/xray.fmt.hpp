#pragma once

#include <span>
#include <fmt/format.h>

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

}  // namespace xray::base
