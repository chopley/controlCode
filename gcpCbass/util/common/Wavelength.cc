#define __FILEPATH__ "util/common/Wavelength.cc"

#include "gcp/util/common/Frequency.h"
#include "gcp/util/common/Wavelength.h"

using namespace std;
using namespace gcp::util;

Speed Wavelength::lightSpeed_ = 
Speed(Speed::CentimetersPerSec(), 2.99792458e10);

const double Wavelength::cmPerAngstrom_ = 1.0e-10;

/**.......................................................................
 * Constructor.
 */
Wavelength::Wavelength() {
  initialize();
}

Wavelength::Wavelength(const Frequency& frequency)
{
  if(frequency.Hz() > 0.0) 
    setCentimeters(lightSpeed_.centimetersPerSec() / frequency.Hz());
  else
    setFinite(false);
}

Wavelength::Wavelength(const Length::Centimeters& units, double cm) {
  setCentimeters(cm);
}

Wavelength::Wavelength(const Microns& units, double microns) {
  setMicrons(microns);
}

/**.......................................................................
 * Destructor.
 */
Wavelength::~Wavelength() {};

void Wavelength::setMicrons(double microns) 
{
  cm_ = microns/micronsPerCm_;
}

void Wavelength::setAngstroms(double angstroms) 
{
  cm_ = angstroms * cmPerAngstrom_;
}

double Wavelength::microns() 
{
  return cm_ * micronsPerCm_;
}

double Wavelength::angstroms() 
{
  return cm_ / cmPerAngstrom_;
}

void Wavelength::initialize()
{
  setFinite(true);
  Length::initialize();
}
