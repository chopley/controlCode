#define __FILEPATH__ "antenna/control/Test/tCanMaster.cc"

#include <pthread.h>
#include <iostream>

#include "gcp/util/common/Directives.h"

#include "gcp/antenna/canbus/CanMaster.h"

#include "gcp/util/common/Debug.h"

using namespace std;
using namespace gcp::antenna::canbus;

// If the CARMA environment is present, use carma::util::Program,
// otherwise use our own version of Program.

#if DIR_HAVE_CARMA
#include "carma/util/Program.h"
using namespace carma::util;
#else
#include "gcp/program/common/Program.h"
using namespace gcp::program;
#endif

// Define recognized Program keywords

KeyTabEntry Program::keywords[] = {
  { "antenna",     "0",                         "i", "Antenna number"},
  { "host",        "cntrl.localnet",            "s", "ACC control host"},
  { "nameserver",  "nameserver.localnet:4000",  "s", "Name Server host:port"},
  { "eventserver", "eventserver.localnet:4001", "s", "Event Server host:port"},
  { "notifyserver","notifyserver.localnet:4006","s", "Notification Server host:port"},
  { "debuglevel",  "0",                         "i", "Debugging level (0==off)"},
  { "simcanbus",   "t",                         "b", "Simulate having a CANbus?"},
  { "simpmac",     "f",                         "b", "Simulate having a PMAC?"},
  { END_OF_KEYWORDS},
};

/*.......................................................................
 * Create an AntennaMaster object with subsystems running in separate
 * threads.
 *
 * Defining Program::main() automatically gives us the
 * command-line parsing in Program.  
 */
int carma::util::Program::main(void)
{
  gcp::util::Debug::setLevel(Program::getiParameter("debuglevel"));

  CanMaster* master = 0;

  master = new CanMaster();

  if(master != 0)
    delete master;
}

