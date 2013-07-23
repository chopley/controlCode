#include "gcp/antenna/control/specific/PointingCorrections.h"

#include <cmath>

using namespace std;
using namespace gcp::antenna::control;

/**.......................................................................
 * Constructor.
 */
PointingCorrections::PointingCorrections()
{
  az      = 0.0; // The apparent azimuth of the source 
  el      = 0.0; // The apparent elevation of the source 
  pa      = 0.0; // The apparent parallactic angle of the source 
  lat     = 0.0; // The corrected latitude of the source 
  sin_az  = 0.0; // sin(az) 
  cos_az  = 0.0; // cos(az) 
  sin_el  = 0.0; // sin(el) 
  cos_el  = 0.0; // cos(el) 
  sin_lat = 0.0; // sin(lat) 
  cos_lat = 0.0; // cos(lat) 
}
/**.......................................................................
 * Constructor.
 */
PointingCorrections::PointingCorrections(double az0, double el0, double pa0)
{
  az      = az0; // The apparent azimuth of the source 
  el      = el0; // The apparent elevation of the source 
  pa      = pa0; // The apparent parallactic angle of the source 
  lat     = 0.0; // The corrected latitude of the source 
  sin_az  = sin(az); // sin(az) 
  cos_az  = cos(az); // cos(az) 
  sin_el  = sin(el); // sin(el) 
  cos_el  = cos(el); // cos(el) 
  sin_lat = 0.0; // sin(lat) 
  cos_lat = 0.0; // cos(lat) 
}
