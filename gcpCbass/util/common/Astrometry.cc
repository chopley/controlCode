#define __FILEPATH__ "util/common/Astrometry.cc"

#include "gcp/util/common/Astrometry.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/QuadraticInterpolatorNormal.h"

#include "gcp/control/code/share/slalib/slalib.h"

using namespace gcp::util;
using namespace std;

// A macro for checking input values

#define CHECK_MJD(mjd) \
 {\
   if(mjd < 0) { \
     LogStream errStr; \
     errStr.initMessage(true); \
     errStr << "Illegal mjd: " << mjd << endl; \
     throw Error(errStr); \
   }; \
 }

// Some constants used by this class

const double Astrometry::secondsPerDay_ = 86400;
const double Astrometry::pi_            = M_PI;
const double Astrometry::twopi_         = 2*M_PI;

/**.......................................................................
 * Constructor.
 */
Astrometry::Astrometry() 
{
  ut1Utc_ = 0;
  eqnEqx_ = 0;
  
  ut1Utc_ = new QuadraticInterpolatorNormal(0.0);
  eqnEqx_ = new QuadraticInterpolatorNormal(0.0);
}

/**.......................................................................
 * Destructor.
 */
Astrometry::~Astrometry() 
{
  if(ut1Utc_ != 0) {
    delete ut1Utc_;
    ut1Utc_ = 0;
  }
  
  if(eqnEqx_ != 0) {
    delete eqnEqx_;
    eqnEqx_ = 0;
  }
}

/**.......................................................................
 * Extend the quadratic interpolation table of ut1 - utc
 * versus MJD UTC.
 *
 * @throws Exception
 */
void Astrometry::extendUt1Utc(double mjd, double ut1Utc)
{
  ut1Utc_->extend(mjd, ut1Utc);
}

/**.......................................................................
 * Extend the quadratic interpolation table of the equation
 * of the equinoxes versus Terrestrial Time (as a Modified
 * Julian Date).
 *
 * @throws Exception
 */
void Astrometry::extendEqnEqx(double tt, double eqnEqx)
{
  eqnEqx_->extend(tt, eqnEqx);
}

/**.......................................................................
 * Get the value of UT1-UTC for a given UTC.
 *
 * @throws Exception
 */
double Astrometry::getUt1Utc(double mjdUtc)
{
  if(ut1Utc_->canBracket(mjdUtc))
    return ut1Utc_->evaluate(mjdUtc);
  else {
    LogStream errStr;
    errStr.initMessage(true);
    errStr << "UT1-UTC ephemeris can't bracket mjd = " << mjdUtc << endl;
    throw Error(errStr);
  }
}

/**.......................................................................
 * Get the value of the equation of the equinoxes for a
 * given terrestrial time.
 *
 * @throws Exception
 */
double Astrometry::getEqnEqx(double mjdTt)
{
  if(eqnEqx_->canBracket(mjdTt))
    return eqnEqx_->evaluate(mjdTt);
  else {
    LogStream errStr;
    errStr.initMessage(true);
    errStr << "EqnEqx ephemeris can't bracket mjd = " << mjdTt << endl;
    throw Error(errStr);
  }
}

/**.......................................................................
 * Return true if ephemeris parameters can be interpolated for
 * this timestamp
 */
bool Astrometry::canBracket(double mjdUtc)
{
  return ut1Utc_->canBracket(mjdUtc) && 
    eqnEqx_->canBracket(mjdUtcToMjdTt(mjdUtc));
}


/**......................................................................
 * Return the local sidereal time for a given site and UTC.
 *
 * Input:
 *  utc     double    The current date and time (UTC), expressed as a
 *                    Modified Julian Date.
 *  longitude double  The longitude at which the lst is desired
 *  ut1Utc   double    The current value of UT1-UTC. If you don't need
 *                    more than one second of accuracy, this can be
 *                    given as zero.
 *  eqnEqx  double    The current value of the equation of the
 *                    equinoxes if you want apparent sidereal time,
 *                    or 0 if you can make do with mean sidereal time.
 *                    The equation of the equinoxes is a slowly varying
 *                    number that is computationally intensive to calculate,
 *                    so it doesn't make sense to calculate it anew on
 *                    each call.
 * Output:
 *  return  double    The local sidereal time, expressed in radians.
 */
HourAngle Astrometry::mjdUtcToLst(double mjd, Angle longitude, double ut1Utc, 
				  double eqnEqx)
{
  HourAngle lst;     // The local sidereal time 
  LogStream errStr;

  // Check arguments.

  CHECK_MJD(mjd);

  // Determine the local apparent (mean if eqnEqx is 0) sidereal time.

  lst.setRadians(slaGmst(mjd + ut1Utc/secondsPerDay_) + eqnEqx + 
		 longitude.radians());

  // The addition of the longitude and the equation of the equinoxes
  // may have caused lst to go outside the range 0-2.pi. Correct this.

  if(lst.radians() < 0)
    lst.addRadians(twopi_);
  else if(lst.radians() > twopi_)
    lst.addRadians(-twopi_);

  return lst;
}

/**.......................................................................
 * Same as above, using internal ephemerides
 */
HourAngle Astrometry::mjdUtcToLst(double mjdUtc, Angle longitude)
{
  return mjdUtcToLst(mjdUtc, longitude, 
		     ut1Utc_->evaluate(mjdUtc),
		     eqnEqx_->evaluate(mjdUtcToMjdTt(mjdUtc)));
}

/**......................................................................
 * Return the Terestrial time (aka Ephemeris Time), corresponding to a
 * given UTC (expressed as a Modified Julian date).
 *
 * Input:
 *
 *  mjd      double   The Modified Julian date.
 *
 * Output:
 *
 *  return   double   The corresponding Terrestrial Time
 */
double Astrometry::mjdUtcToMjdTt(double mjdUtc)
{
  CHECK_MJD(mjdUtc);

  return mjdUtc + slaDtt(mjdUtc) / secondsPerDay_;
}

Astrometry::Date Astrometry::mjdUtcToCalendarDate(double mjdUtc)
{
  double frc;     /* The unused fraction of a day returned by slaDjcl() */
  double integer; /* The integral part of a number */
  int status;     /* The status return value of slaDjcl() */

  Date date;

  // Perform the conversion.

  slaDjcl(mjdUtc, &date.year_, &date.month_, &date.day_, &frc, &status);
  
  // Check for errors.

  switch(status) {
  case 0:        /* No error */
    break;
  case 1:        /* Invalid mjd */
    ThrowError("MJD before 4701BC March 1");
    break;
  };
  
  // Fill in the hours minutes and seconds fields.

  frc = modf(frc * 24, &integer);

  date.hour_ = (unsigned int)integer;

  frc = modf(frc * 60, &integer);

  date.min_ = (unsigned int)integer;

  frc = modf(frc * 60, &integer);

  date.sec_ = (unsigned int)integer;
  date.nsec_ = (unsigned int)(frc * 1000000000U);

  return date;
}
