#define __FILEPATH__ "util/common/Test/tCoord.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/Coord.h"
#include "gcp/util/common/CoordAxes.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));

  Coord coord;

  coord.setIndex(0, 1);

  cout << coord << endl;

  coord.setIndex(1, 4);
  coord.setIndex(2, 5);

  cout << coord << endl;

  Coord newCoord(&coord);
  cout << newCoord << endl;

  coord.reset(0);
  cout << coord << endl;

  Coord* cPtr=0;

  Coord testCoord(cPtr);
  cout << testCoord << endl;

  CoordAxes axes;
  axes.elementOffsetOf(cPtr);

  return 0;
}
