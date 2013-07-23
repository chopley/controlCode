#define __FILEPATH__ "util/common/Location.cc"

#include "gcp/util/common/Location.h"
#include "gcp/util/common/Coordinates.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/DelayLocation.h"
#include "gcp/util/common/Source.h"

#include "gcp/control/code/unix/libunix_src/common/const.h"

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Location::Location() 
{
  lacking_ = ALL;
  changed_ = true;
  delayLocation_ = 0;
  
  fiducialAltitude_ = 0.0;
  
  uen_.resize(3);
  
  uen_[0] = uen_[1] = uen_[2] = 0.0;
  
  actualAltitude_ = 0.0;
  
  geocentricXyz_.resize(3);
  
  geocentricXyz_[0] = 0.0;
  geocentricXyz_[1] = 0.0;
  geocentricXyz_[2] = 0.0;
  
  topocentricXyz_.resize(3);
  
  topocentricXyz_[0] = 0.0;
  topocentricXyz_[1] = 0.0;
  topocentricXyz_[2] = 0.0;
}

/**.......................................................................
 * Destructor.
 */
Location::~Location() {}

/**.......................................................................
 * A method to set a fiducial LLA point
 */
void Location::setFiducialSite(Angle longitude, Angle latitude, double altitude)
{
  fiducialLongitude_ = longitude;
  fiducialLatitude_  = latitude;
  fiducialAltitude_  = altitude;
  
  DBPRINT(true, DEBUG_DELAY, "Long is: " << longitude.degrees()
	  << "Lat is: " << latitude.degrees()
	  << "Alt is: " << altitude);

  lacking_ &= ~SITE;
  changed_  = true;
  
  // If the site has changed, we need to update coordinates
  
  updateCoordinates();
}

/**.......................................................................
 * A method to set an UEN offset relative to the fiducial
 */
void Location::setOffset(double up, double east, double north)
{
  uen_[0] = up;
  uen_[1] = east;
  uen_[2] = north;
  
  lacking_ &= ~LOCATION;
  changed_  = true;
  
  // If the offset has changed, we need to update coordinates
  
  updateCoordinates();
}

/**.......................................................................
 * Return true if a site and offset have ben set for this
 * location
 */
bool Location::canLocate()
{
  return lacking_ == NONE;
}

/**.......................................................................
 * Update coordinate representations of this location
 */
void Location::updateCoordinates()
{
  // Update the (L, L, A) representation of this location
  
  Vector<double> lla = Coordinates::
    llaAndUenToLla(longitude(true), latitude(true), altitude(true), 
		   up(), east(), north());

  actualLongitude_.setRadians(lla[0]);
  actualLatitude_.setRadians(lla[1]);
  actualAltitude_ = lla[2];
  
  DBPRINT(true, DEBUG_DELAY, "LLA coords are: " << lla);
  
  // Update the geocentric (X, Y, Z) representation of this location
  
  geocentricXyz_  = Coordinates::laAndUenToXyz(latitude(true), altitude(true), 
					       up(), east(), north(), true);
  
  DBPRINT(true, DEBUG_DELAY, "Geocentric coords are: " << geocentricXyz_);
  
  // Update the topocentric (X, Y, Z) representation of this location
  
  topocentricXyz_ = Coordinates::laAndUenToXyz(latitude(true), altitude(true), 
					       up(), east(), north(), false);
  
  DBPRINT(true, DEBUG_DELAY, "Topocentric coords are: " << topocentricXyz_);
  
  // And call back anyone who's registered
  
  if(delayLocation_ != 0)
    delayLocation_->locationChanged(this);
}

/**.......................................................................
 * Register to be called back when this object's location is updated
 */
void Location::registerLocationCallback(DelayLocation* location)
{
  DBPRINT(true, Debug::DEBUG4, "Registering location pointer: " << location);
  
  delayLocation_ = location;
}

/**.......................................................................
 * Get geometric delay for an Ha Dec source position, in
 * nanoseconds.
 */
Delay Location::geometricDelay(HourAngle ha, DecAngle dec,
			       Location& refLoc,
			       bool doMotionCorrection)
{
  // Pass earth-centered coordinates, e.g., X(true), in case we are
  // doing the motion correction
  
  DBPRINT(true, DEBUG_DELAY, "Ant X is: " << X(true)
	  << " Y is: " << Y(true)
	  << " Z is: " << Z(true));

  DBPRINT(true, DEBUG_DELAY, "Ref X is: " << refLoc.X(true)
	  << " Y is: " << refLoc.Y(true)
	  << " Z is: " << refLoc.Z(true));

  DBPRINT(true, DEBUG_DELAY, "ha is: " << ha.hours()
	  << "dec is: " << dec.degrees());

  return Coordinates::
    getGeometricDelay(ha, dec,
		      X(true), Y(true), Z(true),
		      refLoc.X(true), refLoc.Y(true), refLoc.Z(true), 
		      doMotionCorrection);
}

/**.......................................................................
 * Get geometric delay for an Az El source position, in
 * nanoseconds
 */
Delay Location::geometricDelay(Angle az, Angle el,
			       Location& refLoc)
{
  // If we have a valid reference location, it doesn't matter which
  // coordinates we pass, earth-centered or surface-relative, since
  // all positions are relative.  Note however that if the reference
  // location hasn't been specified, it will default to (0,0,0), in
  // which case we want to make sure that the antenna coordinates are
  // surface-relative.
  //
  // Make sure we use the actual latitude (latitude(false)) of this
  // location, and not the fiducial latitude.
  
  return Coordinates::
    getGeometricDelay(latitude(false), az, el, 
		      X(false)-refLoc.X(false), 
		      Y(false)-refLoc.Y(false), 
		      Z(false)-refLoc.Z(false));
}

/**.......................................................................
 * Convert mjd to lst for the actual location of this antenna
 */
HourAngle Location::getLst(double mjdUtc)
{
  // Use the actual position of this antenna

  DBPRINT(true, DEBUG_DELAY, "Long is (degrees): " << longitude(false).degrees()
	  << "Lat is (degrees): " << latitude(false).degrees());

  return astrom_.mjdUtcToLst(mjdUtc, longitude(false));
}

/**.......................................................................
 * Convert to Ha for the actual location of this antenna
 */

HourAngle Location::getHa(double mjdUtc, HourAngle ra)
{
  DBPRINT(true, DEBUG_DELAY, "RA is: " << ra.hours()
	  << "LST is: " << getLst(mjdUtc).hours());

  return getLst(mjdUtc) - ra;
}

HourAngle Location::getHa(double mjdUtc, Source* src)
{
  return getHa(mjdUtc, src->getRa(mjdUtc));
}

