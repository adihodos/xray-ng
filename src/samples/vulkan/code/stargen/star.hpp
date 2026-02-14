#pragma once

#include <cstdint>
#include <au/au.hh>
#include "stargen.constants.hpp"

namespace xray::stargen {

struct SolarLuminosityUnit : decltype(au::Watts{} * au::mag<3'828>() * au::pow<23>(au::mag<10>())) {
	static constexpr inline const char label[] = "L";
};

constexpr auto solar_luminosity_unit = au::SingularNameFor<SolarLuminosityUnit>{};
constexpr auto solar_luminosity		 = au::QuantityMaker<SolarLuminosityUnit>{};

struct SolarMassUnit : decltype(au::Kilo<au::Grams>{} * au::mag<1'989>() * au::pow<27>(au::mag<10>())) {
	static constexpr inline const char label[] = "M";
};

constexpr auto sol_mass_unit = au::SingularNameFor<SolarMassUnit>{};
constexpr auto sol_mass		 = au::QuantityMaker<SolarMassUnit>{};

struct Star {
	static Star random();
	au::QuantityD<SolarMassUnit> Mass;
	au::QuantityD<SolarLuminosityUnit> Luminosity;
	au::QuantityD<AstronomicalUnit> Ecosphere;
	double Age;
	double Life;
	
};

}  // namespace xray::stargen
