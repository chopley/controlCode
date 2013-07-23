#define __FILEPATH__ "util/common/Test/tAxisRange.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/AxisRange.h"
#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::util;using namespace gcp::program;void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { "start",       "0", "i", "Start byte index"},
  { "stop",        "0", "i", "Stop byte index"},
  { "archive",     "t", "b", "Archived only?"},
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

int Program::main()
{
  Debug::setLevel(Program::getiParameter("debuglevel"));

  AxisRange range(10);

  for(range.reset(); !range.isEnd(); ++range)
    cout << range << endl;

  CoordAxes axes(2,3,10);
  CoordRange coordRange;

  coordRange.setStartIndex(0, 0);
  coordRange.setStopIndex(0, 1);

  coordRange.setStartIndex(1, 1);
  coordRange.setStopIndex(1, 2);

  coordRange.setStartIndex(2, 2);
  coordRange.setStopIndex(2, 5);

  cout << endl;

  cout << "Range is: " << coordRange 
       << " for axes: " << axes << endl;

  AxisRange newRange(axes, coordRange);
  for(newRange.reset(); !newRange.isEnd(); ++newRange)
    cout << newRange << endl;

  return 0;
}
