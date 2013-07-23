#include <iostream>
#include <sstream>

#include "gcp/program/common/Program.h"

#include "gcp/util/common/Exception.h"
#include "gcp/util/common/FdSet.h"
#include "gcp/util/common/Profiler.h"

#include "gcp/antenna/control/specific/GpsIntHandler.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;
using namespace gcp::antenna::control;

#define TFP_DEBUG(statement) \
{\
    os_.str("");\
    os_ << ": " << statement << std::endl; \
    logFile_.append(os_.str(), true); \
}

void Program::initializeUsage() {};

KeyTabEntry Program::keywords[] = {
  { END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS,END_OF_KEYWORDS},
};

int Program::main()
{
  GpsIntHandler gps(false, false, true);

  gps.run();

  return 0;
}
