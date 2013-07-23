#define __FILEPATH__ "antenna/control/common/Encoder.cc"

#include <iostream>
#include <cmath>
#include <climits>

#include "gcp/antenna/control/specific/Encoder.h"
#include "gcp/antenna/control/specific/PmacAxis.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"

#include "gcp/control/code/unix/libunix_src/common/const.h"

#define PI_VAL 3.14159265359
using namespace gcp::antenna::control;
using namespace gcp::util;
using namespace std;

/**.......................................................................
 * Constructor sets the axis this object represents
 */
Encoder::Encoder(Axis::Type axis)
{
  axis_ = axis;
  reset();
}

/**.......................................................................
 * Reset internal members of this object
 */
void Encoder::reset()
{
  zero_            = 0.0;
  countsPerRadian_ = 0.0;
  countsPerTurn_   = 0;
  min_             = 0;
  max_             = 0;
  mountMin_        = 0;
  mountMax_        = 0;
  slewRate_        = 100;  // Full speed = 100% 
}

/**.......................................................................
 * Set the encoder count at the zero point of this axis
 */
void Encoder::setZero(double zero)
{
  zero_ = zero;
}

/**.......................................................................
 * Set the slew rate for this axis
 */
void Encoder::setSlewRate(long rate)
{
  slewRate_ = rate;
}

/**.......................................................................
 * Convert from encoder counts to radians on the sky
 */
double Encoder::convertCountsToSky(int count)
{
  return count / countsPerRadian_ - zero_;
}

double Encoder::convertAngleToSky(Angle& angle)
{
  return angle.radians() - zero_;
}

Angle Encoder::convertSkyToAngle(Angle sky)
{
  return Angle(Angle::Radians(), sky.radians() + zero_);
}

/**....................................................................... 
 * Set the counts per radian for this encoder.
 */
void Encoder::setCountsPerRadian(double countsPerRadian)
{
  countsPerRadian_ = countsPerRadian;
}

/**.......................................................................
 * Set the counts per turn for this encoder.
 */
void Encoder::setCountsPerTurn(int countsPerTurn)
{
  countsPerTurn_ = countsPerTurn;
}

/**.......................................................................
 * Update the mount limits.
 */
void Encoder::updateMountLimits()
{
  double emin, emax;

  //  emin = convertCountsToSky(min_);
  // emax = convertCountsToSky(max_);

  emin = minLim_.radians();
  emax = maxLim_.radians();

  //  COUT("setting limits to : " << emin << " , " << emax);

  // Record the new azimuth limits, noting that the min encoder value
  // will correspond to the maximum mount angle if the two coordinate
  // systems increase in opposite directions.

  if(emin < emax) {
    mountMin_ = emin;
    mountMax_ = emax;
  } else {
    mountMin_ = emax;
    mountMax_ = emin;
  };
}

/**.......................................................................
 * Install new encoder (count) limits
 */
void Encoder::setLimits(long min, long max)
{
  // Enforce min < max.

  //  COUT("setting limits to : " << min << " , " << max);

  if(min <= max) {
    min_ = min;
    max_ = max;
  } else {
    min_ = max;
    max_ = min;
  };
}


/**.......................................................................
 * Install new encoder (count) limits
 */
void Encoder::setLimits(gcp::util::Angle& min, gcp::util::Angle& max)
{
  // Enforce min < max.

  //  COUT("setting (angle) limits to : " << min.degrees() << " , " << max.degrees());

  if(min.degrees() <= max.degrees()) {
    minLim_ = min;
    maxLim_ = max;
  } else {
    minLim_ = max;
    maxLim_ = min;
  };
}

/**
 * Return the axis type of this encoder
 */
Axis::Type Encoder::getAxis()
{
  return axis_;
}

double Encoder::getMountMin()
{
  return mountMin_;
}

double Encoder::getMountMax()
{
  return mountMax_;
}

signed Encoder::getSlewRate()
{
  return slewRate_;
}

/**.......................................................................
 * Convert from Mount coordinates (radians, radians/sec) to encoder
 * counts and rates.
 */
void Encoder::convertMountToEncoder(double angle, double rate, 
				    PmacAxis* axis,
				    int current)
{
  signed long enc_rate, enc_count;
  LogStream errStr;

  // Don't proceed if the encoder calibration is not present.  This
  // will cause an arithmetic exception below if it is zero.

  if(countsPerTurn_ == 0) {
    errStr.appendMessage(true, "Encoder counts per turn is 0.");
    throw Error(errStr);
  }

  // Convert the specified rate to encoder units * 1000.

  rate *= floor(countsPerRadian_ * 1000.0 + 0.5);
  /*
   * Limit the rate to the range of a signed long. Note that I use
   * -LONG_MAX in place of LONG_MIN because the VxWorks definition of
   * LONG_MIN is currently broken. It is defined as (-2147483648),
   * which translates to a unary minus operator followed by a number
   * that is > LONG_MAX, so gcc complains that it has to represent it
   * as an unsigned long, and that definately isn't what I want!
   */
  if(rate < -LONG_MAX)
    enc_rate = -LONG_MAX;
  else if(rate > LONG_MAX)
    enc_rate = LONG_MAX;
  else
    enc_rate = (long) rate;

  // Add in user-specified tracking offsets to the target position and
  // convert to encoder units.

  enc_count = static_cast<long>
    (floor((zero_ + angle) * countsPerRadian_ + 0.5));

  // Is the encoder limited to less than a turn?

  if(max_ != min_) {
  
    if(max_ - min_ < countsPerTurn_) {
      
      // Adjust the preliminary encoder count to within 1 turn forward
      // of the minimum available encoder position.

      int delta = enc_count - min_;
      if(delta < 0)
	enc_count += countsPerTurn_ * (-delta / countsPerTurn_ + 1);
      else if(delta > countsPerTurn_)
	enc_count -= countsPerTurn_ * (delta / countsPerTurn_);

      // If the resulting encoder position is not in the available
      // range, substitute the count of the nearest encoder limit.

      if(enc_count > max_) {
	if(enc_count - max_ < min_ - (enc_count - countsPerTurn_))
	  enc_count = max_;
	else
	  enc_count = min_;
      };
    } else {

      /**
       * If the encoder limits cover a turn or more, then the requested
       * position can always be reached. In fact, because of the overlap
       * regions of the drive, a given angle may correspond to more than one
       * encoder position. We want to choose the one that is closest to the
       * current position of the drive. Start by determining the distance
       * between our preliminary encoder position and the current
       * encoder position on this axis.
       */
      int delta = enc_count - current;

      // Adjust the preliminary encoder count to within 1 turn forward
      // of the current encoder position.

      if(delta < 0)
	enc_count += countsPerTurn_ * (-delta / countsPerTurn_ + 1);
      else if(delta > countsPerTurn_)
	enc_count -= countsPerTurn_ * (delta / countsPerTurn_);

      // If the equivalent encoder count behind the current encoder
      // position is closer to the current encoder position, use it
      // instead.

      if(enc_count - current > countsPerTurn_/2)
	enc_count -= countsPerTurn_;

      // If the encoder count exceeds one of the limits, adjust it by a turn.

      if(enc_count < min_)
	enc_count += countsPerTurn_;
      else if(enc_count > max_)
	enc_count -= countsPerTurn_;
    };
  }

  // Install the newly computed values in the axis object

  axis->setRate(enc_rate);
  axis->setCount(enc_count);
  axis->setRadians(angle);
}

/**.......................................................................
 * Apply Limits 
 */
void Encoder::applyLimits(double angle, double rate, 
			  PmacAxis* axis, double current)
{

  LogStream errStr;

  // Don't proceed if the encoder calibration is not present.  This
  // will cause an arithmetic exception below if it is zero.

  if(countsPerTurn_ == 0) {
    errStr.appendMessage(true, "Encoder counts per turn is 0.");
    throw Error(errStr);
  }
  
  // Add in user-specified tracking offsets to the target position and
  // convert to encoder units.

  angle = (zero_ + angle);

  // Now we just need to get the limits for this axis.
  double minLim = getMountMin();
  double maxLim = getMountMax();

  // here's where we check whether we need to do a big round or just go negative/positive.
  if( ((current - angle) < -PI_VAL) && ((angle-2*PI_VAL) > minLim) ){
    angle = angle - 2*PI_VAL;
  } else if( ((current - angle) > PI_VAL) && ((angle+2*PI_VAL) < maxLim) ) {
    angle = angle + 2*PI_VAL;
  } else {
    angle = angle;
  };

  if(angle < minLim){
    angle = minLim;
  } else if (angle > maxLim) {
    angle = maxLim;
  };

    
  // Install the newly computed values in the axis object
  
  axis->setRate(0);
  axis->setCount(0);
  axis->setRadians(angle);
}

/**.......................................................................
 * Pack encoder limits for archival in the register database
 */
void Encoder::packLimits(signed* s_elements)
{
  s_elements[0] = static_cast<signed>(mountMin_ * rtomas);
  s_elements[1] = static_cast<signed>(mountMax_ * rtomas);
}

/**.......................................................................
 * Pack encoder zero points for archival in the register database.
 */
void Encoder::packZero(signed* s_elements)
{
  *s_elements = static_cast<signed>(zero_ * rtomas);
}

/**.......................................................................
 * Pack this encoder multiplier for archival in the register database.
 */
void Encoder::packCountsPerTurn(signed* s_elements)
{
  *s_elements = static_cast<signed>(countsPerTurn_);
}
