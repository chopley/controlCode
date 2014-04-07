#define __FILEPATH__ "antenna/control/specific/antennaControl.cc"

#include <pthread.h>
#include <iostream>

#include "gcp/antenna/control/specific/AntennaMaster.h"

#include "gcp/util/common/AntNum.h"
#include "gcp/util/common/Debug.h"
#include "gcp/util/common/Logger.h"

#include "gcp/util/specific/Directives.h"

using namespace std;
using namespace gcp::antenna::control;
using namespace gcp::util;

#include "gcp/program/common/Program.h"
using namespace gcp::program;

// Define recognized Program keywords

KeyTabEntry Program::keywords[] = {
  { "host",        "localhost",                 "s", "mediator host"},
  { "simpmac",      "f",                        "b", "Simulate having a PMAC?"},
  { "simgpib",      "f",                        "b", "Simulate having a GPIB network?"},
  { "simdlp",       "f",                        "b", "Simulate having a DLP temperature sensor bus?"},
  { "useprio",      "f",                        "b", "Run in privileged mode?"},
  { "simlna",       "f",                        "b", "Simulate having a LNA temperature sensor bus?"},
  { "simadc",       "f",                        "b", "Simulate the Labjack ADC?"},
  { "simroach1",    "f",                        "b", "Simulate having roach1?"},
  { "simroach2",    "f",                        "b", "Simulate having roach2?"},
  { END_OF_KEYWORDS},
};

void Program::initializeUsage() {};

/*.......................................................................
 * Create an AntennaMaster object with subsystems running in separate
 * threads.
 *
 * Defining Program::main() automatically gives us the
 * command-line parsing in Program.  
 */
int Program::main(void)
{
  // And create the object which will instantiate the control system

  AntennaMaster master(Program::getParameter("host"), 
		       Program::getbParameter("simpmac"),
		       Program::getbParameter("simgpib"),
		       Program::getbParameter("simdlp"),
		       Program::getbParameter("useprio"),
		       Program::getbParameter("simlna"),
		       Program::getbParameter("simadc"),
		       Program::getbParameter("simroach1"),
		       Program::getbParameter("simroach2"));

  return 0;
}

