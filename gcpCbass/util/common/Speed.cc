#define __FILEPATH__ "util/common/Speed.cc"

#include "gcp/util/common/Length.h"
#include "gcp/util/common/Speed.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Speed::Speed() 
{
  initialize();
}

Speed::Speed(const CentimetersPerSec& units, double cmPerSec)
{
  setCentimetersPerSec(cmPerSec);
}

Speed::Speed(const MetersPerSec& units, double mPerSec)
{
  setMetersPerSec(mPerSec);
}

Speed::Speed(const MilesPerHour& units, double mPh)
{
  setMilesPerHour(mPh);
}

/**.......................................................................
 * Destructor.
 */
Speed::~Speed() {}

void Speed::setCentimetersPerSec(double cmPerSec)
{
  cmPerSec_ = cmPerSec;
}

void Speed::setMetersPerSec(double mPerSec)
{
  cmPerSec_ = mPerSec * Length::cmPerMeter_;
}

void Speed::setMilesPerHour(double mPh)
{
  cmPerSec_ = mPh * Length::cmPerMile_ / 3600;
}

double Speed::centimetersPerSec()
{
  return cmPerSec_;
}

double Speed::metersPerSec()
{
  return cmPerSec_ / Length::cmPerMeter_;
}

double Speed::mph()
{
  return milesPerHour();
}

double Speed::milesPerHour()
{
  return cmPerSec_ / Length::cmPerMile_;
}

void Speed::initialize()
{
  setCentimetersPerSec(0.0);
}
