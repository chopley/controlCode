#include <iostream>
#include <iomanip>

#include <cmath>

#include "gcp/program/common/Program.h"
#include "gcp/util/common/GpibUsbController.h"
#include "gcp/util/common/HpSynthesizer.h"
#include "gcp/util/common/LsThermal.h"
#include "gcp/util/common/Frequency.h"
#include "gcp/util/common/Power.h"
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

  COUT("Version is:   '" << gpib.getVersion()  << "'");

  HpSynthesizer synth(gpib);
  Agilent33220AWaveformGenerator dev2(gpib);
  LsThermal dev3(gpib);

  synth.setAddress(3);
  dev2.setAddress(12);
  dev3.setAddress(11);

  COUT("Help is:    '" << gpib.getHelp()  << "'");
  COUT("Eos is:     '" << gpib.getEos()  << "'");
  COUT("Eoi is:     '" << gpib.getEoi()  << "'");

  COUT("Device1 is: '"   << synth.getDevice()     << "'");
  COUT("Frequency is: '" << synth.getFrequency().MHz()  << "'");

  Frequency freq;
  freq.setMHz(995);
  freq = synth.setFrequency(freq);

  Power pow;
  pow.setdBm(10);
  synth.setOutputPower(pow);

  sleep(1);

  gpib.stop();

  return 0;
}
