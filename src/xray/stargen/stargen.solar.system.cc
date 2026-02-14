#include "xray/stargen/stargen.solar.system.hpp"

namespace xray::stargen {
/// Constructor.
/**
@param seed Seed for generation of the random data.
*/
SG_SolarSystem::SG_SolarSystem(long seed) {
	// SG_Utils::writeLog(">> STARGEN starting ", false);
	// SG_Utils::writeLog(
	// "Size of float      :" + std::to_string(sizeof(float)) + " bytes: " + std::to_string(sizeof(float) * 8) +
	// " bits"
	// );
	// SG_Utils::writeLog(
	// "Size of long double:" + std::to_string(sizeof(long double)) +
	// " bytes: " + std::to_string(sizeof(long double) * 8) + " bits"
	// );
	// SG_Utils::writeLog(
	// "Size of short      :" + std::to_string(sizeof(short)) + " bytes: " + std::to_string(sizeof(short) * 8) +
	// " bits"
	// );
	// SG_Utils::writeLog(
	// "Size of int        :" + std::to_string(sizeof(int)) + " bytes: " + std::to_string(sizeof(int) * 8) + " bits"
	// );
	// SG_Utils::writeLog(
	// "Size of long       :" + std::to_string(sizeof(long)) + " bytes: " + std::to_string(sizeof(long) * 8) + " bits"
	// );
	// SG_Utils::writeLog(
	// "Size of long int   :" + std::to_string(sizeof(long int)) + " bytes: " + std::to_string(sizeof(long int) * 8) +
	// " bits"
	// );
	// SG_Utils::writeLog(
	// "Size of double     :" + std::to_string(sizeof(double)) + " bytes: " + std::to_string(sizeof(double) * 8) +
	// " bits"
	// );

	mStardust = nullptr;
	mSun	  = new SG_Star();
	this->setSeed(seed);
}

/// Destructeur
SG_SolarSystem::~SG_SolarSystem() {
	delete mSun;
	if (mStardust) delete mStardust;
}

}  // namespace xray::stargen
