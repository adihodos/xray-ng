#include "xray/stargen/stargen.helpers.hpp"
#include "xray/stargen/stargen.consts.hpp"

namespace xray::stargen {

StargenHelper::StargenHelper(tl::optional<U32> seed) {
	if (seed) {
		mRandEng.seed(*seed);
	} else {
		std::random_device rand_device{};
		mRandEng.seed(rand_device());
	}
}

long double StargenHelper::random_number(long double rmin, long double rmax) {
	const auto rand_val = mUniformDist(mRandEng);
	return rmin + (rmax - rmin) * rand_val;
}

long double StargenHelper::about(long double value, long double variation) {
	return value + value * random_number(-variation, variation);
}

long double StargenHelper::random_eccentricity(void) {
	const auto e = 1.0 - std::pow(mUniformDist(mRandEng), ECCENTRICITY_COEFF);
	return e > 0.99 ? 0.99 : e;
}

}  // namespace xray::stargen
