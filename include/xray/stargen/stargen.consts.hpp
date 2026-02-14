#pragma once

#include <cstdint>
#include <cmath>

namespace xray::stargen {

/// Conversion coeffs
constexpr inline double RADIANS_PER_ROTATION = (6.2831853072); /* 2.0 * PI                 */
constexpr inline double SECONDS_PER_HOUR	 = (3600.0);
constexpr inline double CM_PER_AU			 = (1.495978707e13); /* number of cm in an AU	*/
constexpr inline double CM_PER_KM			 = (1.0E5);			 /* number of cm in a km		*/
constexpr inline double KM_PER_AU			 = (CM_PER_AU / CM_PER_KM);
constexpr inline double CM_PER_METER		 = (100.0);
constexpr inline double MILLIBARS_PER_BAR	 = (1000.00);

constexpr inline double ECCENTRICITY_COEFF = (0.077);	/* Dole's was 0.077			*/
constexpr inline double PROTOPLANET_MASS   = (1.0e-15); /* Units of solar masses	*/

/// Sun data
constexpr inline double SOLAR_MASS_IN_GRAMS		   = (1.989e33);  /* Units of grams			*/
constexpr inline double SOLAR_MASS_IN_KILOGRAMS	   = (1.989e30);  /* Units of kg				*/
constexpr inline double SOLAR_MASS_IN_EARTH_MASSES = (332775.64); /* Units of Earth Mass      */
constexpr inline double SOLAR_RADIUS_IN_M		   = (6.96e8);	  /* Units of meters          */
constexpr inline double SOLAR_RADIUS_IN_KM		   = (696000);	  /* Units of kilometers      */
constexpr inline double SOLAR_RADIUS_IN_AU		   = (4.65e-3);	  /* Units of AU              */
constexpr inline double SOLAR_TEMPERATURE		   = (6000);	  /* Units of degrees Kelvin  */

/* Convert from SM to EM  */
inline constexpr double to_em(const double x) noexcept { return x * SOLAR_MASS_IN_EARTH_MASSES; }
/* Convert from EM to SM  */
inline constexpr double to_sm(const double x) noexcept { return x / SOLAR_MASS_IN_EARTH_MASSES; }

/// Earth data
constexpr inline double FREEZING_POINT_OF_WATER		 = (273.15);   /* Units of degrees Kelvin	*/
constexpr inline double BOILING_POINT_OF_WATER		 = (373.35);   /* Units of degrees Kelvin	*/
constexpr inline double CHANGE_IN_EARTH_ANG_VEL		 = (-1.3E-15); /* Units of radians/sec/year*/
constexpr inline double EARTH_MASS_IN_GRAMS			 = (5.977E27); /* Units of grams			*/
constexpr inline double EARTH_RADIUS				 = (6.378E8);  /* Units of cm				*/
constexpr inline double EARTH_DENSITY				 = (5.52);	   /* Units of g/cc			*/
constexpr inline double EARTH_RADIUS_IN_KM			 = (6378.0);   /* Units of km				*/
constexpr inline double EARTH_ACCELERATION			 = (980.7);	   /* Units of cm/sec2			*/
constexpr inline double EARTH_AXIAL_TILT			 = (23.4);	   /* Units of degrees			*/
constexpr inline double EARTH_EXOSPHERE_TEMP		 = (1273.0);   /* Units of degrees Kelvin	*/
constexpr inline double EARTH_EFFECTIVE_TEMP		 = (255.0);	   /* Units of degrees Kelvin (was 250) */
constexpr inline double EARTH_WATER_MASS_PER_AREA	 = (3.83e15);  /* grams per square km		*/
constexpr inline double EARTH_SURF_PRES_IN_MILLIBARS = (1013.25);
constexpr inline double EARTH_SURF_PRES_IN_BARS		 = (1.01325);
constexpr inline double EARTH_SURF_PRES_IN_MMHG		 = (760.);	 /* Dole p. 15				*/
constexpr inline double EARTH_SURF_PRES_IN_PSI		 = (14.696); /* Pounds per square inch	*/
constexpr inline double EARTH_CONVECTION_FACTOR		 = (0.43);	 /* from Hart, eq.20			*/
constexpr inline double EARTH_AVERAGE_CELSIUS		 = (14.0);	 /* Average Earth Temperature*/
constexpr inline double EARTH_AVERAGE_KELVIN		 = (EARTH_AVERAGE_CELSIUS + FREEZING_POINT_OF_WATER);
constexpr inline double EARTH_DAYS_IN_A_YEAR		 = (365.256); /* Earth days per Earth year*/

// Atmosphere calculation
constexpr inline double MMHG_TO_MILLIBARS = (EARTH_SURF_PRES_IN_MILLIBARS / EARTH_SURF_PRES_IN_MMHG);
constexpr inline double PSI_TO_MILLIBARS  = (EARTH_SURF_PRES_IN_MILLIBARS / EARTH_SURF_PRES_IN_PSI);
constexpr inline double PPM_PRESSURE	  = (EARTH_SURF_PRES_IN_MILLIBARS / 1000000.); /* Parts per million */
constexpr inline double MMHG_TO_PPM		  = (1000000 / EARTH_SURF_PRES_IN_MMHG);

constexpr inline double H20_ASSUMED_PRESSURE   = (47. * MMHG_TO_MILLIBARS); /* Dole, p. 15    */
constexpr inline double MIN_O2_IPP			   = (72. * MMHG_TO_MILLIBARS); /* Dole, p. 15	*/
constexpr inline double MAX_HABITABLE_PRESSURE = (118 * PSI_TO_MILLIBARS);	/* Dole, p. 16	*/

constexpr inline double GAS_RETENTION_THRESHOLD	  = (6.0);		/* ratio of esc vel to RMS vel */
constexpr inline double ASTEROID_MASS_LIMIT		  = (0.001);	/* Units of Earth Masses	*/
constexpr inline double CLOUD_COVERAGE_FACTOR	  = (1.839e-8); /* Km2/kg					*/
constexpr inline double ICE_ALBEDO				  = (0.7);
constexpr inline double CLOUD_ALBEDO			  = (0.52);
constexpr inline double GAS_GIANT_ALBEDO		  = (0.5); /* albedo of a gas giant	*/
constexpr inline double AIRLESS_ICE_ALBEDO		  = (0.5);
constexpr inline double EARTH_ALBEDO			  = (0.3); /* was .33 for a while */
constexpr inline double GREENHOUSE_TRIGGER_ALBEDO = (0.20);
constexpr inline double ROCKY_ALBEDO			  = (0.15);
constexpr inline double ROCKY_AIRLESS_ALBEDO	  = (0.07);
constexpr inline double WATER_ALBEDO			  = (0.04);

constexpr inline double GRAV_CONSTANT	   = (6.672e-8); /* units of dyne cm2/gram2	*/
constexpr inline double MOLAR_GAS_CONST	   = (8314.41);	 /* units: g*m2/(sec2*K*mol) */
constexpr inline double K				   = (50.0);	 /* K = gas/dust ratio		*/
constexpr inline double B				   = (1.2e-5);	 /* Used in Crit_mass calc	*/
constexpr inline double DUST_DENSITY_COEFF = (2.0e-3);	 /* A in Dole's paper		*/
constexpr inline double ALPHA			   = (5.0);		 /* Used in density calcs	*/
constexpr inline double N				   = (3.0);		 /* Used in density calcs	*/
constexpr inline double J				   = (1.46e-19); /* Used in day-length calcs (cm2/sec2 g) */

constexpr inline double INCREDIBLY_LARGE_NUMBER =
#ifdef HUGE_VAL
	HUGE_VAL
#else
	(9.9999e37)
#endif
	;

/*	Now for a few molecular weights (used for RMS velocity calcs):	   */
/*	This table is from Dole's book "Habitable Planets for Man", p. 38  */

constexpr inline auto ATOMIC_HYDROGEN	= (1.0);   /* H   */
constexpr inline auto MOL_HYDROGEN		= (2.0);   /* H2  */
constexpr inline auto HELIUM			= (4.0);   /* He  */
constexpr inline auto ATOMIC_NITROGEN	= (14.0);  /* N   */
constexpr inline auto ATOMIC_OXYGEN		= (16.0);  /* O   */
constexpr inline auto METHANE			= (16.0);  /* CH4 */
constexpr inline auto AMMONIA			= (17.0);  /* NH3 */
constexpr inline auto WATER_VAPOR		= (18.0);  /* H2O */
constexpr inline auto NEON				= (20.2);  /* Ne  */
constexpr inline auto MOL_NITROGEN		= (28.0);  /* N2  */
constexpr inline auto CARBON_MONOXIDE	= (28.0);  /* CO  */
constexpr inline auto NITRIC_OXIDE		= (30.0);  /* NO  */
constexpr inline auto MOL_OXYGEN		= (32.0);  /* O2  */
constexpr inline auto HYDROGEN_SULPHIDE = (34.1);  /* H2S */
constexpr inline auto ARGON				= (39.9);  /* Ar  */
constexpr inline auto CARBON_DIOXIDE	= (44.0);  /* CO2 */
constexpr inline auto NITROUS_OXIDE		= (44.0);  /* N2O */
constexpr inline auto NITROGEN_DIOXIDE	= (46.0);  /* NO2 */
constexpr inline auto OZONE				= (48.0);  /* O3  */
constexpr inline auto SULPH_DIOXIDE		= (64.1);  /* SO2 */
constexpr inline auto SULPH_TRIOXIDE	= (80.1);  /* SO3 */
constexpr inline auto KRYPTON			= (83.8);  /* Kr  */
constexpr inline auto XENON				= (131.3); /* Xe  */

/*	The following defines are used in the kothari_radius function 	*/
constexpr inline double A1_20	= (6.485e12);  /* All units are in cgs system.	 */
constexpr inline double A2_20	= (4.0032e-8); /*	 ie: cm, g, dynes, etc.		 */
constexpr inline double BETA_20 = (5.71e12);

constexpr inline double JIMS_FUDGE = (1.004);

/*	 The following defines are used in determining the fraction of a planet	 */
/*	covered with clouds in function cloud_fraction							 */
#define Q1_36 (1.258e19) /* grams	*/
#define Q2_36 (0.0698)	 /* 1/Kelvin */

//	Atomic numbers, are used as a gas ID key

constexpr inline uint32_t AN_H	= 1;
constexpr inline uint32_t AN_HE = 2;
constexpr inline uint32_t AN_C	= 6;
constexpr inline uint32_t AN_N	= 7;
constexpr inline uint32_t AN_O	= 8;
constexpr inline uint32_t AN_F	= 9;
constexpr inline uint32_t AN_NE = 10;
constexpr inline uint32_t AN_P	= 15;
constexpr inline uint32_t AN_S	= 16;
constexpr inline uint32_t AN_CL = 17;
constexpr inline uint32_t AN_AR = 18;
constexpr inline uint32_t AN_FE = 26;
constexpr inline uint32_t AN_NI = 28;
constexpr inline uint32_t AN_CU = 29;
constexpr inline uint32_t AN_BR = 35;
constexpr inline uint32_t AN_KR = 36;
constexpr inline uint32_t AN_I	= 53;
constexpr inline uint32_t AN_XE = 54;
constexpr inline uint32_t AN_HG = 80;
constexpr inline uint32_t AN_AT = 85;
constexpr inline uint32_t AN_RN = 86;
constexpr inline uint32_t AN_FR = 87;

constexpr inline uint32_t AN_NH3	  = 900;
constexpr inline uint32_t AN_H2O	  = 901;
constexpr inline uint32_t AN_CO2	  = 902;
constexpr inline uint32_t AN_O3		  = 903;
constexpr inline uint32_t AN_CH4	  = 904;
constexpr inline uint32_t AN_SO2	  = 905;
constexpr inline uint32_t AN_CH3CH2OH = 906;
constexpr inline uint32_t AN_CO		  = 907;

template <typename T>
	requires std::is_arithmetic_v<T>
constexpr inline T pow2(const T a) noexcept {
	return a * a;
}

template <typename T>
	requires std::is_arithmetic_v<T>
constexpr inline T pow3(const T a) noexcept {
	return a * a * a;
}

template <typename T>
	requires std::is_arithmetic_v<T>
constexpr inline T pow4(const T a) noexcept {
	return a * a * a * a;
}

template <typename T>
	requires std::is_floating_point_v<T>
inline constexpr T pow1_2(const T a) noexcept {
	return std::sqrt(a);
}

template <typename T>
	requires std::is_floating_point_v<T>
inline constexpr T pow1_3(const T a) noexcept {
	return std::pow(a, (0.3333333));
}

template <typename T>
	requires std::is_floating_point_v<T>
inline constexpr T pow2_3(const T a) noexcept {
	return std::pow(a, (0.66666666));
}

template <typename T>
	requires std::is_floating_point_v<T>
inline constexpr T pow4_3(const T a) noexcept {
	return std::pow(a, (1.3333333));
}

template <typename T>
	requires std::is_floating_point_v<T>
inline constexpr T pow1_4(const T a) noexcept {
	return std::sqrt(std::sqrt(a));
}

template <typename T>
	requires std::is_floating_point_v<T>
inline constexpr T pow1_8(const T a) noexcept {
	return std::pow(a, 0.125);
}

}  // namespace xray::stargen
