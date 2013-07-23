#include "gcp/util/common/LsThermal.h"
#include "gcp/util/common/Timer.h"
#include "gcp/antenna/control/specific/SpecificShare.h"

#include "gcp/control/code/unix/libunix_src/common/tcpip.h"
#include "gcp/control/code/unix/libunix_src/common/regmap.h"

using namespace std;
using namespace gcp::util;

/**.......................................................................
 * Constructors
 */
LsThermal::LsThermal(bool doSpawn) : GpibUsbDevice(doSpawn) {}
LsThermal::LsThermal(std::string port, bool doSpawn) : GpibUsbDevice(port, doSpawn) {}
LsThermal::LsThermal(GpibUsbController& controller) : GpibUsbDevice(controller) {}

/**.......................................................................
 * Destructor.
 */
LsThermal::~LsThermal() {}

/**.......................................................................
 * Request a single temperature
 */
std::vector<float> LsThermal::requestMonitor(int monitorNumber)
{
  std::vector<float> returnValues;
  std::string retVal;
  std::ostringstream os;
  int numValues = 8;
  int i;	

  os << "KRDG? ";

  if(monitorNumber >= 1) {
    os << monitorNumber;
    numValues = 1;
  };
 
  Timer timer;
  //COUT("Timer in Lakeshore")	
  timer.start();
  sendDeviceCommand(os.str(), true, GpibUsbController::checkString, true, (void*)&retVal);
  timer.stop();

  returnValues = parseRegularResponse(retVal, numValues);
  //  COUT("the return value is" <<retVal<< "yebo");
  for(i=0;i<numValues;i++){
    //  	COUT("the return2 value is" <<returnValues[i]<< "yebo");
	}
  return returnValues;
};

/**.......................................................................
 * Request the Analog output for a given data.
 */
std::vector<float>  LsThermal::requestAnalogOutput(int monitorNumber)
{
  std::string retVal;
  std::ostringstream os;
  std::vector<float> returnValues;

  os << "AOUT?" << monitorNumber;
  sendDeviceCommand(os.str(), true, GpibUsbController::checkString, true, (void*)&retVal);
  returnValues = parseRegularResponse(retVal);
  COUT("RESPONSE PARSED");
  
  return returnValues;
};

/**.......................................................................
 * Reset the module
 */
void LsThermal::resetModule()
{
  std::ostringstream os;

  os << "*RST";
  sendDeviceCommand(os.str());
  return;
};

std::string LsThermal::queryDataLog()
{
  std::string retVal;
  std::ostringstream os;
  sendDeviceCommand("LOGVIEW?", true, GpibUsbController::checkString, true, (void*)&retVal);
  return retVal;
}


/**.......................................................................
 * Parse the string and turn it into floats.
 */
std::vector<float> LsThermal::parseRegularResponse(std::string responseString, int numValues)
{

  int i;
  std::vector<float> response(numValues);
  std::string compPlus = "+";
  std::string compMinus = "-";

  bool isValid = 0;
  bool fail = 0;
  int stringLength;

  while(!isValid) {
    // First we have to check if the first character is a +/-.
    // Sometimes the gpib response has a spurious character at the
    // beginning.
    stringLength = responseString.length();
    //COUT("Lakeshore length "<<stringLength);
    int isPlus  = responseString.compare(0, 1, compPlus);
    int isMinus = responseString.compare(0, 1, compMinus);

    if(isPlus==0 || isMinus==0){
      isValid = 1;
      fail = 0;
    } else {
      if(stringLength > 1){
	responseString = responseString.substr(1, stringLength);
      } else {
	isValid = 1; 
	fail = 1;
      };
    };
  }

  if(fail){
    ThrowError("Response not recognized");
    COUT("Lakeshore failed");
  }

  std::string junk;
  // otherwise the response is good, and we should check them.
  String retString(responseString);
  for (i=0;i<numValues; i++) { 
    junk = retString.findNextStringSeparatedByChars(", ").str();
    response[i] = atof(junk.c_str());
    //COUT("Lakeshore Values"<<response[i]);
  };
  return response;
}


