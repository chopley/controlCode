#include "gcp/util/common/Agilent33220AWaveformGenerator.h"

using namespace std;

using namespace gcp::util;

/**.......................................................................
 * Constructor.
 */
Agilent33220AWaveformGenerator::Agilent33220AWaveformGenerator(GpibUsbController& controller) : GpibUsbDevice(controller) {}

/**.......................................................................
 * Destructor.
 */
Agilent33220AWaveformGenerator::~Agilent33220AWaveformGenerator() {}

void Agilent33220AWaveformGenerator::setOutputType(OutputType type)
{
  std::ostringstream os;
  os << "APPL:";

  switch (type) {
  case Agilent33220AWaveformGenerator::SINUSOID:
    os << "SIN";
    break;
  case Agilent33220AWaveformGenerator::SQUARE:
    os << "SQU";
    break;
  case Agilent33220AWaveformGenerator::RAMP:
    os << "RAMP";
    break;
  case Agilent33220AWaveformGenerator::PULSE:
    os << "PULS";
    break;
  case Agilent33220AWaveformGenerator::NOISE:
    os << "NOIS";
    break;
  case Agilent33220AWaveformGenerator::DC:
    os << "DC";
    break;
  case Agilent33220AWaveformGenerator::USER:
    os << "USER";
    break;
  }

  sendDeviceCommand(os.str());
  return;
}

std::string Agilent33220AWaveformGenerator::getLastError()
{
  std::string retVal;
  sendDeviceCommand("SYST:ERR?", true, GpibUsbController::checkString, true, (void*)&retVal);
  return retVal;
}
