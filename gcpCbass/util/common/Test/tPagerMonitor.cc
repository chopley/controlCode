#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/util/common/Test/Program.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/PagerMonitor.h"
#include "gcp/util/common/MonitorPoint.h"

#include "gcp/program/common/Program.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  { "debuglevel",  "0", "i", "Debug level"},
  { END_OF_KEYWORDS}
};

MONITOR_CONDITION_HANDLER(handler)
{
  COUT("Handler got called with message: " << message);
}

void Program::initializeUsage() {};

int Program::main()
{
  ArrayDataFrameManager arrman;
  PagerMonitor pm(&arrman);
  pm.setHandler(handler);

  double min = 10, max = 100;

  COUT("Here 0");
  pm.addOutOfRangeMonitorPoint("antenna0.thermal.lsTemperatureSensors[0]", 0, 40, 0, 5);
  COUT("Here 1\n");

  min = 10, max = 200;

  for(unsigned i=0; i < 15; i++) {
    COUT("Checking: i = " << i);
    arrman.writeReg("antenna0",   "thermal",   "lsTemperatureSensors", (float) 103.0);
    pm.checkRegisters();
  }

  pm.clear();

  return 0;
}
