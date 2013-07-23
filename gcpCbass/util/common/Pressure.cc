#include "gcp/util/common/Pressure.h"

using namespace std;

using namespace gcp::util;

const double Pressure::mBarPerInHg_    = 33.8639;

/**.......................................................................
 * Constructor.
 */
Pressure::Pressure() 
{
  setMilliBar(0.0);
}

Pressure::Pressure(const InchesHg& unit, double inchesHg)
{
  setInchesHg(inchesHg);
}

Pressure::Pressure(const MilliBar& unit, double mBar)
{
  setMilliBar(mBar);
}

Pressure::Pressure(const Torr& unit, double torr)
{
  setTorr(torr);
}

/**.......................................................................
 * Destructor.
 */
Pressure::~Pressure() {}

void Pressure::setMilliBar(double mBar)
{
  mBar_ = mBar;
}

void Pressure::setInHg(double inHg)
{
  setInchesHg(inHg);
}

void Pressure::setInchesHg(double inchesHg)
{
  mBar_ = inchesHg * Pressure::mBarPerInHg_;
}

void Pressure::setTorr(double torr)
{
  setInchesHg(torr);
}

double Pressure::milliBar()
{
  return mBar_;
}

double Pressure::inchesHg()
{
  return mBar_ / Pressure::mBarPerInHg_;
}

double Pressure::inHg()
{
  return inchesHg();
}

double Pressure::torr()
{
  return inchesHg();
}

/**.......................................................................
 * Write the contents of this object to an ostream
 */
ostream& 
gcp::util::operator<<(ostream& os, Pressure& pressure)
{
  os << pressure.milliBar() << " mBar";
  return os;
}
