#include <iostream>
#include <sstream>
#include <cmath>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TipperServer.h"
#include "gcp/util/common/Ports.h"

#include "gcp/program/common/Program.h"

using namespace std;
using namespace gcp::util;
using namespace gcp::program;

KeyTabEntry Program::keywords[] = {
  {END_OF_KEYWORDS}
};

void Program::initializeUsage() {}

int Program::main()
{
  TipperServer server("smmtipper", false);

  try {
    server.run();
  } catch (Exception& err) {
    COUT("Caught an exception: " << err.what());
  } catch (...) {
    COUT("Caught an exception");
  }

  COUT("Exiting");

  return 0;
}

