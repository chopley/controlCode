#define __FILEPATH__ "util/common/Atmosphere.cc"

#include "gcp/util/common/Atmosphere.h"
#include "gcp/util/common/Debug.h"

#include "gcp/control/code/share/slalib/slalib.h"

#include <iomanip>

using namespace std;
using namespace gcp::util;

Wavelength Atmosphere::opticalWavelength_ = Wavelength(Length::Microns(), 0.57);
Angle      Atmosphere::refracAccuracy_    = Angle(Angle::MilliArcSec(), 10.0);

const double Atmosphere::tropoLapseRate_ = 0.0065;

/**.......................................................................
 * Constructor.
 */
Atmosphere::Atmosphere() 
{
  initialize();
}

void Atmosphere::initialize() 
{
  lacking_ = ALL;
}

/**.......................................................................
 * Destructor.
 */
Atmosphere::~Atmosphere() {}

/**.......................................................................
 * Fully-specified method for computing refraction coefficients
 */
Atmosphere::RefractionCoefficients
Atmosphere::refractionCoefficients(Length altitude, 
				   Temperature airTemp,
				   Pressure pressure, 
				   Percent humidity,
				   Wavelength wavelength,
				   Angle latitude,
				   double tropoLapseRate,
				   Angle accuracy)
{
  RefractionCoefficients coeff;

  slaRefco(altitude.meters(), airTemp.K(), pressure.milliBar(),
	   humidity.percentMax1(), wavelength.microns(), latitude.radians(), 
	   tropoLapseRate, accuracy.radians(), &coeff.a, &coeff.b);

  DBPRINT(true, Debug::DEBUG9, "Inside refractionCoefficients: parameters are: " << std::endl
	  << "Altitude   = " << std::setprecision(6) << altitude.meters()        << " m "      << std::endl
	  << "Air Temp   = " << std::setprecision(6) << airTemp.K()              << " K "      << std::endl
	  << "Pressure   = " << std::setprecision(6) << pressure.milliBar()      << " mBar"    << std::endl
	  << "Humidity   = " << std::setprecision(6) << humidity.percentMax100() << " %"       << std::endl
	  << "Wavelength = " << std::setprecision(6) << wavelength.microns()     << " microns" << std::endl
	  << "Latitude   = " << std::setprecision(6) << latitude.radians()       << " rad "    << std::endl
	  << "TLR        = " << std::setprecision(6) << tropoLapseRate           << " K/m "    << std::endl
	  << "Accuracy   = " << std::setprecision(6) << accuracy.mas()           << " mas "    << std::endl
	  << "Coeff A    = " << std::setprecision(6) << coeff.a                  << std::endl
	  << "Coeff B    = " << std::setprecision(6) << coeff.b                  << std::endl);
  
  return coeff;
}

/**.......................................................................
 * Fully un-specified versions
 */
Atmosphere::RefractionCoefficients
Atmosphere::refractionCoefficients()
{
  return refractionCoefficients(altitude_, airTemperature_, 
				pressure_, humidity_, 
				wavelength_, latitude_,
				tropoLapseRate_, 
				refracAccuracy_);
}

Atmosphere::RefractionCoefficients
Atmosphere::opticalRefractionCoefficients()
{
  return refractionCoefficients(altitude_, airTemperature_, 
				pressure_, humidity_, 
				opticalWavelength_, latitude_,
				tropoLapseRate_, 
				refracAccuracy_);
}

// Set methods

void Atmosphere::setAirTemperature(Temperature airTemp) 
{
  airTemperature_ = airTemp;
  lacking_ &= ~TEMP;
}

void Atmosphere::setPressure(Pressure pressure) 
{
  pressure_ = pressure;
  lacking_ &= ~PRESSURE;
}

void Atmosphere::setHumidity(Percent humidity) 
{
  humidity_ = humidity;
  lacking_ &= ~HUMIDITY;
}

void Atmosphere::setWavelength(Wavelength wavelength) 
{
  wavelength_ = wavelength;
  lacking_ &= ~WAVE;
}

void Atmosphere::setFrequency(Frequency frequency) 
{
  wavelength_ = Wavelength(frequency);
  lacking_   &= ~WAVE;
}

void Atmosphere::setAltitude(Length altitude) 
{
  altitude_ = altitude;
  lacking_ &= ~ALTITUDE;
}

void Atmosphere::setLatitude(Angle latitude) 
{
  latitude_ = latitude;
  lacking_ &= ~LATITUDE;
}

bool Atmosphere::canComputeRefraction() 
{
  return !(lacking_ & ALL);
}

bool Atmosphere::canComputeOpticalRefraction() 
{
  return !(lacking_ & ALLOPTICAL);
}
