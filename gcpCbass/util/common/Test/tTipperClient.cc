#include <iostream>
#include <sstream>
#include <cmath>

#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Exception.h"
#include "gcp/util/common/LogStream.h"
#include "gcp/util/common/TipperClient.h"
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

class Ssh : public gcp::util::Runnable {
public:

  Ssh() : Runnable(true, runFn) {};

  virtual ~Ssh() {};

  static RUN_FN(runFn) {
    Ssh* runnable = (Ssh*) arg;

    unsigned short port = Ports::tipperPort("bicep");
    ostringstream os;
    os << "ssh -L " << port << ":bicep3:" << port << " bicep";
    
    COUT("About to call system with string: " << os.str());
    system(os.str().c_str());
    runnable->blockForever();
  }
};

int Program::main()
{
  //  SshTunnel ssh("bicep", "bicep3", Ports::tipperPort("bicep"));

  // Now connect to our local port

  try {
    COUT("About to try connecting to local port");

    TipperClient client(false, "localhost",
			Ports::tipperPort("bicep"));
    client.run();
  } catch (Exception& err) {
    COUT(err.what());
  } catch(...) {
    COUT("Caught an unknown exception");
  }

  COUT("Exiting");

  return 0;
}

