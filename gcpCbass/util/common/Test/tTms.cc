#define __FILEPATH__ "util/common/Test/tTms.cc"

#include <iostream>

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
  { "n",    "0", "f", "North"},
  { "e",    "0", "f", "East"},
  { "u",    "0", "f", "Up"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));

  Vector<double> tmsVals;

  tmsVals = Coordinates::laAndUenToXyz(getParameter("lat"),
				       getdParameter("alt"),
				       getdParameter("u"),
				       getdParameter("e"),
				       getdParameter("n"));



  cout << "TMS coordinates are: " << tmsVals << endl;

  return 0;
}
