#define __FILEPATH__ "util/common/Source.cc"

#include <cstring>

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/Source.h"

using namespace std;
using namespace gcp::util;

#define CHECK_RADEC() \
  if(!isRaDec()) {\
    ThrowError("Source " << name_ << " is not an Ra/Dec source");\
  };

#define CHECK_AZEL() \
  if(!isAzEl()) {\
    ThrowError("Source " << name_ << " is not an Az/El source");\
  };

/**.......................................................................
 * Constructor.
 */
Source::Source()
{
  ra_   = 0;
  dec_  = 0;
  dist_ = 0;

  ra_   = new QuadraticInterpolatorPositiveAngle(0.0);
  dec_  = new QuadraticInterpolatorSignedAngle(0.0);
  dist_ = new QuadraticInterpolatorNormal(0.0);

  // And reset all of our members.

  reset();
}

/**.......................................................................
 * Destructor for class Source
 */
Source::~Source()
{
  // Initialize the source type to something unknown

  type_ = gcp::control::SRC_UNKNOWN;

  // Delete resources allocated by the constructor

  if(ra_ != 0)
    delete ra_;

  if(dec_ != 0)
    delete dec_;

  if(dist_ != 0)
    delete dist_;
}

/**.......................................................................
 * Reset the source
 */
void Source::reset()
{
  name_[0] = '\0';

  type_ = gcp::control::SRC_UNKNOWN;

  if(ra_)
    ra_->empty();

  if(dec_)
    dec_->empty();

  if(dist_)
    dist_->empty();
}

/**.......................................................................
 * Extend the ephemeris of this source
 */
void Source::extend(double mjd, HourAngle ra, DecAngle dec, double dist)
{
  if(ra_)
    ra_->extend(mjd, ra.radians());

  if(dec_)
    dec_->extend(mjd, dec.radians());

  if(dist_)
    dist_->extend(mjd, dist);
}

/**.......................................................................
 * Set the source name
 */
void Source::setName(char* name)
{
  strncpy(name_, name, SRC_LEN);
  name_[SRC_LEN-1] = '\0';
}

/**.......................................................................
 * Return a pointer to the source name
 */
char* Source::getName()
{
  return name_;
}

/**.......................................................................
 * Return the Az of this source
 */
Angle Source::getAz()
{
  CHECK_AZEL();
  return az_;
}

/**.......................................................................
 * Return the El of this source
 */
Angle Source::getEl()
{
  CHECK_AZEL();
  return el_;
}

/**.......................................................................
 * Return the interpolated RA of this source.
 */
HourAngle Source::getRa(double tt)
{
  CHECK_RADEC();
  HourAngle ra;
  ra.setRadians(ra_->evaluate(tt));
  return ra;
}

/**.......................................................................
 * Return the interpolated DEC of this source.
 */
DecAngle Source::getDec(double tt)
{
  CHECK_RADEC();
  DecAngle dec;
  dec.setRadians(dec_->evaluate(tt));
  return dec;
}

/**.......................................................................
 * Return the interpolated distance of this source.
 */
double Source::getDist(double tt)
{
  return dist_->evaluate(tt);
}

/**.......................................................................
 * Return the gradient of the RA of this source.
 */
HourAngle Source::getGradRa(double tt)
{
  CHECK_RADEC();
  HourAngle ra;
  ra.setRadians(ra_->gradient(tt));
  return ra;
}

/**.......................................................................
 * Return the gradient of the DEC of this source.
 */
DecAngle Source::getGradDec(double tt)
{
  CHECK_RADEC();
  DecAngle dec;
  dec.setRadians(dec_->gradient(tt));
  return dec;
}

/**.......................................................................
 * Set axes for a source
 */
void Source::setAxis(gcp::util::Axis::Type axisMask, Angle az, Angle el, Angle pa)
{
  gcp::util::Axis axis(axisMask);

  if(axis.isSet(gcp::util::Axis::AZ))
    az_ = az;
  if(axis.isSet(gcp::util::Axis::EL))
    el_ = el;
  if(axis.isSet(gcp::util::Axis::PA))
    pa_ = pa;
}

/**.......................................................................
 * Return true if this is an Az/El source
 */
bool Source::isAzEl()
{
  return (type_ == gcp::control::SRC_FIXED);
}

/**.......................................................................
 * Return true if this is an Ra/Dec source
 */
bool Source::isRaDec()
{
  return (type_ == gcp::control::SRC_EPHEM || type_ == gcp::control::SRC_J2000);
}

/**.......................................................................
 * Return true if this is an J2000 source
 */
bool Source::isJ2000()
{
  return type_ == gcp::control::SRC_J2000;
}

/**.......................................................................
 * Return true if this is an J2000 source
 */
bool Source::isEphem()
{
  return type_ == gcp::control::SRC_EPHEM;
}

/**.......................................................................
 * Return true if source parameters can be bracketed for this
 * timestamp
 */
bool Source::canBracket(double mjd)
{
  return (isAzEl() || 
	  (isJ2000() && ra_->getNpt() > 0 && dec_->getNpt() > 0) ||
	  (ra_->canBracket(mjd) && dec_->canBracket(mjd) && dist_->canBracket(mjd)));
}

