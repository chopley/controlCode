#define __FILEPATH__ "util/common/Ellipsoid.cc"

#include "gcp/util/common/Ellipsoid.h"

#include <cmath>

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Ellipsoid::Ellipsoid() {}

/**.......................................................................
 * Constructor.
 */
Ellipsoid::Ellipsoid(Length majorAxis, Length minorAxis) 
{
  a_  = majorAxis;
  b_  = minorAxis;
}

/**.......................................................................
 * Destructor.
 */
Ellipsoid::~Ellipsoid() {}

/**.......................................................................
 * Return the length of the vector from the center of the
 * ellipsoid to the surface at a given ellipsoidal latitude
 */
double Ellipsoid::flattening()
{
  return 1.0 - (b_/a_);
}

/**.......................................................................
 * Return the eccentricity e
 */
double Ellipsoid::eccentricity()
{
  return sqrt(eccentricitySquared());
}
 
double Ellipsoid::eccentricitySquared()
{
  return flattening() * (2.0 - flattening());
}

/**.......................................................................
 * Return the length of the radius vector at a given ellipsoidal
 * latitude
 */
Length Ellipsoid::radius(Angle latitude)
{
  double slat  = sin(latitude.radians());
  double slat2 = slat * slat;
  double e2    = eccentricitySquared();

  double num = 1.0 - e2 * (2.0 - e2) * slat2;
  double den = 1.0 - e2 * slat2;

  return a_ * sqrt(num/den);
}
