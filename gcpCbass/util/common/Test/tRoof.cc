#define __FILEPATH__ "util/common/Test/tRoof.cc"

#include <iostream>
#include <cmath>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Coordinates.h"
#include "gcp/util/common/Vector.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "dist", "0", "f", "Distance"},
  { "u",    "0", "f", "Up"},
  { "e",    "0", "f", "East"},
  { "n",    "0", "f", "North"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));

  Vector<Angle> azelVals;
  Angle lon, lat;

  // Fiducial location of ant2

  lon.setDegrees("-118:17:45.9");
  lat.setDegrees("37:13:57.5");

  Coordinates ant2(lon, lat, 1208);

  // Az/El of the roof source from ant2

  double ant2Az = 152.0/180*M_PI;
  double ant2El = 10.8/180*M_PI;

  // Get the distance of the source

  double D = Program::getdParameter("dist");

  // Now construct the location of the source

  Coordinates roof(ant2), ant(ant2);
  roof.add(D*sin(ant2El), D*cos(ant2El)*sin(ant2Az), D*cos(ant2El)*cos(ant2Az));

  // Get the offset of the new antenna from ant2

  double U = Program::getdParameter("u");
  double E = Program::getdParameter("e");
  double N = Program::getdParameter("n");

  ant.add(U, E, N);

  // And get the Az/El of the source relative to the new antenna

  azelVals = ant.getAzEl(roof);

  cout << "Az El is: " << azelVals[0] << ", " << azelVals[1] << endl;

  return 0;
}
