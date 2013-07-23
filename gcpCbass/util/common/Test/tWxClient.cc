#include <iostream>
#include <sstream>
#include <cmath>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/WxClient40m.h"
#include "gcp/util/common/Ports.h"
#include "gcp/util/common/SshTunnel.h"

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
  // Now connect to our local port

  try {
    COUT("About to try connecting to local port");

    WxClient40m client(false, "localhost");

    client.run();
  } catch (Exception& err) {
    COUT(err.what());
  } catch(...) {
    COUT("Caught an unknown exception");
  }

  COUT("Exiting");

  return 0;
}

