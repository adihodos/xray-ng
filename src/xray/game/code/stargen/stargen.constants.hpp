#pragma once

#include <au/au.hh>

namespace xray::stargen {

inline constexpr auto kRadiansPerRotation = au::revolutions(1);
inline constexpr auto kSecondsPerHour	  = au::seconds(3600);

struct AstronomicalUnit : decltype(au::Meters{} * au::mag<149'597'870'700>()) {
	static inline constexpr const char label[] = "au";
};

constexpr auto astonomical_unit	  = au::SingularNameFor<AstronomicalUnit>{};
constexpr auto astronomical_units = au::QuantityMaker<AstronomicalUnit>{};

namespace symbols {
constexpr auto au = au::SymbolFor<AstronomicalUnit>{};
}

// struct GravitationalConstantUnit : decltype(au::Newtons{} * au::Meters{} * au::Meters{} / (au::Kilo<au::Grams> *
// au::Kilo<au::Grams>) *  )

}  // namespace xray::stargen
