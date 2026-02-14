#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "xray/stargen/stargen.gas.hpp"

namespace xray::stargen {

class SG_Planet;

enum class AtmosphereType : uint8_t {
	None,
	Breathable,
	Unbreathable,
	Stinking,
	Poisonous,
};

inline constexpr size_t MAX_GAZ = 30;  ///< Max number of Gases (can be changed)

/* ------------------------------------------------------------------------- */
/// A planet's Atmosphere.
/* ------------------------------------------------------------------------- */
class SG_Atmosphere {
public:
	SG_Atmosphere(SG_Planet* myPlanet);
	~SG_Atmosphere();
	int calculateAtmosphere();
	AtmosphereType getBreathability();
	std::string getToxicGasList();
	std::string getGasName(int rank, uint32_t stateMask = GasType::All);
	std::string getGasSymbol(int rank, uint32_t stateMask = GasType::All);
	SG_Gas* getGas(int rank, uint32_t stateMask = GasType::All);
	long double getGasRatio(int rank);
	GasType getGasState(int rank);

private:
	int estimateComposition();
	long calculateGasRepartition();
	void sortGasTable();
	long double getGasAmount(SG_Gas* gaz);
	long double getGasPressure(SG_Gas* gaz);

	SG_Planet* mPlanet;			   ///< The planet which this atmosphere belongs.
	long double mTotalAmount{};	   ///< Total amount of gases in the atmosphere.
	long double mTotalIPP{};	   ///< Total pressure for the gases heavier than Nitrogen.
	int mGases{};				   ///< The number of gases in the atmosphere.
	std::vector<SG_Gas> mGasList;  ///< The list of the gas of the atmosphere.
	std::string mPoisonedBy{};	   ///< List of the toxic gas of the atmosphere.
};

}  // namespace xray::stargen
