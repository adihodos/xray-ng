#pragma once

#include <cstdint>
#include <string_view>

namespace xray::stargen {

enum GasType : uint8_t {
	None   = 0,
	Solid  = 1,
	Liquid = 2,
	Gas	   = 4,
	All	   = 7,
};

/* ------------------------------------------------------------------------- */
/// A Gas from an atmosphere.
/* ------------------------------------------------------------------------- */
class SG_Gas {
public:
	struct SG_colour {
		double r;  ///< Red component of the Gas colour
		double g;  ///< Green component of the Gas colour
		double b;  ///< Blue component of the Gas colour
		double a;  ///< Transparency of the Gas
	};

public:
	SG_Gas(
		int AtomicNumber,
		std::string_view Symbol,
		long double AtomicWeight,
		long double MeltPoint,
		long double BoilingPoint,
		long double Density,
		long double Abunds,
		long double Reactivity,
		long double Max_ipp,
		std::string_view Name
	);
	~SG_Gas();

	long double getAbound() const noexcept;
	long double getBoilingPoint(long double pressure) const noexcept;
	long double getMeltingPoint(long double pressure) const noexcept;
	bool isRadioactive() const noexcept;
	bool isToxic(long double surf_pressure) const noexcept;
	GasType getState(long double temperature, long double pressure) const noexcept;
	long double getReact(long double temperature, long double pressure, long double years) const noexcept;
	long double getPVRMS(long double temperature, long double Escape_velocity, long double years) const noexcept;
	long double getFract(long double Molec_weight) const noexcept;

	void setAmount(long double amount);
	long double getAmount() const noexcept;
	long double getWeight() const noexcept;
	int getAtomicNumber() const noexcept;
	std::string_view getSymbol() const noexcept;
	std::string_view getName() const noexcept;
	long double getMaxIPP() const noexcept;
	long double getInspiredPartialPressure(long double surf_pressure) const noexcept;
	void setPartialPressure(long double Partial_pressure);
	long double getPartialPressure() const noexcept;
	void setPartialPercentage(long double pourcentage);
	long double getPartialPercentage() const noexcept;
	void setGasColour(double r, double g, double b, double a);
	SG_colour getGasColour() const noexcept;

private:
	long double getPres2(long double temperature, long double pressure, long double years) const noexcept;
	long double getRMSvelocity(long double temperature) const noexcept;

	std::string_view mName;	   ///< Name of the gas molecule.
	std::string_view mSymbol;  ///< Chemical Symbol of the Gas.
	int mNum;				   ///< Atomic Number (AN).
	long double mWeight;	   ///< Atomic Weight.
	long double mMelt;		   ///< Fusion Point of the molecule (under 1 bar).
	long double mBoil;		   ///< Boiling Point of the molecule (under 1 bar).
	long double mAbunds;	   ///< Aboundance of this gas in the Sun
	long double mReactivity;
	long double mDensity;  ///< Gas density of the gas. (g/cc)
	long double mMax_ipp;  ///< Max inspired partial pressure (millibars)
	SG_colour mGasColour;  ///< The colour of the gas

	// Calculated values for a specific atmosphere
	long double mAmount;			 ///< Amount of the gas in the atmosphere.
	long double mPartialPressure;	 ///< Part of the atmosphere pressure due to the gas.
	long double mPartialPercentage;	 ///< The percentage of this gas in the atmosphere [0..1]
};

}  // namespace xray::stargen
