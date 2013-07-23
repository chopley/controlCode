#define __FILEPATH__ "util/common/Geoid.cc"

#include "gcp/util/common/Geoid.h"

using namespace std;
using namespace gcp::util;

// Set the equatorial radius of the earth.  Value is IAU equatorial
// radius, in cm, from Explanatory Supplement p. 161

const Length Geoid::earthEqRadius_ = 
Length(Length::Centimeters(), 637814000.0);

const double Geoid::flattening_ = 1.0/298.257;

/**.......................................................................
 * Constructor.
 */
Geoid::Geoid() {}

/**.......................................................................
 * Destructor.
 */
Geoid::~Geoid() {}

/**.......................................................................
 * Return the length of the radius vector from the center of the
 * earth to the surface at a given geocentric latitude.
 */
Length Geoid::geocentricRadius(Angle geocentricLatitude)
{
  return radius(geocentricLatitude);
}

/**.......................................................................
 * Return the length of the radius vector normal to the surface
 * at a given geodetic latitude.
 *
 * From:
 *
 *     geodR * sin(geodLat) = geocR * sin(geocLat),
 *
 * we have 
 *
 *     geodR  = geocR * sin(geocLat)/sin(geodLat)
 *
 */
Length Geoid::geodeticRadius(Angle geodLat)
{
  Angle geocLat  = geocentricLatitude(geodLat);
  Length geocRad = geocentricRadius(geodLat);

  Length geodRad = geocRad * (sin(geocLat.radians())/sin(geodLat.radians()));

  return geodRad;
}

/**.......................................................................
 * Return the geocentric latitude corresponding to a given
 * geodetic latitude
 *
 * Geocntric latitude and Geodetic latitude are related by:
 *
 *     tan(geoc) = (b/a)^2 * tan(geod)
 *
 *  Flattening is defined as: f = 1.0 - (b/a), so (b/a) = (1 - f)
 */
Angle Geoid::geocentricLatitude(Angle geodeticLatitude)
{
  double df = (1.0 - flattening());
  double df2 = df * df;
  double tGeocLat = df2 * tan(geodeticLatitude.radians());

  Angle geocLat;
  geocLat.setRadians(atan(tGeocLat));

  return geocLat;
}

/**.......................................................................
 * Return the eccentricity squared
 *
 * Eccentricity is defined as:
 *
 *             1 - e^2 = (b/a)^2
 *
 * where a, b = semi-major, semi-minor radii of the ellipse.
 *
 * flattening is defined as:
 *
 *             f = (a - b)/a = 1 - (b/a)
 *
 * Thus:  (b/a)^2 = (1 - f)^2 = 1 - e^2, or
 *
 *        e^2 = 1 - (1 - f)^2 = 2f - f^2 = f(2 - f)
 *
 */
double Geoid::eccentricitySquared()
{
  return flattening() * (2 - flattening());
}

