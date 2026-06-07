#pragma once

#include <random>
#include <tl/optional.hpp>

#include "xray/base/xray.types.hpp"

namespace xray::stargen {

class StargenHelper {
public:
	StargenHelper(tl::optional<U32> seed = tl::nullopt);

	long double random_number(long double, long double);
	long double about(long double, long double);
	long double random_eccentricity(void);

	auto randomSeed() const noexcept { return (std::random_device{})(); }

private:
	std::mt19937 mRandEng{};
	std::uniform_real_distribution<double> mUniformDist{0.0, 1.0};
};

}  // namespace xray::stargen
