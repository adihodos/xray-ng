#pragma once

#include <string>
#include <string_view>

namespace xray::stargen {

	class StargenHelper;
/* ------------------------------------------------------------------------- */
/// The primary star of the planetary system.
/**
- The star is initialized with the Sun characteristics.
- The star can then be defined with setMass and setAge. Then the other parameters are
automatically calculated.
- A better manual control of the star characteristics is possible with setLuminosity,
setEcosphere and setLife.
- The star can also be defined randomly with the setRandomStar function.
*/
/* ------------------------------------------------------------------------- */
class SG_Star {
public:
	/// The star is initialized with the Sun characteristics.
	SG_Star() noexcept = default;

	void setRandomStar(StargenHelper& helper);
	void setName(const std::string_view name) { this->mName = name; }
	void setMass(long double mass);
	void setLuminosity(long double luminosity);
	void setEcosphere(long double ecosphere);
	void setLife(long double life);
	void setAge(long double age);
	void setMagnitude(long double magnitude);

	long double getBodyTemperature(long double orbit_radius, long double albedo);
	long double getEffectiveTemperature(long double orbit_radius, long double albedo);
	long double getNearestPlanetOrbit();
	long double getBodePlanetOrbit(int index, StargenHelper& helper);
	long double getFarthestPlanetOrbit();
	long double getStellarDustLimit();
	int getPlanetNumber() const noexcept { return mPlanetNumber; }
	std::string_view getName() const noexcept { return this->mName; }

	std::string mName{};		   ///< The name of the star
	long double mLum{1.0};		   ///< The luminosity of the star (unit=solar lum)
	long double mMass{1.0};		   ///< The mass of the star (unit= solar mass)
	long double mLife{10e9};		   ///< The total lifetime estimlated for the star (unit=year)
	long double mAge{5e9};		   ///< The elapsed lifetile of the star (unit=year)
	long double mR_ecosphere{1.0};  ///< The radius of the ecosphere (unit=UA)
	int mPlanetNumber{0};		   ///< The number of planets orbiting around the star.

private:
	long double calculateLuminosity(long double mass);
	long double calculateEcosphere(long double luminosity);
	long double calculateLife();
};

}  // namespace xray::stargen
