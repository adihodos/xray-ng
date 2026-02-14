#pragma once

#include "xray/base/xray.types.hpp"
#include <string>
#include "xray/stargen/stargen.helpers.hpp"

namespace xray::stargen {

class SG_Star;
class SG_Stardust;

/// A Solar System (contains a primary star, several planets, and a dust cloud).
/**
	Librairie de génération d'un système planétaire réaliste à partir d'informations
	sur l'étoile primaire (cad centrale) du système.
*/
class SG_SolarSystem {
public:
	SG_SolarSystem(const U32 seed);
	~SG_SolarSystem();
	void setSeed(long seed);
	void generateSystem(std::string filename);
	void generateSolarSystem(std::string filename);
	void setStarMass(double mass = 1.0);
	void setStarAge(long double age);
	void setStarLuminosity(long double luminosity = 1);
	void setStarBoloMagnitude(long double magnitude);
	void setStarName(std::string name);
	SG_Star* getStar();

private:
	void calculatePlanets();
	void writePlanets();

	StargenHelper mHelper;
	SG_Star* mSun;			 ///< The central star of eh planetary system
	SG_Stardust* mStardust;	 ///< The stardust cloud sourrounding the star
};
	
}  // namespace xray::stargen
