#define __FILEPATH__ "util/common/Frequency.cc"

#include "gcp/util/common/Frequency.h"
#include "gcp/util/common/Wavelength.h"

#include <iomanip>
#include <cmath>

using namespace std;
using namespace gcp::util;

Speed Frequency::lightSpeed_ = 
Speed(Speed::CentimetersPerSec(), 2.99792458e10);

const double Frequency::HzPerGHz_   = 1e9;
const double Frequency::HzPerMHz_   = 1e6;

/**.......................................................................
 * Constructor.
 */
Frequency::Frequency() 
{
  initialize();
}

/**.......................................................................
 * Constructor.
 */
Frequency::Frequency(const Hertz& units, double Hz) 
{
  setHz(Hz);
}

/**.......................................................................
 * Constructor.
 */
Frequency::Frequency(const MegaHz& units, double MHz) 
{
  setMHz(MHz);
}

/**.......................................................................
 * Constructor.
 */
Frequency::Frequency(const GigaHz& units, double GHz) 
{
  setGHz(GHz);
}

/**.......................................................................
 * Constructor.
 */
Frequency::Frequency(Wavelength& wavelength) 
{
  setHz(lightSpeed_.centimetersPerSec() / wavelength.centimeters());
}

/**.......................................................................
 * Destructor.
 */
Frequency::~Frequency() {}

// Set the frequency, in GHz

void Frequency::setGHz(double GHz)
{
  setHz(GHz * HzPerGHz_);
}

// Set the frequency, in MHz

void Frequency::setMHz(double MHz)
{
  setHz(MHz * HzPerMHz_);
}

// Set the frequency, in Hz

void Frequency::setHz(double Hz)
{
  Hz_     = Hz;
  finite_ = finite(Hz);
}

/**.......................................................................
 * Allows cout << Frequency
 */
std::ostream& gcp::util::operator<<(std::ostream& os, Frequency& frequency)
{
  os << setw(18) << setprecision(12) << frequency.GHz() << " (GHz)";
  return os;
}

void Frequency::initialize()
{
  setHz(0.0);
}

/** .......................................................................
 * Subtract two Frequencys
 */
Frequency Frequency::operator-(Frequency& frequency)
{
  Frequency diff;
  diff.setHz(Hz_ - frequency.Hz());
  return diff;
}
