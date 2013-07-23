#define __FILEPATH__ "util/common/Test/tRegMapDataFrameManager.cc"

#include <iostream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/ArrayRegMapDataFrameManager.h"
#include "gcp/util/common/CoordRange.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/MonitorPointManager.h"
#include "gcp/util/common/MonitorPoint.h"

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

  gcp::util::ArrayRegMapDataFrameManager adf;
  MonitorPointManager monitor(&adf);
  MonitorPoint* mp=0;
  CoordRange range;

  range.setStartIndex(0, 0);
  range.setStopIndex(0, 0);

  mp = monitor.addMonitorPoint("dcon", "psys", &range); 

  return 0;
}
