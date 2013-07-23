#define __FILEPATH__ "util/common/Length.cc"

#include "gcp/util/common/Length.h"

#include <iomanip>

using namespace std;

using namespace gcp::util;

const unsigned Length::cmPerMeter_   = 100;
const unsigned Length::cmPerKm_      = 100000;
const unsigned Length::micronsPerCm_ = 10000;

const double Length::metersPerMile_  = 1609.344;
const double Length::cmPerMile_      = 160934.4;

/**.......................................................................
 * Constructor.
 */
Length::Length() 
{
  initialize();
}

/**.......................................................................
 * Copy constructor.
 */
Length::Length(const Length& length) 
{
  setCentimeters(length.centimeters());
}

Length::Length(const Centimeters& units, double cm)
{
  setCentimeters(cm);
}

Length::Length(const Meters& units, double m)
{
  setMeters(m);
}

/**.......................................................................
 * Destructor.
 */
Length::~Length() {}

/** .......................................................................
 * Add two Lengths
 */
Length Length::operator+(Length& length)
{
  Length sum;
  sum.setCentimeters(cm_ + length.centimeters());
  return sum;
}

/** .......................................................................
 * Subtract two Lengths
 */
Length Length::operator-(Length& length)
{
  Length diff;
  diff.setCentimeters(cm_ - length.centimeters());
  return diff;
}

/** .......................................................................
 * Divide two Lengths
 */
double Length::operator/(Length& length)
{
  return cm_/length.centimeters();
}

/** .......................................................................
 * Multiply a length by a constant
 */
Length Length::operator*(double multFac)
{
  Length mult;
  mult.setCentimeters(cm_ * multFac);
  return mult;
}

/**.......................................................................
 * Allows cout << Length
 */
std::ostream& operator<<(std::ostream& os, Length& length)
{
  os << setw(18) << setprecision(12) << length.centimeters() << " (cm)";
  return os;
}

void Length::initialize()
{
  setCentimeters(0.0);
}
