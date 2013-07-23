#define __FILEPATH__ "antenna/control/specific/Pointing.cc"

#include <cstring>
#include <cmath>

#include "gcp/antenna/control/specific/Pointing.h"

#include "gcp/control/code/unix/libunix_src/common/const.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor just calls reset()
 */
Pointing::Pointing()
{
  reset();
}

/**.......................................................................
 * Initialize data members to something sensible
 */
void Pointing::reset()
{
  srcName_[0]  = '\0';
  scanName_[0] = '\0';
  mjd_        = 0;
  sec_        = 0;
  ra_         = 0.0;   // Zero the pointing flow parameters. (these are
		    // unknown for slews)
  dec_        = 0.0;
  dist_       = 0.0;
  refraction_ = 0.0;

  geocentric_.reset();
  topocentric_.reset();
  
  // pmac_new_position() will fill in the position fields with the
  // reported location of the telescope.

  mountAngles_.reset();
  
  // We want the drives to stop when they reach the target position.

  mountRates_.reset();
  
  // Tell pmac_new_position() not to move any of the axes.

  axes_ = Axis::NONE;
  // Default to a normal source

  isCenter_ = false;
}

/**.......................................................................
 * Install the name of the source
 */
void Pointing::setSrcName(char* name)
{
  strncpy(srcName_, name, SRC_LEN);
  srcName_[SRC_LEN-1] = '\0';
}

/**.......................................................................
 * Return the name of the source
 */
unsigned char* Pointing::getSrcName()
{
  return (unsigned char*)srcName_;
}

/**.......................................................................
 * Install the name of the scan
 */
void Pointing::setScanName(char* name)
{
  strncpy(scanName_, name, SCAN_LEN);
  scanName_[SCAN_LEN-1] = '\0';
}

/**.......................................................................
 * Return the name of the scan
 */
unsigned char* Pointing::getScanName()
{
  return (unsigned char*)scanName_;
}

/**.......................................................................
 * Record the current time as days and seconds of UTC
 */
void Pointing::setTime(double utc)
{
  // using round because we're near the second boundary anyway
  mjd_ = static_cast<int>(floor(utc));
  sec_ = static_cast<int>(round((utc - mjd_) * daysec));
}

/**.......................................................................
 * Record the current time as days and seconds of UTC
 */
void Pointing::setTime(int mjd, int sec)
{
  mjd_ = mjd;
  sec_ = sec;
}

/**.......................................................................
 * Install the angles to which the axes will be driven.
 */
void Pointing::setAngles(double az, double el, double pa)
{
  mountAngles_.az_ = az;
  mountAngles_.el_ = el;
  mountAngles_.pa_ = pa;
  saveAngles_.az_ = az;
  saveAngles_.el_ = el;
  saveAngles_.pa_ = pa;
}

/**.......................................................................
 * Install the angles to which the axes will be driven.
 */
void Pointing::resetAngles()
{
  mountAngles_.az_ = saveAngles_.az_;
  mountAngles_.el_ = saveAngles_.el_;
  mountAngles_.pa_ = saveAngles_.pa_;
}

/**.......................................................................
 * Install the rates with which the axes will be driven.
 */
void Pointing::setRates(double az, double el, double pa)
{
  mountRates_.az_ = az;
  mountRates_.el_ = el;
  mountRates_.pa_ = pa;
}

/**.......................................................................
 * Install the set of axes to drive.
 */
void Pointing::setAxes(Axis::Type axes)
{
  axes_ = axes;
}

/**.......................................................................
 * Return the set of axes to drive.
 */
Axis::Type Pointing::getAxes()
{
  return axes_;
}

/**.......................................................................
 * Set the RA of the source.
 */
void Pointing::setRa(double ra)
{
  ra_ = ra;
}

/**.......................................................................
 * Set the DEC of the source.
 */
void Pointing::setDec(double dec)
{
  dec_ = dec;
}

/**.......................................................................
 * Set the distance to the source.
 */
void Pointing::setDist(double dist)
{
  dist_ = dist;
}

/**.......................................................................
 * Set the refraction correction.
 */
void Pointing::setRefraction(double refraction)
{ 
  refraction_ = refraction;
}

/**
 * A public method to convert from mount angle to encoder counts
 */
void Pointing::convertMountToEncoder(Encoder* encoder, 
				     PmacAxis* axis,
				     int current)
{
  switch (encoder->getAxis()) {
  case Axis::AZ:
    encoder->convertMountToEncoder(mountAngles_.az_, mountRates_.az_, axis, 
				   current);
    break;
  case Axis::EL:
    encoder->convertMountToEncoder(mountAngles_.el_, mountRates_.el_, axis,
				   current);
    break;
  case Axis::PA:
    encoder->convertMountToEncoder(mountAngles_.pa_, mountRates_.pa_, axis,
				   current);
    break;
  default:
    ErrorDef(err, "Pointing::convertMountToEncoder: Unrecognized axis.\n");
    break;
  }
}

/**
 * A public method to apply the mount angle limits
 */
void Pointing::convertMountToEncoder(Encoder* encoder, 
				     PmacAxis* axis, double current)
{
  switch (encoder->getAxis()) {
  case Axis::AZ:
    encoder->applyLimits(mountAngles_.az_, mountRates_.az_, axis, current);
    break;
  case Axis::EL:
    encoder->applyLimits(mountAngles_.el_, mountRates_.el_, axis, current);
    break;
  case Axis::PA:
    encoder->applyLimits(mountAngles_.pa_, mountRates_.pa_, axis, current);
    break;
  default:
    ErrorDef(err, "Pointing::convertMountToEncoder: Unrecognized axis.\n");
    break;
  }
}

/**.......................................................................
 * Return true if the axis mask includes the requested axis
 */
bool Pointing::includesAxis(Axis::Type axis)
{
  return axes_ & axis;
}

/**.......................................................................
 * Return a pointer to the requested Position object
 */
gcp::antenna::control::Position* Pointing::Position(PositionType type)
{
  switch (type) {
  case Pointing::MOUNT_ANGLES:
    return &mountAngles_;
    break;
  case Pointing::MOUNT_RATES:
    return &mountRates_;
    break;
  case Pointing::TOPOCENTRIC:
    return &topocentric_;
    break;
  case Pointing::GEOCENTRIC:
    return &geocentric_;
    break;
  default:
    throw Error("Pointing::Position: Unrecognized position type.\n");
    break;
  }
}

/*.......................................................................
 * Compute the geocentric azimuth and elevation of the source.
 *
 * Input:
 *  lst      double   The local apparent sidereal time.
 *
 * Input/Output:
 *  f   PointingCorrections * On input the sin_lat and cos_lat members must have
 *                    been initialized. On output the az,el,sin_az,cos_az,
 *                    sin_el,cos_el terms will have been initialized.
 */
void Pointing::computeGeocentricPosition(double lst, PointingCorrections *f)
{
  // Compute the hour angle of the source.

  double ha = lst - ra_;
  double sin_ha = sin(ha);
  double cos_ha = cos(ha);

  // Precompute trig terms.

  double cos_dec = cos(dec_);
  double sin_dec = sin(dec_);

  // Evaluate pertinent spherical trig equations.

  double cos_el_cos_az = sin_dec * f->cos_lat - cos_dec * f->sin_lat * cos_ha;
  double cos_el_sin_az = -cos_dec * sin_ha;
  double cos2_el=cos_el_cos_az * cos_el_cos_az + cos_el_sin_az * cos_el_sin_az;
  double cos_el_sin_pa = sin_ha * f->cos_lat;
  double cos_el_cos_pa = f->sin_lat * cos_dec - sin_dec * f->cos_lat * cos_ha;

  // Get the geocentric elevation and its trig forms from the above equations.

  f->cos_el = sqrt(cos2_el);
  f->sin_el = sin_dec * f->sin_lat + cos_dec * f->cos_lat * cos_ha;
  f->el = asin(f->sin_el);

  // Compute the geocentric azimuth.

  f->az = (cos_el_cos_az==0.0 && cos_el_sin_az==0.0) ?
    0.0 : wrap2pi(atan2(cos_el_sin_az, cos_el_cos_az));
  f->sin_az = sin(f->az);
  f->cos_az = cos(f->az);

  // Compute the parallactic angle.

  f->pa = (cos_el_cos_pa==0.0 && cos_el_sin_pa==0.0) ?
    0.0 : wrapPi(atan2(cos_el_sin_pa, cos_el_cos_pa));
  
  // Install these in our geocentric container.

  geocentric_.set(Axis::AZ, f->az);
  geocentric_.set(Axis::EL, f->el);
  geocentric_.set(Axis::PA, f->pa);
}

/**.......................................................................
 * Pack the UTC for archival into the register database
 */
void Pointing::packUtc(unsigned* u_elements) 
{
  u_elements[0] = mjd_;
  u_elements[1] = sec_ * 1000;
}

/**.......................................................................
 * Return the current UTC.
 */
double Pointing::getUtc()
{
  return static_cast<double>(mjd_) + static_cast<double>(sec_) / daysec;
}

/**.......................................................................
 * Return the current UTC.
 */
RegDate Pointing::getDate()
{
  RegDate date(mjd_, sec_ * 1000);
  return date;
}

/**.......................................................................
 * Return the applied refraction correction.
 */
double Pointing::getRefraction()
{
  return refraction_;
}

/**.......................................................................
 * Pack the source name for archival in the register database.
 */
void Pointing::packSourceName(unsigned* u_elements, int nel)
{
  if(pack_int_string(srcName_, nel, u_elements))
    throw Error("Pointing::packSourceName: Error in pack_int_string().\n");
}

/**.......................................................................
 * Pack the scan name for archival in the register database.
 */
void Pointing::packScanName(unsigned* u_elements, int nel)
{
  if(pack_int_string(scanName_, nel, u_elements))
    throw Error("Pointing::packScanName: Error in pack_int_string().\n");
}

/**.......................................................................
 * Pack the equatorial geocentric position.
 */
void Pointing::packEquatGeoc(signed* s_elements)
{
  s_elements[0] = static_cast<signed>(ra_   * rtomas);
  s_elements[1] = static_cast<signed>(dec_  * rtomas);
  s_elements[2] = static_cast<signed>(dist_ * 1.0e6);  // Micro-AU
}

/**.......................................................................
 * Pack geocentric horizon coordinates.
 */
void Pointing::packHorizGeoc(signed* s_elements)
{
  geocentric_.pack(s_elements);
}

/**.......................................................................
 * Pack topocentric horizon coordinates.
 */
void Pointing::packHorizTopo(signed* s_elements)
{
  topocentric_.pack(s_elements);
}

/**.......................................................................
 * Pack horizon mount coordinates.
 */
void Pointing::packHorizMount(double* array)
{
  mountAngles_.pack(array);
}

/**.......................................................................
 * Pack horizon mount coordinates.
 */
void Pointing::packHorizMount(signed* s_elements)
{
  mountAngles_.pack(s_elements);
}

/**.......................................................................
 * Set up pointing for a new halt command
 */
void Pointing::setupForHalt(SpecificShare* share)
{
  // Reset all pointing parameters

  reset();

  // Record a blank source name

  setSrcName("(none)");
  setScanName("(none)");

  isCenter_ = true;
  // Give the slew the timestamp of the next 1-second boundary. This
  // is when it will be submitted to the pmac.

  double utc = share->getUtc();
  setTime(utc);
}
/**.......................................................................
 * Set up pointing in preparation for a pmac reboot
 */
void Pointing::setupForReboot(SpecificShare* share)
{
  // Reset all pointing parameters

  reset();
  
  // Record a blank source name.

  setSrcName("(none)");
  setScanName("(none)");
  
  // Give the slew the timestamp of the next 1-second boundary. This
  // is when it will be submitted to the pmac.
  
  double  utc = share->getUtc();

  setTime(utc);
}
/**.......................................................................
 * Set up pointing for a new track
 */
void Pointing::setupForTrack()
{
  isCenter_ = false;
}

/**.......................................................................
 * Set up pointing for a new slew command
 */
void Pointing::setupForSlew(SpecificShare* share, TrackerMsg* msg)
{
  
  // Zero the pointing flow parameters. These are unknown for slews.

  reset();
  
  // Record the source name.

  setSrcName(msg->body.slew.source);
  
  // Give the slew the timestamp of the next 1-second boundary. This
  // is when it will be submitted to the pmac.

  double utc = share->getUtc();
  setTime(utc);
  
  // Record the new positions.

  setAngles(msg->body.slew.az, msg->body.slew.el, 
	    msg->body.slew.pa);
  
  // We want the drives to stop when they reach the target position.

  setRates(0.0, 0.0, 0.0);
  
  // Which axes are to be slewed?
  
  setAxes(msg->body.slew.axes);
  // If this was a center source, mark it so

  isCenter_ = true;
}

/**.......................................................................
 * Round an angle into the range -pi..pi.
 *
 * Input:
 *  angle    double   The angle to be rounded (radians).
 *
 * Output:
 *  return   double   An angle in the range:  -pi <= angle < pi.
 */
double Pointing::wrapPi(double angle)
{
  double a = wrap2pi(angle);
  return a < pi ? a : (a-twopi);
}

/**.......................................................................
 * Round an angle into the range 0-2.pi. Note that fmod() is not used
 * because it was found that the version of fmod() that comes with the
 * 68060 and other Motorola CPU's is implemented as a while loop which
 * takes seconds (!) to finish if the divisor is much smaller than the
 * numerator.
 *
 * Input:
 *  angle    double   The angle to be rounded (radians).
 * Output:
 *  return   double   An angle in the range:  0 <= angle < 2.pi.
 */
double Pointing::wrap2pi(double angle)
{
  return (angle >= 0.0) ? (angle - twopi * floor(angle/twopi)) :
			  (angle + twopi * ceil(-angle/twopi));
}
