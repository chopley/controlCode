#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/GpibUsbController.h"
#include "gcp/util/common/LsThermal.h"
#include "gcp/util/common/Agilent33220AWaveformGenerator.h"

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

  GpibUsbController gpib(Program::getParameter("dev"), true);

  sleep(1);

  gpib.connectAndClear();

  gpib.setEoi(true);
  gpib.setAuto(false);
  gpib.setEos(GpibUsbController::EOS_CRLF);
  gpib.setEotChar('\n');
  gpib.enableEot(true);

  COUT("Version is: '" << gpib.getVersion()  << "'");

  GpibUsbDevice dev1(gpib);
  Agilent33220AWaveformGenerator dev2(gpib);
  LsThermal dev3(gpib);

  dev1.setAddress(3);
  dev2.setAddress(12);
  dev3.setAddress(11);

  COUT("Help is:    '" << gpib.getHelp()  << "'");
  COUT("Eos is:     '" << gpib.getEos()  << "'");
  COUT("Eoi is:     '" << gpib.getEoi()  << "'");

  COUT("Device3 is: '" << dev3.getDevice()  << "'");
  COUT("Device1 is: '" << dev1.getDevice()  << "'");
  COUT("Device2 is: '" << dev2.getDevice()  << "'");

#if 0
  TimeVal start,stop,diff;

  std::vector <float> ch1;

  start.setToCurrentTime();
  ch1 = dev3.requestMonitor();
  ch1 = dev3.requestMonitor();
  ch1 = dev3.requestMonitor();
  ch1 = dev3.requestMonitor();
  ch1 = dev3.requestMonitor();
  stop.setToCurrentTime();

  diff = stop-start;
  COUT("Diff was: " << diff.getTimeInSeconds());

  sleep(1);

  //  dev2.setOutputType(Agilent33220AWaveformGenerator::SINUSOID);

  sleep(1);

  //  COUT("Error was: " << dev2.getLastError());

  sleep(1);

#endif

  sleep(1);

  gpib.stop();

  return 0;
}
