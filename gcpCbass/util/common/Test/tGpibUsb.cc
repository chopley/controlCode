#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/GpibUsbController.h"
#include "gcp/util/common/HpDcPowerSupply.h"

using namespace std;
using namespace gcp::program;
using namespace gcp::util;

KeyTabEntry Program::keywords[] = {
  { "dev",        "/dev/ttyUSB0", "s", "device"},
  { "volts",      "5", "f", "voltage"},
  { END_OF_KEYWORDS}
};

void Program::initializeUsage() {};

int Program::main()
{
  // Connect first GPIB device


  COUT("Atempting to connect to device: " << Program::getParameter("dev"));

  GpibUsbController gpib1(Program::getParameter("dev"), true);

  //  gpib1.spawn();
  gpib1.connect();

  // Now reset the interface

  gpib1.clearInterface();

  COUT("Version is:  '" << gpib1.getVersion() << "'");

  COUT("Help:  '" << gpib1.getHelp() << "'");

  gpib1.stop();

  sleep(1);

  return 0;
}
