#define __FILEPATH__ "util/common/Test/tCoordRange.cc"

#include <iostream>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/CoordRange.h"
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

  CoordRange range1, range2;

  range1.setIndex(0,0);

  range1.setStartIndex(1,1);
  range1.setStopIndex(1,4);

  range2 = range1;

  cout << range1 << endl;
  cout << range2 << endl;
  return 0;
}
