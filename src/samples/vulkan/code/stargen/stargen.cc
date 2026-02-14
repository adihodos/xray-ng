#include "stargen/star.hpp"

#include <random>

namespace xray::stargen {

au::QuantityD<SolarLuminosityUnit> compute_luminosity(const au::QuantityD<SolarMassUnit> mass) noexcept {
	double n = 0.0;
	if (mass < sol_mass(1.0)) {
		n = 1.75 * (mass.in(SolarMassUnit{}) - .1) + 3.325;
	} else {
		n = 0.5 * (2.0 - mass.in(SolarMassUnit{})) + 4.4;
	}

	return solar_luminosity(std::pow(mass.in(SolarMassUnit{}), n));
}

Star Star::random() {
	std::random_device rand_dev{};
	std::mt19937 rand_eng{rand_dev()};
	std::uniform_real_distribution<double> udist{0.1, 10.0};

	const auto mass = sol_mass(udist(rand_eng));
	return Star{
		.Mass		= mass,
		.Luminosity = compute_luminosity(mass),
	};
}
}  // namespace xray::stargen
