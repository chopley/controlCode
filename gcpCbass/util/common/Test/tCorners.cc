#define __FILEPATH__ "util/common/Test/tCorners.cc"

// Explore corner cases for coordinate transforms

#include <iostream>
#include <iomanip>
#include <vector>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Coordinates.h"
#include "gcp/util/common/Vector.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "long", "0", "s", "Longitude"},
  { "lat",  "0", "s", "Latitude"},
  { "alt",  "0", "f", "Altitude"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

void testAbsXyzConversions();
void testXyzConversions(bool ec);
void testAzElTransforms();
void testHaDecTransforms();

int Program::main()
{
  Debug::setLevel(getiParameter("debuglevel"));

  cout << endl << ">>>>>>>>> AbsXyz conversions <<<<<<<<<" << endl << endl;

  testAbsXyzConversions();

  cout << endl << ">>>>>>>>> earth-centered Xyz conversions <<<<<<<<<" << endl << endl;

  testXyzConversions(true);

  cout << endl << ">>>>>>>>> non earth-centered Xyz conversions <<<<<<<<<" << endl << endl;

  testXyzConversions(false);

  cout << endl << ">>>>>>>>> Az/El transforms <<<<<<<<<" << endl << endl;

  testAzElTransforms();

  cout << endl << ">>>>>>>>> Ha/Dec transforms <<<<<<<<<" << endl << endl;

  testHaDecTransforms();

  return 0;
}


void testXyzConversions(bool ec)
{
  Vector<double> xyzVals;
  Vector<double> llaVals;
  Coordinates coords;
  Angle lat;
  
  lat.setRadians(0.0);
  xyzVals = coords.laAndUenToXyz(lat, 0, 0, 0, 0, ec);
  cout << " X pole:           " << xyzVals << endl;

  lat.setRadians(0.0);
  xyzVals = coords.laAndUenToXyz(lat, 0, 0, 0, 1000, ec);
  cout << " X pole, Z = 1000: " << xyzVals << endl;

  lat.setRadians(M_PI/2);
  xyzVals = coords.laAndUenToXyz(lat, 0, 0, 0, 0, ec);
  cout << " Z pole:           " << xyzVals << endl;

  xyzVals = coords.laAndUenToXyz(lat, 0, 0, 1000, 0, ec);
  cout << " Z pole, Y=1000:   " << xyzVals << endl;

  xyzVals = coords.laAndUenToXyz(lat, 0, 0, 0, 1000, ec);
  cout << " Z pole, X=-1000:  " << xyzVals << endl;
}

void testAbsXyzConversions()
{
  Vector<double> xyzVals;
  Vector<double> llaVals;
  Coordinates coords;
  Angle lon, lat;

  lat.setRadians(0.0);
  lon.setRadians(-M_PI/2);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << "-Y pole:    " << xyzVals << endl;

  lon.setRadians(M_PI/2);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << " Y pole:    " << xyzVals << endl;

  lon.setRadians(0.0);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << " X pole:    " << xyzVals << endl;

  lon.setRadians(M_PI);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << "-X pole:    " << xyzVals << endl;

  lon.setRadians(0.0);
  lat.setRadians(M_PI/2);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << " Z pole:    " << xyzVals << endl;

  lon.setRadians(0.0);
  lat.setRadians(-M_PI/2);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << "-Z pole:    " << xyzVals << endl;

  lon.setRadians(-M_PI/2);
  lat.setRadians(-M_PI/2);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << "-Z pole:    " << xyzVals << endl;

  lon.setRadians(0.0);
  lat.setRadians(M_PI/4);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << " Equal X-Z: " << xyzVals << endl;

  lon.setRadians(M_PI/2);
  lat.setRadians(M_PI/4);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << " Equal Y-Z: " << xyzVals << endl;

  lon.setRadians(M_PI/4);
  lat.setRadians(0.0);
  xyzVals = coords.llaAndUenToAbsXyz(lon, lat, 0, 0, 0, 0);
  cout << " Equal X-Y: " << xyzVals << endl;

  cout << endl << ">>>>>>>>> absXyzToLla conversions <<<<<<<<<" << endl << endl;

  xyzVals = coords.absXyzAndUenToLla(Coordinates::earthEqRadiusMeters_, 0, 0, 0, 0, 0);
  cout << " X pole:    " << xyzVals << endl;

  xyzVals = coords.absXyzAndUenToLla(-Coordinates::earthEqRadiusMeters_, 0, 0, 0, 0, 0);
  cout << "-X pole:    " << xyzVals << endl;

  xyzVals = coords.absXyzAndUenToLla(0, Coordinates::earthEqRadiusMeters_, 0, 0, 0, 0);
  cout << " Y pole:    " << xyzVals << endl;

  xyzVals = coords.absXyzAndUenToLla(0, -Coordinates::earthEqRadiusMeters_, 0, 0, 0, 0);
  cout << "-Y pole:    " << xyzVals << endl;

  xyzVals = coords.absXyzAndUenToLla(0, 0, Coordinates::earthEqRadiusMeters_, 0, 0, 0);
  cout << " Z pole:    " << xyzVals << endl;

  xyzVals = coords.absXyzAndUenToLla(0, 0, -Coordinates::earthEqRadiusMeters_, 0, 0, 0);
  cout << "-Z pole:    " << xyzVals << endl;

  xyzVals = coords.absXyzAndUenToLla(4510026.04536, 0, 4510026.04536, 0, 0);
  cout << " Equal X-Z: " << xyzVals << endl << endl;;

  xyzVals = coords.absXyzAndUenToLla(0, 4510026.04536, 4510026.04536, 0, 0, 0);
  cout << " Equal Y-Z: " << xyzVals << endl << endl;;

  xyzVals = coords.absXyzAndUenToLla(4510026.04536, 4510026.04536, 0, 0, 0, 0);
  cout << " Equal X-Y: " << xyzVals << endl << endl;;
}


void testAzElTransforms()
{
  Coordinates coords(0.0, 0.0, 0);
  Angle lon, lat;

  lon.setRadians(0.0);
  lat.setRadians(0.0);

  Vector<double> llaVals = 
    coords.llaAndUenToLla(lon, lat, 0, -Coordinates::earthEqRadiusMeters_, 
			  Coordinates::earthEqRadiusMeters_+1000, 0);

  cout << "Y pole (offsetting from X pole):     " << llaVals << endl;

  cout << "Az El of Y pole from X pole:         ";
  Vector<Angle> azelVals = coords.uenToAzEl(-Coordinates::earthEqRadiusMeters_, Coordinates::earthEqRadiusMeters_+1000, 0);
  cout << "(" << azelVals[0]
       << ", " << azelVals[1] << ")" << endl;

  lon.setRadians(llaVals[0]);
  lat.setRadians(llaVals[1]);
  cout << "Az El of Y pole from X pole:         ";
  azelVals = coords.getAzEl(lon, lat, llaVals[2]);
  cout << "(" << azelVals[0]
       << ", " << azelVals[1] << ")" << endl;
}

//-----------------------------------------------------------------------
// Test Ha/Dec -> Az/El transforms

void testHaDecTransforms()
{
  HourAngle ha;
  DecAngle dec;
  Angle lat;
  Vector<Angle> azelVals;
  Vector<double> dvals(2);

  cout << "Source at dec=90, HA=0 from equator:                         ";
  lat.setRadians(0.0);
  ha.setRadians(0.0);
  dec.setRadians(M_PI/2);
  azelVals = Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec);
  dvals[0] = azelVals[0].radians();
  dvals[1] = azelVals[1].radians();
  cout << dvals << endl;

  cout << "Source at dec=90, HA=0 from equator:                         ";
  azelVals = Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec, Coordinates::INF);
  dvals[0] = azelVals[0].radians();
  dvals[1] = azelVals[1].radians();
  cout << dvals << endl;

  cout << "Source at dec=0, HA=0 from equator:                          ";
  lat.setRadians(0.0);
  ha.setRadians(0.0);
  dec.setRadians(0.0+0.000001);
  azelVals = Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec);
  dvals[0] = azelVals[0].radians();
  dvals[1] = azelVals[1].radians();
  cout << dvals << endl;

  cout << "Source at dec=0, HA=0, R=Rearth from Z pole (topocentric):   "; 
  lat.setRadians(M_PI/2);
  ha.setRadians(0.0);
  dec.setRadians(0.0);
  azelVals = Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec, false, 
					   Coordinates::ACTUAL, 
					   (Coordinates::earthEqRadiusMeters_)/Coordinates::auToMeters_);
    dvals[0] = azelVals[0].radians();
    dvals[1] = azelVals[1].radians();
    cout << dvals << endl;

  cout << "Source at dec=0, HA=-6 from equator: (geocentric)            ";
  lat.setRadians(0.0);
  ha.setHours(-6);
  dec.setRadians(0.0);
  azelVals = Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec);
  dvals[0] = azelVals[0].radians();
  dvals[1] = azelVals[1].radians();
  cout << dvals << endl;

  cout << "Source at dec=0, HA=-6, R=Rearth from equator (geocentric):  ";
  azelVals = 
    Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec, 
				  true, Coordinates::ACTUAL, 
				  Coordinates::earthEqRadiusMeters_/
				  Coordinates::auToMeters_);
  dvals[0] = azelVals[0].radians();
  dvals[1] = azelVals[1].radians();
  cout << dvals << endl;
  
  cout << "Source at dec=90, HA=-6, R=Rearth from equator (topocentric): ";
  lat.setRadians(0.0);
  ha.setHours(-6);
  dec.setRadians(M_PI/2);
  azelVals = 
    Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec, false, 
				  Coordinates::ACTUAL, 
				  (Coordinates::earthEqRadiusMeters_)/
				  Coordinates::auToMeters_);
  dvals[0] = azelVals[0].radians();
  dvals[1] = azelVals[1].radians();
  cout << dvals << endl;

  cout << "Source at dec=0, HA=+6, R=Rearth from equator (topocentric): ";
  lat.setRadians(0.0);
  ha.setHours(+6);
  dec.setRadians(0.0);
  azelVals = 
    Coordinates::laAndHaDecToAzEl(lat, 0.0, ha, dec, false, 
				  Coordinates::ACTUAL, 
				  (Coordinates::earthEqRadiusMeters_)/
				  Coordinates::auToMeters_);
  dvals[0] = azelVals[0].radians();
  dvals[1] = azelVals[1].radians();
  cout << dvals << endl;
}


  

  
